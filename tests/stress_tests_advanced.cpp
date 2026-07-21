/*
    Advanced Stress Tests for resource_pool

    These tests focus on extreme scenarios, edge cases, and adversarial conditions
    to ensure the resource pool implementation is robust and reliable under stress.

    Test Categories:
    1. Extreme Concurrency Tests - High thread counts with various patterns
    2. Resource Lifecycle Tests - Complex creation/destruction scenarios
    3. Memory Pressure Tests - Large objects and rapid allocation
    4. Contention Tests - Competing threads with limited resources
    5. Fairness Tests - Ensuring fair resource distribution
    6. Chaos Tests - Random operations and timing variations
    7. Capacity Tests - Boundary conditions and limits
    8. Recovery Tests - Behavior after errors and exceptions
*/

#include "gtest/gtest.h"
#include <memory>
#include <thread>
#include <format>
#include <atomic>
#include <vector>
#include <barrier>
#include <chrono>
#include <random>
#include <queue>
#include <numeric>
#include <algorithm>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/resource_pool.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

namespace
{
    // Helper to measure operation timing
    struct TimingStats
    {
        std::vector<std::chrono::microseconds> durations;

        void                                   record(std::chrono::microseconds duration) { durations.push_back(duration); }

        std::chrono::microseconds              min() const { return *std::min_element(durations.begin(), durations.end()); }

        std::chrono::microseconds              max() const { return *std::max_element(durations.begin(), durations.end()); }

        std::chrono::microseconds              avg() const
        {
            if (durations.empty()) return std::chrono::microseconds(0);
            auto sum = std::accumulate(durations.begin(), durations.end(), std::chrono::microseconds(0));
            return sum / durations.size();
        }
    };

    // Wrapper type for small objects (int is arithmetic and not allowed)
    struct SmallObject
    {
        int value;
        SmallObject(int v = 0)
            : value(v)
        {
        }
    };
} // namespace

// ============================================================================
// EXTREME CONCURRENCY TESTS
// ============================================================================

/// @brief Test with maximum thread count and high iteration count
/// Validates thread safety under extreme concurrency
TEST(stress_extreme_concurrency, max_threads_high_iterations)
{
    constexpr int                                POOL_SIZE    = 32;
    constexpr int                                THREAD_COUNT = 64;
    constexpr int                                ITERATIONS   = 1000;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::atomic_int           total_checkouts {0};
    std::atomic_int           total_exceptions {0};
    std::barrier              start_barrier {THREAD_COUNT};

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < ITERATIONS; ++i) {
                try {
                    auto res = pool.checkout();
                    total_checkouts++;
                    // Minimal work
                    std::this_thread::yield();
                }
                catch (const std::runtime_error&) {
                    total_exceptions++;
                }
            }
        });
    }

    threads.clear();

    EXPECT_EQ(static_cast<size_t>(POOL_SIZE), pool.size());
    EXPECT_GT(total_checkouts.load(), 0);
    EXPECT_EQ(THREAD_COUNT * ITERATIONS, total_checkouts.load() + total_exceptions.load());
}

/// @brief Test with alternating thread creation and destruction
/// Validates pool behavior with dynamic thread lifecycle
TEST(stress_extreme_concurrency, dynamic_thread_lifecycle)
{
    constexpr int                                POOL_SIZE             = 8;
    constexpr int                                WAVE_COUNT            = 10;
    constexpr int                                THREADS_PER_WAVE      = 16;
    constexpr int                                ITERATIONS_PER_THREAD = 50;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::atomic_int total_ops {0};

    for (int wave = 0; wave < WAVE_COUNT; ++wave) {
        std::vector<std::jthread> threads;
        std::barrier              start_barrier {THREADS_PER_WAVE};

        for (int t = 0; t < THREADS_PER_WAVE; ++t) {
            threads.emplace_back([&]() {
                start_barrier.arrive_and_wait();
                for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
                    try {
                        auto res = pool.checkout();
                        total_ops++;
                    }
                    catch (const std::runtime_error&) {
                        // Expected under contention
                    }
                }
            });
        }

        threads.clear();
    }

    EXPECT_EQ(static_cast<size_t>(POOL_SIZE), pool.size());
    EXPECT_GT(total_ops.load(), 0);
}

