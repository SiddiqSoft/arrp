/*
    Counter Balance Tests for resource_pool

    This file contains comprehensive tests to verify that the m_resources_checkedout
    counter is properly balanced between try_borrow() and return_to_pool() operations.

    The counter should:
    1. Increment when a resource is borrowed
    2. Decrement when a resource is returned (valid or invalid)
    3. Remain balanced after all operations complete
    4. Handle concurrent access correctly
    5. Handle exceptions and error conditions properly

    BSD 3-Clause License

    Copyright (c) 2026 Abdulkareem Siddiq
    All rights reserved.
 */

#include "gtest/gtest.h"

#include <iostream>
#include <format>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <barrier>
#include <chrono>
#include <random>
#include <memory>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/private/resource_pool.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)


// HELPER FUNCTION TO EXTRACT CHECKEDOUT COUNT FROM JSON


/// @brief Helper function to get the checkedout count from pool JSON
/// @param pool The resource pool
/// @return The number of checked-out resources, or -1 if unable to determine
template <typename T>
int64_t get_borrow_count(siddiqsoft::arrp::resource_pool<T>& pool)
{
    auto doc = pool.to_json();
    return doc.value("borrows", -1);
}

template <typename T>
int64_t get_loan_count(siddiqsoft::arrp::resource_pool<T>& pool)
{
    auto doc = pool.to_json();
    return doc.value("loans", -1);
}


// BASIC COUNTER BALANCE TESTS


/// @brief Test counter increments on borrow and decrements on return
TEST(counter_balance, basic_borrow_return)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed(std::string("resource-1"));

    // Initial state: 0 checked out
    EXPECT_EQ(0, get_borrow_count(pool));

    {
        // Borrow: counter should increment
        auto res = pool.try_borrow();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        // Still in scope: counter should remain 1
        EXPECT_EQ(1, get_borrow_count(pool));
    }

    // After scope: counter should decrement back to 0
    EXPECT_EQ(0, get_loan_count(pool));
}

/// @brief Test counter balance with multiple sequential borrows
TEST(counter_balance, multiple_sequential_borrows)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 5; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    // Borrow and return sequentially
    for (int i = 0; i < 5; ++i) {
        {
            auto res = pool.try_borrow();
            EXPECT_TRUE(res.has_value());
            EXPECT_EQ(1, get_loan_count(pool));
        }
        EXPECT_EQ(0, get_loan_count(pool));
    }

    // Final state: all returned
    EXPECT_EQ(0, get_loan_count(pool));
    EXPECT_EQ(5, pool.size());
}

/// @brief Test counter balance with multiple concurrent borrows
TEST(counter_balance, multiple_concurrent_borrows)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 5; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    {
        auto res1 = pool.try_borrow();
        EXPECT_TRUE(res1.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        {
            auto res2 = pool.try_borrow();
            EXPECT_TRUE(res2.has_value());
            EXPECT_EQ(2, get_borrow_count(pool));

            {
                auto res3 = pool.try_borrow();
                EXPECT_TRUE(res3.has_value());
                EXPECT_EQ(3, get_borrow_count(pool));
            }

            EXPECT_EQ(2, get_loan_count(pool));
        }

        EXPECT_EQ(1, get_loan_count(pool));
    }

    EXPECT_EQ(0, get_loan_count(pool));
}

/// @brief Test counter balance with invalidated resources
TEST(counter_balance, invalidated_resources)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    pool.seed(std::string("resource-1"));
    pool.seed(std::string("resource-2"));

    EXPECT_EQ(0, get_borrow_count(pool));

    {
        auto res1 = pool.try_borrow();
        EXPECT_TRUE(res1.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        auto res2 = pool.try_borrow();
        EXPECT_TRUE(res2.has_value());
        EXPECT_EQ(2, get_borrow_count(pool));

        // Invalidate one resource
        res1.invalidate();
        EXPECT_EQ(2, get_borrow_count(pool)); // Still checked out
    }

    // After scope: both should be decremented
    EXPECT_EQ(0, get_loan_count(pool));

    // But only one should be in the pool (the other was invalidated)
    EXPECT_EQ(1, pool.size());
}

