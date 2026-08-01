// ============================================================================
// NEW UNIT TESTS - ADDED FOR COMPREHENSIVE COVERAGE
// ============================================================================

#include "gtest/gtest.h"
#include <format>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <barrier>
#include <chrono>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/resource_pool.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

/// @brief Test 1: Should handle pool capacity enforcement correctly
/// Verifies that when a pool reaches capacity with no auto-grow policy,
/// subsequent borrow attempts fail gracefully with UnderCapacityNoAutoGrow error
TEST(resource_pool, capacity_enforcement_no_autogrow)
{
    constexpr uint8_t POOL_CAPACITY = 3;
    
    // Create pool with fixed capacity and no auto-grow
    siddiqsoft::arrp::resource_pool<std::string> pool {
        POOL_CAPACITY,
        siddiqsoft::arrp::auto_add_policy::NoGrow
    };

    // Seed the pool to capacity
    pool.seed_to_pool(std::string("res1"));
    pool.seed_to_pool(std::string("res2"));
    pool.seed_to_pool(std::string("res3"));
    EXPECT_EQ(3u, pool.size());

    // Borrow all resources
    auto res1 = pool.borrow_from_pool();
    auto res2 = pool.borrow_from_pool();
    auto res3 = pool.borrow_from_pool();

    EXPECT_TRUE(res1.has_value());
    EXPECT_TRUE(res2.has_value());
    EXPECT_TRUE(res3.has_value());

    // Pool is now empty and all resources are checked out
    EXPECT_EQ(0u, pool.size());

    // Try to borrow when pool is starving (under capacity) with no auto-grow
    auto res4 = pool.borrow_from_pool();
    EXPECT_FALSE(res4.has_value());
    EXPECT_EQ(res4.error(), siddiqsoft::arrp::pool_error::NoMoreResources);
}

/// @brief Test 2: Should properly track deficit size calculations
/// Ensures deficit_size() correctly calculates the difference between
/// capacity and current resources (pool + checked-out)
TEST(resource_pool, deficit_size_calculation)
{
    constexpr uint8_t POOL_CAPACITY = 5;
    
    siddiqsoft::arrp::resource_pool<std::string> pool {POOL_CAPACITY};

    // Initially, pool is empty: deficit = capacity - (pool_size + checked_out)
    // deficit = 5 - (0 + 0) = 5
    auto json1 = pool.to_json().value().get();
    EXPECT_EQ(5, json1["deficit"]);

    // Add 2 resources to pool
    pool.seed_to_pool(std::string("res1"));
    pool.seed_to_pool(std::string("res2"));
    // deficit = 5 - (2 + 0) = 3
    auto json2 = pool.to_json().value().get();
    EXPECT_EQ(3, json2["deficit"]);

    // Borrow 1 resource
    auto res = pool.borrow_from_pool();
    EXPECT_TRUE(res.has_value());
    // deficit = 5 - (1 + 1) = 3 (pool has 1, checked out has 1)
    auto json3 = pool.to_json().value().get();
    EXPECT_EQ(3, json3["deficit"]);

    // Return the resource
    // (resource goes out of scope and is returned)
}

/// @brief Test 3: Should handle loan size accounting with abandons
/// Verifies that loan_size() correctly accounts for borrows, returns,
/// and abandoned resources
TEST(resource_pool, loan_size_with_abandons)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // Seed pool with resources
    pool.seed_to_pool(std::string("res1"));
    pool.seed_to_pool(std::string("res2"));
    pool.seed_to_pool(std::string("res3"));

    // Initial state: loans = 0
    auto json1 = pool.to_json().value().get();
    EXPECT_EQ(0, json1["loans"]);

    // Borrow 2 resources
    auto res1 = pool.borrow_from_pool();
    auto res2 = pool.borrow_from_pool();
    EXPECT_TRUE(res1.has_value());
    EXPECT_TRUE(res2.has_value());

    // loans = borrows - returns - abandons = 2 - 0 - 0 = 2
    auto json2 = pool.to_json().value().get();
    EXPECT_EQ(2, json2["loans"]);

    // Invalidate one resource (abandon it)
    res1.value().invalidate();

    // Return both resources
    // res1 is abandoned, res2 is returned
    // loans = 2 - 1 - 1 = 0 (1 return, 1 abandon)
    {
        auto temp1 = std::move(res1.value());
        auto temp2 = std::move(res2.value());
    }

    auto json3 = pool.to_json().value().get();
    EXPECT_EQ(0, json3["loans"]);
    EXPECT_EQ(1, json3["abandons"]);
    EXPECT_EQ(1, json3["returns"]);
}