/// @brief Test with thread pool pattern - persistent worker threads
/// Simulates a realistic thread pool scenario
TEST(stress_extreme_concurrency, persistent_worker_threads)
{
    constexpr int                                POOL_SIZE        = 16;
    constexpr int                                WORKER_COUNT     = 8;
    constexpr int                                TASKS_PER_WORKER = 200;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::atomic_int           tasks_completed {0};
    std::atomic_bool          shutdown {false};
    std::barrier              start_barrier {WORKER_COUNT};

    std::vector<std::jthread> workers;
    for (int w = 0; w < WORKER_COUNT; ++w) {
        workers.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            int local_tasks = 0;
            while (local_tasks < TASKS_PER_WORKER && !shutdown.load()) {
                try {
                    {
                        auto res = pool.checkout();
                        // Simulate work
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                        local_tasks++;
                    }
                    tasks_completed++;
                }
                catch (const std::runtime_error&) {
                    // Pool starved - wait a bit and retry
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
            }
        });
    }

    workers.clear();

    EXPECT_EQ(static_cast<size_t>(POOL_SIZE), pool.size());
    EXPECT_GT(tasks_completed.load(), 0);
}

// ============================================================================
// RESOURCE LIFECYCLE TESTS
// ============================================================================

/// @brief Test rapid creation and destruction of pools
/// Validates pool construction/destruction under stress
TEST(stress_lifecycle, rapid_pool_creation_destruction)
{
    for (int i = 0; i < 100; ++i) {
        siddiqsoft::arrp::resource_pool<std::string> pool {};
        pool.checkin(std::string("resource"));

        {
            auto res = pool.checkout();
            EXPECT_EQ("resource", *res);
        }

        EXPECT_EQ(1u, pool.size());
    }
}

/// @brief Test with complex resource types (nested structures)
/// Validates pool with non-trivial resource types
TEST(stress_lifecycle, complex_resource_types)
{
    struct ComplexResource
    {
        std::vector<std::string>     data;
        std::map<std::string, int>   metadata;
        std::unique_ptr<std::string> ptr;

        ComplexResource()
            : ptr(std::make_unique<std::string>("default"))
        {
        }
    };

    siddiqsoft::arrp::resource_pool<ComplexResource> pool {};

    for (int i = 0; i < 5; ++i) {
        ComplexResource res;
        res.data.push_back(std::format("item-{}", i));
        res.metadata["index"] = i;
        pool.checkin(std::move(res));
    }

    EXPECT_EQ(5u, pool.size());

    for (int i = 0; i < 5; ++i) {
        auto res = pool.checkout();
        EXPECT_FALSE((*res).data.empty());
        EXPECT_FALSE((*res).metadata.empty());
        EXPECT_NE(nullptr, (*res).ptr);
    }

    EXPECT_EQ(5u, pool.size());
}

/// @brief Test resource modification persistence across cycles
/// Validates that resource state is preserved
TEST(stress_lifecycle, resource_state_persistence)
{
    siddiqsoft::arrp::resource_pool<std::vector<int>> pool {};
    pool.checkin(std::vector<int> {1, 2, 3});

    for (int cycle = 0; cycle < 50; ++cycle) {
        {
            auto res = pool.checkout();
            (*res).push_back(cycle);
        }
    }

    {
        auto res = pool.checkout();
        EXPECT_EQ(53u, (*res).size()); // 3 initial + 50 cycles
    }
}