TEST(counter_balance, loan_size_matches_checked_out_resources)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed(std::string("resource-1"));
    pool.seed(std::string("resource-2"));

    EXPECT_EQ(0, get_loan_count(pool));

    auto first = pool.try_borrow();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(1, get_loan_count(pool));

    {
        auto second = pool.try_borrow();
        ASSERT_TRUE(second.has_value());
        EXPECT_EQ(2, get_loan_count(pool));
    }

    EXPECT_EQ(1, get_loan_count(pool));

    first.invalidate();
    EXPECT_EQ(1, get_loan_count(pool));

    auto third = pool.try_borrow();
    EXPECT_TRUE(third.has_value());
    EXPECT_EQ(2, get_loan_count(pool));
}

class custom_mr
{
public:
    std::string v;

    custom_mr() = default;

    explicit operator std::string&() { return v; }
    explicit operator const char*() { return v.c_str(); }

    custom_mr(const std::string& s)
        : v(s)
    {
    }
    custom_mr(std::string&& s)
        : v(std::move(s))
    {
    }
    custom_mr(custom_mr&& src) noexcept
        : v(std::move(src.v))
    {
    }
    custom_mr& operator=(custom_mr&& src) noexcept
    {
        if (this != &src) {
            v = std::move(src.v);
        }
        return *this;
    }
    auto operator=(const std::string& s) -> custom_mr&
    {
        v = s;
        return *this;
    }
    ~custom_mr() { std::print(std::cerr, "{} - destroyed: {}\n", __func__, v); }
    bool                 operator==(const std::string& src) const { return v == src; }
    bool                 operator==(const char* src) const { return v == src; }
    std::strong_ordering operator<=>(const std::string& src) const { return v <=> src; }
    std::strong_ordering operator<=>(const char* src) const { return v <=> src; }
};

/// @brief Test counter balance with moved resources
TEST(counter_balance, moved_resources)
{
    siddiqsoft::arrp::resource_pool<custom_mr> pool {};

    pool.seed(custom_mr {"resource-1"});
    pool.seed(custom_mr {"resource-2"});

    EXPECT_EQ(0, get_borrow_count(pool));
    EXPECT_EQ(2, pool.size());

    {
        auto res1 = pool.try_borrow();
        EXPECT_TRUE(res1.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        auto res2 = pool.try_borrow();
        EXPECT_TRUE(res2.has_value());
        EXPECT_EQ(2, get_borrow_count(pool));

        // Move res1 to res2
        res2 = std::move(res1);
        EXPECT_EQ(2, get_borrow_count(pool)); // Still 2 checked out
    }

    std::print(std::cerr, "Stats: {}\n", pool.to_json().dump());

    // After scope: both should be decremented
    // The custom resource cleans up properly!
    EXPECT_EQ(0, get_loan_count(pool));

    // Both resources should be back in the pool!
    EXPECT_EQ(2, pool.size());
}


// EXCEPTION HANDLING COUNTER BALANCE TESTS


/// @brief Test counter balance when exception occurs during borrow
TEST(counter_balance, exception_during_borrow)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {4};

    EXPECT_EQ(0, get_borrow_count(pool));

    // Try to borrow when factory throws
    auto res = pool.try_borrow();
    EXPECT_FALSE(res.has_value());

    // Counter should still be balanced
    EXPECT_EQ(0, get_borrow_count(pool));
}

/// @brief Test counter balance when exception occurs in user code
TEST(counter_balance, exception_in_user_code)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed(std::string("resource-1"));

    EXPECT_EQ(0, get_borrow_count(pool));

    try {
        auto res = pool.try_borrow();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        throw std::runtime_error("User error");
    }
    catch (const std::exception&) {
        // Exception caught
    }

    // Counter should be balanced after exception
    EXPECT_EQ(0, get_loan_count(pool));
}

/// @brief Test counter balance with nested exception handling
TEST(counter_balance, nested_exception_handling)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 3; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    try {
        auto res1 = pool.try_borrow();
        EXPECT_TRUE(res1.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        try {
            auto res2 = pool.try_borrow();
            EXPECT_TRUE(res2.has_value());
            EXPECT_EQ(2, get_borrow_count(pool));

            throw std::runtime_error("Inner error");
        }
        catch (const std::exception&) {
            EXPECT_EQ(1, get_loan_count(pool));
        }

        throw std::runtime_error("Outer error");
    }
    catch (const std::exception&) {
        // Exception caught
    }

    std::print(std::cerr, "Stats: {}\n", pool.to_json().dump());

    // Counter should be balanced
    EXPECT_EQ(0, get_loan_count(pool));
}


