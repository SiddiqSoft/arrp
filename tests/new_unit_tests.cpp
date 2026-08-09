
// NEW UNIT TESTS - ADDED FOR COMPREHENSIVE COVERAGE


#include "gtest/gtest.h"
#include <format>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <barrier>
#include <chrono>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/private/resource_pool.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

/// @brief Test 1: Should handle pool capacity enforcement correctly
/// Verifies that when a pool reaches capacity with no auto-grow policy,
/// subsequent borrow attempts fail gracefully with UnderCapacityNoAutoGrow error
TEST(resource_pool, capacity_enforcement_no_autogrow)
{
    constexpr uint8_t POOL_CAPACITY = 3;

    // Create pool with fixed capacity and no auto-grow
    siddiqsoft::arrp::resource_pool<std::string> pool {POOL_CAPACITY};

    // Seed the pool to capacity
    pool.seed(std::string("res1"));
    pool.seed(std::string("res2"));
    pool.seed(std::string("res3"));
    EXPECT_EQ(3u, pool.size());

    // Borrow all resources
    auto res1 = pool.try_borrow();
    auto res2 = pool.try_borrow();
    auto res3 = pool.try_borrow();

    EXPECT_TRUE(res1.has_value());
    EXPECT_TRUE(res2.has_value());
    EXPECT_TRUE(res3.has_value());

    // Pool is now empty and all resources are checked out
    EXPECT_EQ(0u, pool.size());

    // Try to borrow when pool is starving (under capacity) with no auto-grow
    auto res4 = pool.try_borrow();
    EXPECT_FALSE(res4.has_value());
    EXPECT_EQ(res4.error(), siddiqsoft::arrp::pool_error::NoMoreResources);
}

/// @brief Test 3: Should handle loan size accounting with abandons
/// Verifies that loan_size() correctly accounts for borrows, returns,
/// and abandoned resources
TEST(resource_pool, loan_size_with_abandons)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // Seed pool with resources
    pool.seed(std::string("res1"));
    pool.seed(std::string("res2"));
    pool.seed(std::string("res3"));

    // Initial state: loans = 0
    auto json1 = pool.to_json();
    EXPECT_EQ(0, json1["loans"]);

    // Borrow 2 resources
    auto res1 = pool.try_borrow();
    auto res2 = pool.try_borrow();
    EXPECT_TRUE(res1.has_value());
    EXPECT_TRUE(res2.has_value());

    // loans = borrows - returns - abandons = 2 - 0 - 0 = 2
    auto json2 = pool.to_json();
    EXPECT_EQ(2, json2["loans"]);

    // Invalidate one resource (abandon it)
    res1.invalidate();

    // Return both resources
    // res1 is abandoned, res2 is returned
    // loans = 2 - 1 - 1 = 0 (1 return, 1 abandon)
    {
        auto temp1 = std::move(res1);
        auto temp2 = std::move(res2);
    }

    auto json3 = pool.to_json();
    EXPECT_EQ(0, json3["loans"]);
    EXPECT_EQ(1, json3["abandons"]);
    EXPECT_EQ(1, json3["returns"]);
}

/// @brief Test 4: Should prevent deadlocks during concurrent JSON serialization
/// Ensures to_json() can be called safely while other threads are
/// borrowing/returning resources
TEST(resource_pool, concurrent_json_no_deadlock)
{
    constexpr int                                EXPECTED_THREADS = 3;
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // Seed pool
    for (int i = 0; i < 5; ++i) {
        pool.seed(std::format("resource-{}", i));
    }

    std::atomic_int json_reads {0};
    std::atomic_int borrows {0};
    std::barrier    start_barrier {EXPECTED_THREADS};
    std::atomic_int sync_threads_ready {0};

    auto            sync_threads_point = [&] {
#if defined(_WIN64)
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

    // Thread 1: Continuously borrow/return
    auto borrow_thread = std::jthread([&]() {
        sync_threads_point();
        for (int i = 0; i < 100; ++i) {
            auto res = pool.try_borrow();
            if (res.has_value()) {
                borrows++;
            }
        }
    });

    // Thread 2: Continuously call to_json
    auto json_thread = std::jthread([&]() {
        sync_threads_point();
        for (int i = 0; i < 50; ++i) {
            auto json = pool.to_json();
            EXPECT_TRUE(json.contains("seeds"));
            json_reads++;
        }
    });

    // Main thread: Also borrow/return
    sync_threads_point();
    for (int i = 0; i < 50; ++i) {
        auto res = pool.try_borrow();
        if (res.has_value()) {
            borrows++;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Verify operations completed
    EXPECT_GT(json_reads.load(), 0);
    EXPECT_GT(borrows.load(), 0);
}

/// @brief Test 5: Should properly handle resource invalidation in move assignment
/// Verifies that when a resource_guard is move-assigned, the previous
/// resource is properly returned before taking ownership of the new one
TEST(resource_guard, move_assignment_returns_previous)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // Seed pool with resources
    pool.seed(std::string("original"));
    pool.seed(std::string("replacement"));

    EXPECT_EQ(2u, pool.size());

    // Borrow first resource
    auto res1_result = pool.try_borrow();
    EXPECT_TRUE(res1_result.has_value());
    auto res1 = std::move(res1_result);
    EXPECT_EQ("original", *res1);
    EXPECT_EQ(1u, pool.size());

    // Borrow second resource
    auto res2_result = pool.try_borrow();
    EXPECT_TRUE(res2_result.has_value());
    auto res2 = std::move(res2_result);
    EXPECT_EQ("replacement", *res2);
    EXPECT_EQ(0u, pool.size());

    // Move-assign res2 to res1
    // This should return "original" back to the pool
    res1 = std::move(res2);
    EXPECT_EQ("replacement", *res1);

    // After move-assignment, "original" should be back in pool
    EXPECT_EQ(1u, pool.size());

    // Verify we can borrow the original resource
    auto res3_result = pool.try_borrow();
    EXPECT_TRUE(res3_result.has_value());
    auto res3 = std::move(res3_result);
    EXPECT_EQ("original", *res3);
}

/// @brief Test 6: Verifies that assigning T&& to a resource_guard returns the old resource to the pool
TEST(resource_guard, operator_assign_value_returns_previous)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed(std::string("initial_val"));
    EXPECT_EQ(1u, pool.size());

    {
        auto guard = pool.try_borrow();
        EXPECT_TRUE(guard.has_value());
        EXPECT_EQ("initial_val", *guard);
        EXPECT_EQ(0u, pool.size());

        // Assign a new T value to guard
        guard = std::string("assigned_val");
        EXPECT_EQ("assigned_val", *guard);

        // The old resource ("initial_val") should be returned to pool immediately
        EXPECT_EQ(1u, pool.size());
    }

    // When guard goes out of scope, "assigned_val" is returned to pool as well
    EXPECT_EQ(2u, pool.size());
}


/// @brief Test 8: Verifies concurrent borrow and return does not cause checkout counter underflow
TEST(resource_pool, concurrent_borrow_return_no_underflow)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < 10; ++i) {
        pool.seed(std::string("res_") + std::to_string(i));
    }

    std::atomic<bool> stop {false};
    std::vector<std::thread> threads;

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            while (!stop.load()) {
                auto g = pool.try_borrow();
                if (g.has_value()) {
                    std::this_thread::yield();
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop.store(true);

    for (auto& t : threads) {
        t.join();
    }

    // All resources returned, loan counter must be 0 (no underflow/corruption)
    EXPECT_EQ(0, pool.to_json()["loans"]);
    EXPECT_EQ(10u, pool.size());
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
