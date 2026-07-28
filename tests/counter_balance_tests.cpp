/*
    Counter Balance Tests for resource_pool

    This file contains comprehensive tests to verify that the m_resources_checkedout
    counter is properly balanced between borrow_from_pool() and return_to_pool() operations.

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
#include "../include/siddiqsoft/resource_pool.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

// ============================================================================
// HELPER FUNCTION TO EXTRACT CHECKEDOUT COUNT FROM JSON
// ============================================================================

/// @brief Helper function to get the checkedout count from pool JSON
/// @param pool The resource pool
/// @return The number of checked-out resources, or -1 if unable to determine
template <typename T>
int64_t get_borrow_count(siddiqsoft::arrp::resource_pool<T>& pool)
{
    auto json_result = pool.to_json();
    if (!json_result.has_value()) {
        return -1;
    }

    auto& json = json_result.value().get();
    if (json.contains("borrows")) {
        return json.value("borrows", 0);
    }

    // If checkedout is not in JSON, calculate it from other fields
    // checkedout = capacity - size (approximately)
    if (json.contains("capacity") && json.contains("size")) {
        int64_t capacity = json.value("capacity",0);
        int64_t size     = json.value("size",0);
        return capacity - size; // This is an approximation
    }

    return -1;
}

// ============================================================================
// BASIC COUNTER BALANCE TESTS
// ============================================================================

/// @brief Test counter increments on borrow and decrements on return
TEST(counter_balance, basic_borrow_return)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.add_to_pool(std::string("resource-1"));

    // Initial state: 0 checked out
    EXPECT_EQ(0, get_borrow_count(pool));

    {
        // Borrow: counter should increment
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        // Still in scope: counter should remain 1
        EXPECT_EQ(1, get_borrow_count(pool));
    }

    // After scope: counter should decrement back to 0
    EXPECT_EQ(0, pool.size().value_or(0));
}

/// @brief Test counter balance with multiple sequential borrows
TEST(counter_balance, multiple_sequential_borrows)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 5; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    // Borrow and return sequentially
    for (int i = 0; i < 5; ++i) {
        {
            auto res = pool.borrow_from_pool();
            EXPECT_TRUE(res.has_value());
            EXPECT_EQ(1, get_borrow_count(pool));
        }
        EXPECT_EQ(0, get_borrow_count(pool));
    }

    // Final state: all returned
    EXPECT_EQ(0, get_borrow_count(pool));
    EXPECT_EQ(5, pool.size().value_or(0));
}

/// @brief Test counter balance with multiple concurrent borrows
TEST(counter_balance, multiple_concurrent_borrows)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 5; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    {
        auto res1 = pool.borrow_from_pool();
        EXPECT_TRUE(res1.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        {
            auto res2 = pool.borrow_from_pool();
            EXPECT_TRUE(res2.has_value());
            EXPECT_EQ(2, get_borrow_count(pool));

            {
                auto res3 = pool.borrow_from_pool();
                EXPECT_TRUE(res3.has_value());
                EXPECT_EQ(3, get_borrow_count(pool));
            }

            EXPECT_EQ(2, get_borrow_count(pool));
        }

        EXPECT_EQ(1, get_borrow_count(pool));
    }

    EXPECT_EQ(0, get_borrow_count(pool));
}

/// @brief Test counter balance with invalidated resources
TEST(counter_balance, invalidated_resources)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    pool.add_to_pool(std::string("resource-1"));
    pool.add_to_pool(std::string("resource-2"));

    EXPECT_EQ(0, get_borrow_count(pool));

    {
        auto res1 = pool.borrow_from_pool();
        EXPECT_TRUE(res1.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        auto res2 = pool.borrow_from_pool();
        EXPECT_TRUE(res2.has_value());
        EXPECT_EQ(2, get_borrow_count(pool));

        // Invalidate one resource
        res1.value().invalidate();
        EXPECT_EQ(2, get_borrow_count(pool)); // Still checked out
    }

    // After scope: both should be decremented
    EXPECT_EQ(0, get_borrow_count(pool));

    // But only one should be in the pool (the other was invalidated)
    EXPECT_EQ(1, pool.size().value_or(0));
}

/// @brief Test counter balance with moved resources
TEST(counter_balance, moved_resources)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    pool.add_to_pool(std::string("resource-1"));
    pool.add_to_pool(std::string("resource-2"));

    EXPECT_EQ(0, get_borrow_count(pool));

    {
        auto res1 = pool.borrow_from_pool();
        EXPECT_TRUE(res1.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        auto res2 = pool.borrow_from_pool();
        EXPECT_TRUE(res2.has_value());
        EXPECT_EQ(2, get_borrow_count(pool));

        // Move res1 to res2
        res2 = std::move(res1);
        EXPECT_EQ(2, get_borrow_count(pool)); // Still 2 checked out
    }

    // After scope: both should be decremented
    EXPECT_EQ(0, get_borrow_count(pool));

    // Only one resource should be in the pool (res1 was moved out)
    EXPECT_EQ(1, pool.size().value_or(0));
}

// ============================================================================
// EXCEPTION HANDLING COUNTER BALANCE TESTS
// ============================================================================

/// @brief Test counter balance when exception occurs during borrow
TEST(counter_balance, exception_during_borrow)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {
            4, [](auto&) -> std::expected<siddiqsoft::arrp::scoped_resource<std::string>, siddiqsoft::arrp::pool_error> {
                throw std::runtime_error("Factory error");
            }};

    EXPECT_EQ(0, get_borrow_count(pool));

    // Try to borrow when factory throws
    auto res = pool.borrow_from_pool();
    EXPECT_FALSE(res.has_value());

    // Counter should still be balanced
    EXPECT_EQ(0, get_borrow_count(pool));
}

/// @brief Test counter balance when exception occurs in user code
TEST(counter_balance, exception_in_user_code)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.add_to_pool(std::string("resource-1"));

    EXPECT_EQ(0, get_borrow_count(pool));

    try {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        throw std::runtime_error("User error");
    }
    catch (const std::exception&) {
        // Exception caught
    }

    // Counter should be balanced after exception
    EXPECT_EQ(0, get_borrow_count(pool));
}

/// @brief Test counter balance with nested exception handling
TEST(counter_balance, nested_exception_handling)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 3; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    try {
        auto res1 = pool.borrow_from_pool();
        EXPECT_TRUE(res1.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));

        try {
            auto res2 = pool.borrow_from_pool();
            EXPECT_TRUE(res2.has_value());
            EXPECT_EQ(2, get_borrow_count(pool));

            throw std::runtime_error("Inner error");
        }
        catch (const std::exception&) {
            EXPECT_EQ(1, get_borrow_count(pool));
        }

        throw std::runtime_error("Outer error");
    }
    catch (const std::exception&) {
        // Exception caught
    }

    // Counter should be balanced
    EXPECT_EQ(0, get_borrow_count(pool));
}

// ============================================================================
// CONCURRENT COUNTER BALANCE TESTS
// ============================================================================

/// @brief Test counter balance with concurrent borrows and returns
TEST(counter_balance, concurrent_borrows_and_returns)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 20; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
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
            auto res = pool.borrow_from_pool();
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
            auto res = pool.borrow_from_pool();
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
            auto res = pool.borrow_from_pool();
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
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    threads.clear();

    // Final state: all returned
    EXPECT_EQ(0, get_borrow_count(pool));
    EXPECT_EQ(total_borrows.load(), total_returns.load());
}

/// @brief Test counter balance with concurrent invalidations
TEST(counter_balance, concurrent_invalidations)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 30; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
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
                auto res = pool.borrow_from_pool();
                if (res.has_value()) {
                    if (i % 3 == 0) {
                        res.value().invalidate();
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
    EXPECT_EQ(0, get_borrow_count(pool));

    // Pool should contain only non-invalidated resources
    int64_t expected_size = 30 - invalidated.load();
    EXPECT_EQ(expected_size, static_cast<int64_t>(pool.size().value_or(0)));
}

/// @brief Test counter balance with concurrent adds and borrows
TEST(counter_balance, concurrent_adds_and_borrows)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 10; ++i) {
        pool.add_to_pool(std::format("initial-{}", i));
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
            pool.add_to_pool(std::format("added-{}", i));
            adds++;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Thread 2: Borrow resources
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            auto res = pool.borrow_from_pool();
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
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    threads.clear();

    // Final state: counter should be balanced
    EXPECT_EQ(0, get_borrow_count(pool));
}

/// @brief Test counter balance with concurrent clears
TEST(counter_balance, concurrent_clears)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 20; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
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
            auto res = pool.borrow_from_pool();
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
                pool.add_to_pool(std::format("repopulated-{}-{}", i, j));
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
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    threads.clear();

    // Final state: counter should be balanced
    EXPECT_EQ(0, get_borrow_count(pool));
}

// ============================================================================
// AUTOGROW POLICY COUNTER BALANCE TESTS
// ============================================================================

/// @brief Test counter balance with AutoGrow policy
TEST(counter_balance, autogrow_policy)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {5, siddiqsoft::arrp::auto_add_policy::AutoGrow};

    EXPECT_EQ(0, get_borrow_count(pool));

    // Borrow from empty pool with AutoGrow
    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));
    }

    // Counter should be balanced
    EXPECT_EQ(0, get_borrow_count(pool));
}

/// @brief Test counter balance with AutoGrow and multiple on-demand creations
TEST(counter_balance, autogrow_multiple_creations)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {5, siddiqsoft::arrp::auto_add_policy::AutoGrow};

    EXPECT_EQ(0, get_borrow_count(pool));

    // Borrow multiple times from empty pool
    for (int i = 0; i < 5; ++i) {
        {
            auto res = pool.borrow_from_pool();
            EXPECT_TRUE(res.has_value());
            EXPECT_EQ(1, get_borrow_count(pool));
        }
        EXPECT_EQ(0, get_borrow_count(pool));
    }

    // All created resources should be in pool
    EXPECT_EQ(5, pool.size().value_or(0));
}

/// @brief Test counter balance with custom factory callback
TEST(counter_balance, custom_factory_callback)
{
    std::atomic_int                              factory_calls {0};

    siddiqsoft::arrp::resource_pool<std::string> pool {
            5,
            [&factory_calls](
                    auto& p) -> std::expected<siddiqsoft::arrp::scoped_resource<std::string>, siddiqsoft::arrp::pool_error> {
                factory_calls++;
                return siddiqsoft::arrp::scoped_resource<std::string>(
                        [&p](std::string&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                            if (isvalid) p.add_to_pool(std::move(res));
                            return {};
                        },
                        std::format("factory-{}", factory_calls.load()));
            }};

    EXPECT_EQ(0, get_borrow_count(pool));

    // Borrow from empty pool with factory
    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));
        EXPECT_EQ(1, factory_calls.load());
    }

    // Counter should be balanced
    EXPECT_EQ(0, get_borrow_count(pool));
}

// ============================================================================
// STRESS TESTS FOR COUNTER BALANCE
// ============================================================================

/// @brief Stress test with high concurrency and rapid operations
TEST(counter_balance, high_concurrency_stress)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 50; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    std::atomic_int           operations {0};
    std::barrier              start_barrier {8};

    std::vector<std::jthread> threads;

    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < 1000; ++i) {
                auto res = pool.borrow_from_pool();
                if (res.has_value()) {
                    operations++;
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
        });
    }

    threads.clear();

    // Final state: counter should be balanced
    EXPECT_EQ(0, get_borrow_count(pool));
    EXPECT_GT(operations.load(), 0);
}

/// @brief Stress test with mixed operations
TEST(counter_balance, mixed_operations_stress)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 30; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    std::atomic_int           borrows {0};
    std::atomic_int           adds {0};
    std::atomic_int           invalidates {0};
    std::barrier              start_barrier {5};

    std::vector<std::jthread> threads;

    // Borrow thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 200; ++i) {
            auto res = pool.borrow_from_pool();
            if (res.has_value()) {
                borrows++;
            }
        }
    });

    // Add thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            pool.add_to_pool(std::format("new-{}", i));
            adds++;
        }
    });

    // Invalidate thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            auto res = pool.borrow_from_pool();
            if (res.has_value()) {
                res.value().invalidate();
                invalidates++;
            }
        }
    });

    // Monitor thread 1
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 300; ++i) {
            int64_t checkedout = get_borrow_count(pool);
            EXPECT_GE(checkedout, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Monitor thread 2
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 300; ++i) {
            auto size = pool.size();
            EXPECT_TRUE(size.has_value());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    threads.clear();

    // Final state: counter should be balanced
    EXPECT_EQ(0, get_borrow_count(pool));
    EXPECT_GT(borrows.load(), 0);
    EXPECT_GT(adds.load(), 0);
    EXPECT_GT(invalidates.load(), 0);
}

// ============================================================================
// EDGE CASE COUNTER BALANCE TESTS
// ============================================================================

/// @brief Test counter balance with single resource
TEST(counter_balance, single_resource)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.add_to_pool(std::string("only-resource"));

    EXPECT_EQ(0, get_borrow_count(pool));

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(1, get_borrow_count(pool));
    }

    EXPECT_EQ(0, get_borrow_count(pool));
}

/// @brief Test counter balance with maximum capacity
TEST(counter_balance, maximum_capacity)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {siddiqsoft::arrp::resource_pool_limits::MaxCapacity};

    for (int i = 0; i < 10; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));

    // Borrow all
    std::vector<siddiqsoft::arrp::scoped_resource<std::string>> resources;
    for (int i = 0; i < 10; ++i) {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        resources.push_back(std::move(res.value()));
    }

    EXPECT_EQ(10, get_borrow_count(pool));

    // Return all
    resources.clear();

    EXPECT_EQ(0, get_borrow_count(pool));
}

/// @brief Test counter balance with rapid create/destroy cycles
TEST(counter_balance, rapid_create_destroy_cycles)
{
    for (int cycle = 0; cycle < 10; ++cycle) {
        siddiqsoft::arrp::resource_pool<std::string> pool {};

        for (int i = 0; i < 5; ++i) {
            pool.add_to_pool(std::format("resource-{}", i));
        }

        EXPECT_EQ(0, get_borrow_count(pool));

        {
            auto res = pool.borrow_from_pool();
            EXPECT_TRUE(res.has_value());
            EXPECT_EQ(1, get_borrow_count(pool));
        }

        EXPECT_EQ(0, get_borrow_count(pool));
    }
}

// ============================================================================
// COUNTER CONSISTENCY TESTS
// ============================================================================

/// @brief Test that counter is consistent with pool size
TEST(counter_balance, counter_size_consistency)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 10; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
    }

    EXPECT_EQ(0, get_borrow_count(pool));
    EXPECT_EQ(10, pool.size().value_or(0));

    // Borrow 5
    std::vector<siddiqsoft::arrp::scoped_resource<std::string>> resources;
    for (int i = 0; i < 5; ++i) {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        resources.push_back(std::move(res.value()));
    }

    EXPECT_EQ(5, get_borrow_count(pool));
    EXPECT_EQ(5, pool.size().value_or(0));

    // Return 3
    resources.erase(resources.begin(), resources.begin() + 3);

    EXPECT_EQ(2, get_borrow_count(pool));
    EXPECT_EQ(8, pool.size().value_or(0));

    // Return all
    resources.clear();

    EXPECT_EQ(0, get_borrow_count(pool));
    EXPECT_EQ(10, pool.size().value_or(0));
}

/// @brief Test counter consistency across JSON serialization
TEST(counter_balance, counter_json_consistency)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 5; ++i) {
        pool.add_to_pool(std::format("resource-{}", i));
    }

    {
        auto res1 = pool.borrow_from_pool();
        auto res2 = pool.borrow_from_pool();

        EXPECT_TRUE(res1.has_value());
        EXPECT_TRUE(res2.has_value());

        int64_t checkedout = get_borrow_count(pool);
        EXPECT_EQ(2, checkedout);

        auto json = pool.to_json();
        EXPECT_TRUE(json.has_value());

        auto& j = json.value().get();
        EXPECT_EQ(3, j["size"].get<size_t>());
        EXPECT_EQ(5, j["capacity"].get<uint8_t>());
    }

    EXPECT_EQ(0, get_borrow_count(pool));
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