// CONCURRENT COUNTER BALANCE TESTS


/// @brief Test counter balance with concurrent borrows and returns
TEST(counter_balance, concurrent_borrows_and_returns)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 20; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    std::atomic_int           total_borrows {0};
    std::atomic_int           total_returns {0};
    std::barrier              start_barrier {4};

    std::vector<std::jthread> threads;

    // Thread 1: Borrow and return
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 50; ++i) {
            auto res = pool.try_borrow();
            if (res.has_value()) {
                total_borrows++;
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                total_returns++;
            }
        }
    });

    // Thread 2: Borrow and return
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 50; ++i) {
            auto res = pool.try_borrow();
            if (res.has_value()) {
                total_borrows++;
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                total_returns++;
            }
        }
    });

    // Thread 3: Borrow and return
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 50; ++i) {
            auto res = pool.try_borrow();
            if (res.has_value()) {
                total_borrows++;
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                total_returns++;
            }
        }
    });

    // Thread 4: Monitor counter
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            int64_t checkedout = get_borrow_count(pool);
            // Counter should never be negative
            EXPECT_GE(checkedout, 0);
            // Counter should not exceed total borrows
            EXPECT_LE(checkedout, total_borrows.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(19));
        }
    });

    threads.clear();

    // Final state: all returned
    EXPECT_EQ(0, get_loan_count(pool));
    EXPECT_EQ(total_borrows.load(), total_returns.load());
}

/// @brief Test counter balance with concurrent invalidations
TEST(counter_balance, concurrent_invalidations)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 30; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    std::atomic_int           invalidated {0};
    std::atomic_int           returned {0};
    std::barrier              start_barrier {4};

    std::vector<std::jthread> threads;

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < 50; ++i) {
                auto res = pool.try_borrow();
                if (res.has_value()) {
                    if (i % 3 == 0) {
                        res.invalidate();
                        invalidated++;
                    }
                    else {
                        returned++;
                    }
                }
            }
        });
    }

    threads.clear();

    // Final state: counter should be balanced
    EXPECT_EQ(0, get_loan_count(pool));

    // Pool should contain only non-invalidated resources
    int64_t expected_size = 30 - invalidated.load();
    EXPECT_EQ(expected_size, static_cast<int64_t>(pool.size()));
}

/// @brief Test counter balance with concurrent adds and borrows
TEST(counter_balance, concurrent_adds_and_borrows)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 10; ++i) {
        pool.seed(std::format("initial-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    std::atomic_int           adds {0};
    std::atomic_int           borrows {0};
    std::barrier              start_barrier {3};

    std::vector<std::jthread> threads;

    // Thread 1: Add resources
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            pool.seed(std::format("added-{}", i));
            adds++;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Thread 2: Borrow resources
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            auto res = pool.try_borrow();
            if (res.has_value()) {
                borrows++;
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }
    });

    // Thread 3: Monitor counter
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 200; ++i) {
            int64_t checkedout = get_borrow_count(pool);
            EXPECT_GE(checkedout, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(19));
        }
    });

    threads.clear();

    // Final state: counter should be balanced
    EXPECT_EQ(0, get_loan_count(pool));
}

/// @brief Test counter balance with concurrent clears
TEST(counter_balance, concurrent_clears)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 20; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    std::atomic_int           clears {0};
    std::atomic_int           borrows {0};
    std::barrier              start_barrier {3};

    std::vector<std::jthread> threads;

    // Thread 1: Borrow resources
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            auto res = pool.try_borrow();
            if (res.has_value()) {
                borrows++;
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }
    });

    // Thread 2: Clear and repopulate
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 10; ++i) {
            pool.clear();
            clears++;
            for (int j = 0; j < 10; ++j) {
                pool.seed(std::format("repopulated-{}-{}", i, j));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Thread 3: Monitor counter
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 200; ++i) {
            int64_t checkedout = get_borrow_count(pool);
            EXPECT_GE(checkedout, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(19));
        }
    });

    threads.clear();

    // Final state: counter should be balanced
    EXPECT_EQ(0, get_loan_count(pool));
}