/// @brief Test 4: Should prevent deadlocks during concurrent JSON serialization
/// Ensures to_json() can be called safely while other threads are
/// borrowing/returning resources
TEST(resource_pool, concurrent_json_no_deadlock)
{
    constexpr int EXPECTED_THREADS = 3;
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // Seed pool
    for (int i = 0; i < 5; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_int json_reads {0};
    std::atomic_int borrows {0};
    std::barrier    start_barrier {EXPECTED_THREADS};
    std::atomic_int sync_threads_ready{0};

    auto             sync_threads_point = [&] {
#if defined(_WIN64)
        sync_threads_ready++;
        while (sync_threads_ready.load() < EXPECTED_THREADS) {
            std::this_thread::yield();
        }
#else
        start_barrier.arrive_and_wait();
#endif
        std::println(std::cerr, "   concurrent_json_deadlock_detection - All threads ready to continue..{}/{}", sync_threads_ready.load(), EXPECTED_THREADS);
    };

    // Thread 1: Continuously borrow/return
    auto borrow_thread = std::jthread([&]() {
        sync_threads_point();
        for (int i = 0; i < 100; ++i) {
            auto res = pool.borrow_from_pool();
            if (res.has_value()) {
                borrows++;
                auto r = std::move(res.value());
            }
        }
    });

    // Thread 2: Continuously call to_json
    auto json_thread = std::jthread([&]() {
        sync_threads_point();
        for (int i = 0; i < 50; ++i) {
            auto& json = pool.to_json().value().get();
            EXPECT_TRUE(json.contains("seeds"));
            json_reads++;
        }
    });

    // Main thread: Also borrow/return
    sync_threads_point();
    for (int i = 0; i < 50; ++i) {
        auto res = pool.borrow_from_pool();
        if (res.has_value()) {
            borrows++;
            auto r = std::move(res.value());
        }
    }

    // Verify operations completed
    EXPECT_GT(json_reads.load(), 0);
    EXPECT_GT(borrows.load(), 0);
}

/// @brief Test 5: Should properly handle resource invalidation in move assignment
/// Verifies that when a scoped_resource is move-assigned, the previous
/// resource is properly returned before taking ownership of the new one
TEST(scoped_resource, move_assignment_returns_previous)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // Seed pool with resources
    pool.seed_to_pool(std::string("original"));
    pool.seed_to_pool(std::string("replacement"));

    EXPECT_EQ(2u, pool.size());

    // Borrow first resource
    auto res1_result = pool.borrow_from_pool();
    EXPECT_TRUE(res1_result.has_value());
    auto res1 = std::move(res1_result.value());
    EXPECT_EQ("original", *res1);
    EXPECT_EQ(1u, pool.size());

    // Borrow second resource
    auto res2_result = pool.borrow_from_pool();
    EXPECT_TRUE(res2_result.has_value());
    auto res2 = std::move(res2_result.value());
    EXPECT_EQ("replacement", *res2);
    EXPECT_EQ(0u, pool.size());

    // Move-assign res2 to res1
    // This should return "original" back to the pool
    res1 = std::move(res2);
    EXPECT_EQ("replacement", *res1);

    // After move-assignment, "original" should be back in pool
    EXPECT_EQ(1u, pool.size());

    // Verify we can borrow the original resource
    auto res3_result = pool.borrow_from_pool();
    EXPECT_TRUE(res3_result.has_value());
    auto res3 = std::move(res3_result.value());
    EXPECT_EQ("original", *res3);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