/// @brief Test with move-only resources under concurrent access
/// Validates move semantics with high concurrency
TEST(stress_lifecycle, move_only_concurrent)
{
    constexpr int                                                 POOL_SIZE    = 8;
    constexpr int                                                 THREAD_COUNT = 16;
    constexpr int                                                 ITERATIONS   = 100;

    siddiqsoft::arrp::resource_pool<std::unique_ptr<std::string>> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.checkin(std::make_unique<std::string>(std::format("resource-{}", i)));
    }

    std::atomic_int           total_ops {0};
    std::barrier              start_barrier {THREAD_COUNT};

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < ITERATIONS; ++i) {
                try {
                    auto res = pool.checkout();
                    EXPECT_NE(nullptr, *res);
                    total_ops++;
                }
                catch (const std::runtime_error&) {
                    // Expected
                }
            }
        });
    }

    threads.clear();

    EXPECT_EQ(static_cast<size_t>(POOL_SIZE), pool.size());
    EXPECT_GT(total_ops.load(), 0);
}

// ============================================================================
// MEMORY PRESSURE TESTS
// ============================================================================

/// @brief Test with large objects to stress memory allocation
/// Validates pool behavior with significant memory usage
TEST(stress_memory, large_object_pool)
{
    constexpr size_t                                   OBJECT_SIZE = 10 * 1024 * 1024; // 10MB
    constexpr int                                      POOL_SIZE   = 2;

    siddiqsoft::arrp::resource_pool<std::vector<char>> pool {};

    for (int i = 0; i < POOL_SIZE; ++i) {
        std::vector<char> large_vec(OBJECT_SIZE, 'x');
        pool.checkin(std::move(large_vec));
    }

    {
        auto res = pool.checkout();
        EXPECT_EQ(OBJECT_SIZE, (*res).size());
        (*res)[0] = 'y';
    }

    {
        auto res = pool.checkout();
        EXPECT_EQ('y', (*res)[0]); // Verify state persistence
    }

    EXPECT_EQ(2u, pool.size());
}

/// @brief Test rapid allocation/deallocation with varying sizes
/// Validates memory management under variable load
TEST(stress_memory, variable_size_objects)
{
    siddiqsoft::arrp::resource_pool<std::vector<int>> pool {};

    for (int cycle = 0; cycle < 100; ++cycle) {
        {
            auto res = pool.checkout();
            // Grow the vector
            for (int i = 0; i < 100; ++i) {
                (*res).push_back(i);
            }
        }

        {
            auto res = pool.checkout();
            // Shrink the vector
            (*res).clear();
        }
    }

    EXPECT_EQ(1u, pool.size());
}

/// @brief Test with many small objects
/// Validates pool efficiency with numerous small resources
TEST(stress_memory, many_small_objects)
{
    constexpr int                                POOL_SIZE = 256;

    siddiqsoft::arrp::resource_pool<SmallObject> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.checkin(SmallObject(i));
    }

    EXPECT_EQ(POOL_SIZE, pool.size());

    for (int i = 0; i < POOL_SIZE; ++i) {
        auto res = pool.checkout();
        EXPECT_GE((*res).value, 0);
    }

    EXPECT_EQ(POOL_SIZE, pool.size());
}

// ============================================================================
// CONTENTION TESTS
// ============================================================================

/// @brief Test extreme contention with minimal resources
/// Validates fairness and correctness under starvation
TEST(stress_contention, extreme_starvation)
{
    constexpr int                                POOL_SIZE    = 1;
    constexpr int                                THREAD_COUNT = 32;
    constexpr int                                ITERATIONS   = 100;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.checkin(std::string("single-resource"));

    std::atomic_int           successes {0};
    std::atomic_int           failures {0};
    std::barrier              start_barrier {THREAD_COUNT};

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < ITERATIONS; ++i) {
                try {
                    auto res = pool.checkout();
                    successes++;
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
                catch (const std::runtime_error&) {
                    failures++;
                }
            }
        });
    }

    threads.clear();

    EXPECT_EQ(1u, pool.size());
    EXPECT_GT(successes.load(), 0);
    EXPECT_GT(failures.load(), 0);
    EXPECT_EQ(THREAD_COUNT * ITERATIONS, successes.load() + failures.load());
}