// AUTOGROW POLICY COUNTER BALANCE TESTS


// STRESS TESTS FOR COUNTER BALANCE


/// @brief Stress test with high concurrency and rapid operations
TEST(counter_balance, high_concurrency_stress)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 50; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    std::atomic_int           operations {0};
    std::barrier              start_barrier {8};

    std::vector<std::jthread> threads;

    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < 1000; ++i) {
                auto res = pool.try_borrow();
                if (res.has_value()) {
                    operations++;
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::microseconds(19));
                }
            }
        });
    }

    threads.clear();

    std::print(std::cerr, "Stats: {}\n", pool.to_json().dump());

    // Final state: counter should be balanced
    EXPECT_EQ(0, get_loan_count(pool));
    EXPECT_GT(operations.load(), 0);
}

/// @brief Stress test with mixed operations
TEST(counter_balance, mixed_operations_stress)
{
    constexpr int                                EXPECTED_THREADS = 5;
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 30; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool)) << "Initial borrow count should be 0.";

    std::atomic_int borrows {0};
    std::atomic_int adds {0};
    std::atomic_int invalidates {0};
    std::barrier    start_barrier {EXPECTED_THREADS};
    std::atomic_int sync_threads_ready {0};

    auto            sync_threads_point = [&] {
#if defined(_WIN64) || defined(_WIN32)
        sync_threads_ready++;
        while (sync_threads_ready.load() < EXPECTED_THREADS) {
            std::this_thread::yield();
        }
#else
        start_barrier.arrive_and_wait();
#endif
        std::println(std::cerr,
                     "   concurrent_json_deadlock_detection - All threads ready to continue..{}/{}",
                     sync_threads_ready.load(),
                     EXPECTED_THREADS);
    };

    std::vector<std::jthread> threads;

    // Add thread
    threads.emplace_back([&]() {
        sync_threads_point();
        for (int i = 0; i < 100; ++i) {
            pool.seed(std::format("new-{}", i));
            adds++;
        }
    });

    // Borrow thread
    threads.emplace_back([&]() {
        sync_threads_point();
        for (int i = 0; i < 500; ++i) {
            auto res = pool.try_borrow();
            if (res.has_value()) {
                borrows++;
            }
        }
    });

    // Invalidate thread
    threads.emplace_back([&]() {
        sync_threads_point();
        for (int i = 0; i < 100; ++i) {
            auto res = pool.try_borrow();
            if (res.has_value()) {
                res.invalidate();
                invalidates++;
            }
        }
    });

    // Monitor thread 1
    threads.emplace_back([&]() {
        sync_threads_point();
        for (int i = 0; i < 300; ++i) {
            int64_t checkedout = get_borrow_count(pool);
            EXPECT_GE(checkedout, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(19));
        }
    });

    // Monitor thread 2
    threads.emplace_back([&]() {
        sync_threads_point();
        for (int i = 0; i < 300; ++i) {
            auto size = pool.size();
            EXPECT_GE(size, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(19));
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(5)); // Allow threads to run for a while
    threads.clear();

    // Final state: counter should be balanced
    EXPECT_EQ(0, get_loan_count(pool)) << "Final loan count should be 0 after all threads complete.";
    EXPECT_GT(borrows.load(), 0) << "There should have been some successful borrows.";
    EXPECT_GT(adds.load(), 0) << "There should have been some successful adds.";
    EXPECT_GT(invalidates.load(), 0) << "There should have been some successful invalidations.";
}


// EDGE CASE COUNTER BALANCE TESTS


/// @brief Test counter balance with single resource
TEST(counter_balance, single_resource)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed(std::string("only-resource"));

    EXPECT_EQ(0, get_borrow_count(pool));

    {
        auto res = pool.try_borrow();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(1, get_loan_count(pool));
    }

    EXPECT_EQ(0, get_loan_count(pool));
}

/// @brief Test counter balance with maximum capacity
TEST(counter_balance, maximum_capacity)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {siddiqsoft::arrp::resource_pool_limits::MaxCapacity};

    for (int i = 0; i < 10; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    // Borrow all
    std::vector<siddiqsoft::arrp::resource_guard<std::string>> resources;
    for (int i = 0; i < 10; ++i) {
        auto res = pool.try_borrow();
        EXPECT_TRUE(res.has_value());
        resources.push_back(std::move(res));
    }

    EXPECT_EQ(10, get_borrow_count(pool));

    // Return all
    resources.clear();

    EXPECT_EQ(0, get_loan_count(pool));
}

/// @brief Test counter balance with rapid create/destroy cycles
TEST(counter_balance, rapid_create_destroy_cycles)
{
    for (int cycle = 0; cycle < 10; ++cycle) {
        siddiqsoft::arrp::resource_pool<std::string> pool {};

        for (int i = 0; i < 5; ++i) {
            pool.seed(std::format("resource-{}", i));
        }

        EXPECT_EQ(0, get_borrow_count(pool));

        {
            auto res = pool.try_borrow();
            EXPECT_TRUE(res.has_value());
            EXPECT_EQ(1, get_borrow_count(pool));
        }

        EXPECT_EQ(0, get_loan_count(pool));
    }
}


// COUNTER CONSISTENCY TESTS


/// @brief Test that counter is consistent with pool size
TEST(counter_balance, counter_size_consistency)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 10; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));
    EXPECT_EQ(10, pool.size());

    // Borrow 5
    {
        auto p1 = pool.try_borrow();
        auto p2 = pool.try_borrow();
        {
            { // borrow 3 more.. and release them first..
                auto p3 = pool.try_borrow();
                auto p4 = pool.try_borrow();
                auto p5 = pool.try_borrow();

                EXPECT_EQ(5, get_borrow_count(pool));
                EXPECT_EQ(5, pool.size());

                std::println(std::cerr,
                             "....before returning 3... resources: {}. stats: {}",
                             0, // resources.size(),
                             pool.to_json().dump());
            }
            EXPECT_EQ(2, get_loan_count(pool));
            EXPECT_EQ(8, pool.size());
            std::println(std::cerr,
                         ".....after returning 3...resources:{}. stats: {}",
                         0, // resources.size(),
                         pool.to_json().dump());
        }
    } // release all five..
    std::println(std::cerr,
                 ".....after returning all borrowed...resources:{}. stats: {}",
                 0, // resources.size(),
                 pool.to_json().dump());

    EXPECT_EQ(0, get_loan_count(pool));
    EXPECT_EQ(10, pool.size());
}

TEST(counter_balance, counter_size_consistency_2)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 10; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));
    EXPECT_EQ(10, pool.size());

    // Borrow 5
    {
        std::vector<siddiqsoft::arrp::resource_guard<std::string>> holdResources;

        for (int i = 0; i < 5; i++) {
            holdResources.emplace_back(pool.try_borrow());
        }

        EXPECT_EQ(5, get_borrow_count(pool));
        EXPECT_EQ(5, pool.size());

        std::println(std::cerr,
                     "....before returning 3... resources: {}. stats: {}",
                     0, // resources.size(),
                     pool.to_json().dump());
        // Release three resources only...
        auto _ = holdResources.erase(holdResources.begin(), holdResources.begin() + 3);
        // This is important for std::vector!
        holdResources.shrink_to_fit();

        EXPECT_EQ(2, get_loan_count(pool));
        EXPECT_EQ(8, pool.size());
        std::println(std::cerr, ".....after returning 3...resources:{}. stats: {}", holdResources.size(), pool.to_json().dump());
    } // release all five..
    std::println(std::cerr, ".....after returning all borrowed... stats: {}", pool.to_json().dump());

    EXPECT_EQ(0, get_loan_count(pool));
    EXPECT_EQ(10, pool.size());
}

/// @brief Test counter consistency across JSON serialization
TEST(counter_balance, counter_json_consistency)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 5; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    {
        auto res1 = pool.try_borrow();
        auto res2 = pool.try_borrow();

        EXPECT_TRUE(res1.has_value());
        EXPECT_TRUE(res2.has_value());

        int64_t checkedout = get_borrow_count(pool);
        EXPECT_EQ(2, checkedout);

        auto j = pool.to_json();
        EXPECT_TRUE(j.is_object());

        EXPECT_EQ(3, j["size"].get<size_t>());
        EXPECT_EQ(5, j["seeds"].get<uint8_t>());
    }

    EXPECT_EQ(0, get_loan_count(pool));
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