/// @brief Test with gradually increasing contention
/// Validates pool behavior as contention increases
TEST(stress_contention, gradual_contention_increase)
{
    constexpr int                                POOL_SIZE = 8;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    for (int thread_count = 1; thread_count <= 32; thread_count *= 2) {
        std::atomic_int           ops {0};
        std::barrier              start_barrier {thread_count};

        std::vector<std::jthread> threads;
        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&]() {
                start_barrier.arrive_and_wait();
                for (int i = 0; i < 100; ++i) {
                    try {
                        auto res = pool.checkout();
                        ops++;
                    }
                    catch (const std::runtime_error&) {
                        // Expected
                    }
                }
            });
        }

        threads.clear();

        EXPECT_EQ(static_cast<size_t>(POOL_SIZE), pool.size());
        EXPECT_GT(ops.load(), 0);
    }
}

/// @brief Test fairness - ensure all threads get fair access
/// Validates that no thread is starved indefinitely
TEST(stress_contention, fairness_distribution)
{
    constexpr int                                POOL_SIZE    = 4;
    constexpr int                                THREAD_COUNT = 8;
    constexpr int                                ITERATIONS   = 100;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::vector<std::atomic_int> thread_successes(THREAD_COUNT);
    std::barrier                 start_barrier {THREAD_COUNT};

    std::vector<std::jthread>    threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&, t]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < ITERATIONS; ++i) {
                try {
                    auto res = pool.checkout();
                    thread_successes[t]++;
                }
                catch (const std::runtime_error&) {
                    // Expected
                }
            }
        });
    }

    threads.clear();

    // Check that all threads got at least some resources
    for (int t = 0; t < THREAD_COUNT; ++t) {
        EXPECT_GT(thread_successes[t].load(), 0) << "Thread " << t << " got no resources";
    }

    // Check that distribution is relatively fair (within 50% of average)
    int total_successes = 0;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        total_successes += thread_successes[t].load();
    }
    int avg_per_thread = total_successes / THREAD_COUNT;
    int min_threshold  = avg_per_thread / 2;

    for (int t = 0; t < THREAD_COUNT; ++t) {
        EXPECT_GE(thread_successes[t].load(), min_threshold) << "Thread " << t << " got significantly fewer resources than average";
    }
}

// ============================================================================
// CHAOS TESTS - Random operations and timing variations
// ============================================================================

/// @brief Test with random operation timing
/// Validates pool under unpredictable timing patterns
TEST(stress_chaos, random_operation_timing)
{
    std::random_device                           rd;
    std::mt19937                                 gen(rd());
    std::uniform_int_distribution<>              delay_dist(0, 1000);

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < 8; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::atomic_int           ops {0};
    std::barrier              start_barrier {8};

    std::vector<std::jthread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            std::mt19937 local_gen(rd() + t);
            start_barrier.arrive_and_wait();
            for (int i = 0; i < 100; ++i) {
                try {
                    auto res = pool.checkout();
                    ops++;
                    std::this_thread::sleep_for(std::chrono::microseconds(delay_dist(local_gen)));
                }
                catch (const std::runtime_error&) {
                    // Expected
                }
            }
        });
    }

    threads.clear();

    EXPECT_EQ(8u, pool.size());
    EXPECT_GT(ops.load(), 0);
}

/// @brief Test with random exception injection
/// Validates pool recovery from exceptions
TEST(stress_chaos, random_exception_injection)
{
    std::random_device                           rd;
    std::mt19937                                 gen(rd());
    std::uniform_int_distribution<>              exception_dist(0, 10);

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.checkin(std::string("resource"));

    std::atomic_int exceptions_caught {0};
    std::atomic_int successful_ops {0};

    for (int i = 0; i < 1000; ++i) {
        try {
            auto res = pool.checkout();
            if (exception_dist(gen) < 3) {
                throw std::runtime_error("Injected exception");
            }
            successful_ops++;
        }
        catch (const std::runtime_error&) {
            exceptions_caught++;
        }
    }

    // Resource should still be in pool
    EXPECT_EQ(1u, pool.size());
    EXPECT_GT(successful_ops.load(), 0);
    EXPECT_GT(exceptions_caught.load(), 0);
}

/// @brief Test with random clear operations
/// Validates pool behavior with concurrent clears
TEST(stress_chaos, random_clear_operations)
{
    std::random_device                           rd;
    std::mt19937                                 gen(rd());
    std::uniform_int_distribution<>              clear_dist(0, 100);

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < 10; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::atomic_int  clears {0};
    std::atomic_int  checkouts {0};
    std::atomic_bool stop {false};

    auto             worker  = std::jthread([&]() {
        for (int i = 0; i < 500 && !stop.load(); ++i) {
            try {
                auto res = pool.checkout();
                checkouts++;
            }
            catch (const std::runtime_error&) {
                // Expected
            }
        }
    });

    auto             clearer = std::jthread([&]() {
        for (int i = 0; i < 50 && !stop.load(); ++i) {
            if (clear_dist(gen) < 30) {
                pool.clear();
                clears++;
                // Repopulate
                for (int j = 0; j < 5; ++j) {
                    pool.checkin(std::format("resource-{}", j));
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(2));
    stop = true;

    worker.join();
    clearer.join();

    EXPECT_GT(clears.load(), 0);
    EXPECT_GT(checkouts.load(), 0);
}

// ============================================================================
// CAPACITY TESTS - Boundary conditions and limits
// ============================================================================

/// @brief Test with maximum capacity
/// Validates pool at capacity limits
TEST(stress_capacity, maximum_capacity)
{
    constexpr uint8_t MAX_CAPACITY = siddiqsoft::arrp::resource_pool_limits::MaxCapacity;
    siddiqsoft::arrp::resource_pool<std::string, siddiqsoft::arrp::scoped_resource<std::string>, MAX_CAPACITY> pool {};

    // Fill to capacity
    for (int i = 0; i < MAX_CAPACITY; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    EXPECT_EQ(MAX_CAPACITY, pool.size());

    // Checkout all
    std::vector<siddiqsoft::arrp::scoped_resource<std::string>> resources;
    for (int i = 0; i < MAX_CAPACITY; ++i) {
        resources.push_back(pool.checkout());
    }

    EXPECT_EQ(0u, pool.size());

    // Should throw when trying to checkout beyond capacity
    EXPECT_THROW({ auto res = pool.checkout(); }, std::runtime_error);

    // Return all
    resources.clear();
    EXPECT_EQ(MAX_CAPACITY, pool.size());
}

/// @brief Test capacity enforcement under concurrent access
/// Validates that capacity limits are respected
TEST(stress_capacity, capacity_enforcement_concurrent)
{
    constexpr uint8_t                                                                                      CAPACITY = 16;
    siddiqsoft::arrp::resource_pool<std::string, siddiqsoft::arrp::scoped_resource<std::string>, CAPACITY> pool {};

    for (int i = 0; i < CAPACITY; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::atomic_int           successes {0};
    std::atomic_int           failures {0};
    std::barrier              start_barrier {8};

    std::vector<std::jthread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            start_barrier.arrive_and_wait();
            // std::this_thread::sleep_for(std::chrono::milliseconds(t * 100));
            for (int i = 0; i < 1000; ++i) {
                try {
                    auto res = pool.checkout();
                    successes++;
                    std::this_thread::sleep_for(std::chrono::microseconds(t*500));
                }
                catch (const std::runtime_error&) {
                    failures++;
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    threads.clear();
    
    std::cerr << std::format("{}\n", pool.to_json().dump());

    EXPECT_EQ(CAPACITY, pool.size());
    EXPECT_GT(successes.load(), 0);
    EXPECT_GT(failures.load(), 0);
}

// ============================================================================
// RECOVERY TESTS - Behavior after errors and exceptions
// ============================================================================

/// @brief Test recovery from repeated exceptions
/// Validates pool remains functional after exceptions
TEST(stress_recovery, repeated_exceptions)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.checkin(std::string("resource"));

    for (int i = 0; i < 100; ++i) {
        try {
            auto res = pool.checkout();
            if (i % 2 == 0) {
                throw std::runtime_error("Test exception");
            }
        }
        catch (const std::runtime_error&) {
            // Expected
        }
    }

    // Pool should still be functional
    EXPECT_EQ(1u, pool.size());
    auto res = pool.checkout();
    EXPECT_EQ("resource", *res);
}

/// @brief Test recovery from factory callback failures
/// Validates pool behavior when factory throws
TEST(stress_recovery, factory_callback_failures)
{
    std::atomic_int                              factory_calls {0};
    std::atomic_int                              factory_failures {0};

    siddiqsoft::arrp::resource_pool<std::string> pool {[&](auto& p) -> siddiqsoft::arrp::scoped_resource<std::string> {
        factory_calls++;
        if (factory_calls.load() % 3 == 0) {
            factory_failures++;
            throw std::runtime_error("Factory failure");
        }
        return siddiqsoft::arrp::scoped_resource<std::string>(std::format("created-{}", factory_calls.load()),
                                                              [&p](std::string&& res) { p.checkin(std::move(res)); });
    }};

    std::atomic_int                              successes {0};
    std::atomic_int                              failures {0};

    for (int i = 0; i < 100; ++i) {
        try {
            auto res = pool.checkout();
            successes++;
        }
        catch (const std::runtime_error&) {
            failures++;
        }
    }

    EXPECT_GT(successes.load(), 0);
    EXPECT_GT(failures.load(), 0);
    EXPECT_EQ(factory_failures.load(), failures.load());
}

/// @brief Test recovery from clear operations
/// Validates pool can be repopulated after clear
TEST(stress_recovery, clear_and_repopulate)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int cycle = 0; cycle < 50; ++cycle) {
        // Populate
        for (int i = 0; i < 10; ++i) {
            pool.checkin(std::format("resource-{}-{}", cycle, i));
        }

        EXPECT_EQ(10u, pool.size());

        // Use some resources
        {
            auto res = pool.checkout();
            EXPECT_FALSE((*res).empty());
        }

        // Clear
        pool.clear();
        EXPECT_EQ(0u, pool.size());
    }
}

// ============================================================================
// ADVANCED CONCURRENT PATTERNS
// ============================================================================

/// @brief Test producer-consumer pattern
/// Validates pool in realistic producer-consumer scenario
TEST(stress_patterns, producer_consumer)
{
    constexpr int                                POOL_SIZE          = 8;
    constexpr int                                PRODUCER_COUNT     = 4;
    constexpr int                                CONSUMER_COUNT     = 4;
    constexpr int                                ITEMS_PER_PRODUCER = 100;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::atomic_int           produced {0};
    std::atomic_int           consumed {0};
    std::barrier              start_barrier {PRODUCER_COUNT + CONSUMER_COUNT};

    std::vector<std::jthread> threads;

    // Producers
    for (int p = 0; p < PRODUCER_COUNT; ++p) {
        threads.emplace_back([&, p]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                pool.checkin(std::format("produced-{}-{}", p, i));
                produced++;
            }
        });
    }

    // Consumers
    for (int c = 0; c < CONSUMER_COUNT; ++c) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                try {
                    auto res = pool.checkout();
                    consumed++;
                }
                catch (const std::runtime_error&) {
                    // Expected if pool is empty
                }
            }
        });
    }

    threads.clear();

    EXPECT_GT(produced.load(), 0);
    EXPECT_GT(consumed.load(), 0);
}

/// @brief Test with mixed operation types
/// Validates pool with diverse operation patterns
TEST(stress_patterns, mixed_operations)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < 10; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::atomic_int           checkouts {0};
    std::atomic_int           checkins {0};
    std::atomic_int           clears {0};
    std::barrier              start_barrier {4};

    std::vector<std::jthread> threads;

    // Checkout thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            try {
                auto res = pool.checkout();
                checkouts++;
            }
            catch (const std::runtime_error&) {
            }
        }
    });

    // Checkin thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 50; ++i) {
            pool.checkin(std::format("new-resource-{}", i));
            checkins++;
        }
    });

    // Clear thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 10; ++i) {
            pool.clear();
            clears++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // JSON serialization thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 50; ++i) {
            auto json = pool.to_json();
            EXPECT_TRUE(json.contains("counters"));
        }
    });

    threads.clear();

    EXPECT_GT(checkouts.load(), 0);
    EXPECT_GT(checkins.load(), 0);
    EXPECT_GT(clears.load(), 0);
}

// ============================================================================
// STRESS TEST COMBINATIONS
// ============================================================================

/// @brief Ultimate stress test - combines multiple stress patterns
/// Comprehensive validation of pool robustness
TEST(stress_ultimate, comprehensive_stress)
{
    constexpr int                                POOL_SIZE    = 16;
    constexpr int                                THREAD_COUNT = 32;
    constexpr int                                DURATION_MS  = 5000;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.checkin(std::format("resource-{}", i));
    }

    std::atomic_bool          stop {false};
    std::atomic_int           total_ops {0};
    std::atomic_int           total_exceptions {0};
    std::atomic_int           total_clears {0};
    std::barrier              start_barrier {THREAD_COUNT};

    auto                      start_time = std::chrono::steady_clock::now();

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&, t]() {
            start_barrier.arrive_and_wait();
            std::random_device              rd;
            std::mt19937                    gen(rd() + t);
            std::uniform_int_distribution<> op_dist(0, 100);

            while (!stop.load()) {
                int op = op_dist(gen);

                if (op < 70) {
                    // Checkout operation
                    try {
                        auto res = pool.checkout();
                        total_ops++;
                        std::this_thread::sleep_for(std::chrono::microseconds(op_dist(gen) % 100));
                    }
                    catch (const std::runtime_error&) {
                        total_exceptions++;
                    }
                }
                else if (op < 85) {
                    // Checkin operation
                    pool.checkin(std::format("new-resource-{}-{}", t, op_dist(gen)));
                    total_ops++;
                }
                else if (op < 95) {
                    // Clear operation
                    pool.clear();
                    total_clears++;
                    // Repopulate
                    for (int i = 0; i < 5; ++i) {
                        pool.checkin(std::format("repopulated-{}", i));
                    }
                }
                else {
                    // JSON serialization
                    auto json = pool.to_json();
                    EXPECT_TRUE(json.contains("counters"));
                }
            }
        });
    }

    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(DURATION_MS));
    stop = true;

    threads.clear();

    auto elapsed = std::chrono::steady_clock::now() - start_time;
    std::cerr << std::format("Ultimate stress test completed in {}ms\n",
                             std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    std::cerr << std::format("Total operations: {}\n", total_ops.load());
    std::cerr << std::format("Total exceptions: {}\n", total_exceptions.load());
    std::cerr << std::format("Total clears: {}\n", total_clears.load());
    std::cerr << std::format("Final pool size: {}\n", pool.size());
    std::cerr << std::format("Pool state: {}\n", pool.to_json().dump(2));

    EXPECT_GT(total_ops.load(), 0);
    EXPECT_GE(pool.size(), 0u);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
