/*
    Comprehensive Coverage Tests for resource_pool and scoped_resource

    This file contains additional tests to ensure full coverage of:
    1. Edge cases and boundary conditions
    2. Error handling paths
    3. Operator overloads and special member functions
    4. JSON serialization edge cases
    5. Shutdown and lifecycle management
    6. Pointer-like access patterns
    7. Resource validity tracking
    8. Concurrent access patterns with edge cases

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
#include <map>
#include <set>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/private/resource_pool.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)


/// @brief Test scoped_resource pointer-like access operator->
TEST(scoped_resource_operators, pointer_access)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("test-string"));

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        auto& sr = res;

        // Test operator->
        EXPECT_NE(nullptr, sr.operator->());
        EXPECT_EQ(11u, sr->size());
        sr->append("-modified");
        EXPECT_EQ(20u, sr->size());
    }

    // Verify modification persisted
    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ("test-string-modified", *res);
    }
}

/// @brief Test scoped_resource explicit conversion operator
TEST(scoped_resource_operators, explicit_conversion)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("conversion-test"));

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        auto& sr = res;

        // Test explicit conversion to T&
        auto& ref = static_cast<std::string&>(sr);
        EXPECT_EQ("conversion-test", ref);
        ref.append("-converted");
    }

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ("conversion-test-converted", *res);
    }
}

/// @brief Test scoped_resource is_valid method
TEST(scoped_resource_operators, is_valid_method)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("valid-test"));

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        auto& sr = res;

        EXPECT_TRUE(sr.is_valid());
        sr.invalidate();
        EXPECT_FALSE(sr.is_valid());
    }

    // Resource should not be returned
    EXPECT_EQ(0u, pool.size());
}


class custom_masp
{
public:
    std::string v;

    custom_masp() = default;

    explicit operator std::string&() { return v; }
    explicit operator const char*() { return v.c_str(); }

    custom_masp(const std::string& s)
        : v(s)
    {
    }
    custom_masp(std::string&& s)
        : v(std::move(s))
    {
    }
    custom_masp(custom_masp&& src) noexcept
        : v(std::move(src.v))
    {
    }
    custom_masp& operator=(custom_masp&& src) noexcept
    {
        if (this != &src) {
            v = std::move(src.v);
        }
        return *this;
    }
    auto operator=(const std::string& s) -> custom_masp&
    {
        v = s;
        return *this;
    }
    ~custom_masp() { std::print(std::cerr, "{} - destroyed: {}\n", __func__, v); }
    bool                 operator==(const std::string& src) const { return v == src; }
    bool                 operator==(const char* src) const { return v == src; }
    std::strong_ordering operator<=>(const std::string& src) const { return v <=> src; }
    std::strong_ordering operator<=>(const char* src) const { return v <=> src; }
};

/// @brief Test scoped_resource move assignment with self-assignment protection
TEST(scoped_resource_operators, move_assignment_self_protection)
{
    siddiqsoft::arrp::resource_pool<custom_masp> pool {};

    pool.seed_to_pool(custom_masp {"resource1"});
    pool.seed_to_pool(custom_masp {"resource2"});
    EXPECT_EQ(2, pool.size());

    { // borrow two and clobber one of them..
        auto res1 = pool.borrow_from_pool();
        auto res2 = pool.borrow_from_pool();

        EXPECT_TRUE(res1.has_value());
        EXPECT_TRUE(res2.has_value());

        auto& sr1 = res1;
        auto& sr2 = res2;

        // Move assign sr2 to sr1:
        // - sr1's current resource ("resource1") is returned to the pool first
        // - sr1 then takes ownership of sr2's resource ("resource2")
        // - sr2 is left invalid (no callback, no resource)
        sr1 = std::move(sr2);

        // sr1 should now hold sr2's resource
        EXPECT_EQ("resource2", (*sr1).v);
        // sr2 must be invalid after the move
        EXPECT_FALSE(sr2.is_valid());
    }
    // On scope exit: sr1 (holding "resource2") is returned to pool.
    // sr2 is invalid so nothing is returned for it.
    // "resource1" was already returned during the move-assignment above.
    // Net result: both resources are back in the pool.
    EXPECT_EQ(2, pool.size());
}

/// @brief Test scoped_resource with nullptr pointer access
TEST(scoped_resource_operators, nullptr_pointer_access)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("test"));

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        auto& sr = res;

        // Before invalidation, operator-> should return valid pointer
        EXPECT_NE(nullptr, sr.operator->());

        sr.invalidate();

        // After invalidation, operator-> should return nullptr
        EXPECT_EQ(nullptr, sr.operator->());
    }
}


// RESOURCE_POOL CONSTRUCTOR TESTS


/// @brief Test resource_pool with cleanup callback only
TEST(resource_pool_constructors, cleanup_callback_only)
{
    std::atomic_int cleanup_count {0};

    {
        siddiqsoft::arrp::resource_pool<std::string> pool {[&cleanup_count](auto&& item) {
            cleanup_count++;
            std::print(std::cerr, "Cleanup called for: {}\n", item);
        }};

        pool.seed_to_pool(std::string("item1"));
        pool.seed_to_pool(std::string("item2"));

        EXPECT_EQ(2u, pool.size());
    }

    // Destructor should call cleanup for both items
    EXPECT_EQ(2, cleanup_count.load());
}


// RESOURCE_POOL SIZE AND CAPACITY TESTS


/// @brief Test size() returns correct value under concurrent modifications
TEST(resource_pool_size, concurrent_size_accuracy)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    std::atomic_int                              add_count {0};
    std::barrier                                 start_barrier {3};

    std::vector<std::jthread>                    threads;

    // Thread 1: Add resources
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();

        std::println(std::cerr, "Adding 50 resources to the pool...");
        for (int i = 0; i < 50; ++i) {
            EXPECT_EQ(siddiqsoft::arrp::pool_error::Ok, pool.seed_to_pool(std::format("resource-{}", i)));
            add_count++;
        }

        std::println(std::cerr, "Finished Adding 50 resources to the pool...{}", pool.to_json().dump());
        EXPECT_GE(pool.size(), 50);
    });


    // Thread 2: Borrow resources
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        for (int i = 0; i < 25; ++i) {
            auto res = pool.borrow_from_pool();
            if (res.has_value()) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    });

    // Thread 3: Check size
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (int i = 0; i < 10; ++i) {
            auto sz = pool.size();
            EXPECT_GT(sz, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    threads.clear();

    EXPECT_EQ(50, add_count.load());
}

/// @brief Test size() after shutdown
TEST(resource_pool_size, size_after_shutdown)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("item"));

    // Destroy the pool
    {
        siddiqsoft::arrp::resource_pool<std::string> temp_pool {};
        temp_pool.seed_to_pool(std::string("temp"));
    }

    // After destruction, the pool is gone, so we can't test it
    // But we can test that a new pool works fine
    siddiqsoft::arrp::resource_pool<std::string> new_pool {};
    new_pool.seed_to_pool(std::string("new"));
    EXPECT_EQ(1u, new_pool.size());
}


// RESOURCE_POOL BORROW TESTS


/// @brief Test borrow_from_pool with empty pool and no factory
TEST(resource_pool_borrow, empty_pool_no_factory)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {4};

    auto                                         res = pool.borrow_from_pool();
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), siddiqsoft::arrp::pool_error::NoMoreResources);
}


/// @brief Test borrow_from_pool returns FIFO order
TEST(resource_pool_borrow, fifo_order_verification)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // Add in specific order
    pool.seed_to_pool(std::string("first"));
    pool.seed_to_pool(std::string("second"));
    pool.seed_to_pool(std::string("third"));

    // Borrow in FIFO order
    {
        auto res1 = pool.borrow_from_pool();
        EXPECT_TRUE(res1.has_value());
        EXPECT_EQ("first", *res1);

        auto res2 = pool.borrow_from_pool();
        EXPECT_TRUE(res2.has_value());
        EXPECT_EQ("second", *res2);

        auto res3 = pool.borrow_from_pool();
        EXPECT_TRUE(res3.has_value());
        EXPECT_EQ("third", *res3);
    }

    // All returned
    EXPECT_EQ(3u, pool.size());
}

/// @brief Test borrow_from_pool with capacity limits
TEST(resource_pool_borrow, capacity_limit_enforcement)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {2};

    pool.seed_to_pool(std::string("1"));
    pool.seed_to_pool(std::string("2"));

    // Borrow both
    auto res1 = pool.borrow_from_pool();
    auto res2 = pool.borrow_from_pool();

    EXPECT_TRUE(res1.has_value());
    EXPECT_TRUE(res2.has_value());

    // Try to borrow beyond capacity
    auto res3 = pool.borrow_from_pool();
    EXPECT_FALSE(res3.has_value());
}


// RESOURCE_POOL seed_to_pool TESTS


/// @brief Test seed_to_pool with variadic arguments
TEST(resource_pool_add, variadic_arguments)
{
    siddiqsoft::arrp::resource_pool<std::pair<int, std::string>> pool {};

    // Add using variadic constructor
    pool.seed_to_pool(42, "answer");

    EXPECT_EQ(1u, pool.size());

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(42, (*res).first);
        EXPECT_EQ("answer", (*res).second);
    }
}

/// @brief Test seed_to_pool with rvalue reference
TEST(resource_pool_add, rvalue_reference)
{
    siddiqsoft::arrp::resource_pool<std::vector<int>> pool {};

    std::vector<int>                                  vec {1, 2, 3, 4, 5};
    pool.seed_to_pool(std::move(vec));

    EXPECT_TRUE(vec.empty()); // Original should be moved

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(5u, (*res).size());
    }
}

/// @brief Test seed_to_pool after shutdown
TEST(resource_pool_add, add_after_shutdown)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("item"));

    // Simulate shutdown by destroying and recreating
    {
        siddiqsoft::arrp::resource_pool<std::string> temp {};
    }

    // New pool should work fine
    siddiqsoft::arrp::resource_pool<std::string> new_pool {};
    auto                                         result = new_pool.seed_to_pool(std::string("new-item"));
    EXPECT_TRUE(result == siddiqsoft::arrp::pool_error::Ok);
}


// RESOURCE_POOL RETURN_TO_POOL TESTS


/// @brief Test return_to_pool with valid resource
TEST(resource_pool_return, valid_resource_return)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("test"));

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        // Resource will be returned on destruction
    }

    EXPECT_EQ(1u, pool.size());
}

/// @brief Test return_to_pool with invalid resource
TEST(resource_pool_return, invalid_resource_not_returned)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("test"));

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        res.invalidate();
        // Resource will NOT be returned on destruction
    }

    EXPECT_EQ(0u, pool.size());
}


// RESOURCE_POOL CLEAR TESTS


/// @brief Test clear with cleanup callback
TEST(resource_pool_clear, with_cleanup_callback)
{
    std::atomic_int cleanup_count {0};

    {
        siddiqsoft::arrp::resource_pool<std::string> pool {[&cleanup_count](auto&) { cleanup_count++; }};

        pool.seed_to_pool(std::string("1"));
        pool.seed_to_pool(std::string("2"));
        pool.seed_to_pool(std::string("3"));

        EXPECT_EQ(3u, pool.size());

        auto result = pool.clear();
        EXPECT_EQ(siddiqsoft::arrp::pool_error::Ok, result);

        EXPECT_EQ(3, cleanup_count.load());
        EXPECT_EQ(0u, pool.size());
    }
}

/// @brief Test clear without cleanup callback
TEST(resource_pool_clear, without_cleanup_callback)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    pool.seed_to_pool(std::string("1"));
    pool.seed_to_pool(std::string("2"));

    EXPECT_EQ(2u, pool.size());

    auto result = pool.clear();
    EXPECT_EQ(siddiqsoft::arrp::pool_error::Ok, result);
    EXPECT_EQ(0u, pool.size());
}

/// @brief Test clear during concurrent borrow operations
TEST(resource_pool_clear, concurrent_with_borrow)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 10; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }
    EXPECT_EQ(10, pool.size());

    std::atomic_bool stop {false};
    std::atomic_int  clears {0};
    std::atomic_int  borrows {0};
    std::atomic_int  borrow_fails {0};

    auto             worker = std::jthread([&]() {
        for (int i = 0; i < 100 && !stop.load(); ++i) {
            auto res = pool.borrow_from_pool();
            if (res.has_value()) {
                borrows++;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            else
                borrow_fails++;
        }
    });

    // A slight pause will ensure that the borrow will fulfill at least one
    // before the clear()ers do their work.
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto clearer = std::jthread([&]() {
        for (int i = 0; i < 5 && !stop.load(); ++i) {
            pool.clear();
            clears++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // Repopulate
            for (int j = 0; j < 5; ++j) {
                pool.seed_to_pool(std::format("repopulated-{}", j));
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop = true;

    worker.join();
    clearer.join();

    std::println(std::cerr, "borrows:{}. borrow_fails:{}. clears:{}", borrows.load(), borrow_fails.load(), clears.load());
    EXPECT_GT(clears.load(), 0);
    EXPECT_GT(borrows.load(), 0);
}


// RESOURCE_POOL JSON SERIALIZATION TESTS


/// @brief Test to_json with empty pool
TEST(resource_pool_json, empty_pool_serialization)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    auto                                         j = pool.to_json();
    EXPECT_TRUE(j.is_object());
    std::println(std::cerr, "{} - Contents: {}", __func__, j.dump());
    EXPECT_TRUE(j.contains("_typver"));
    EXPECT_TRUE(j.contains("capacity"));
    EXPECT_TRUE(j.contains("size"));
    EXPECT_EQ(0u, j["size"].get<size_t>());
}

/// @brief Test to_json with populated pool
TEST(resource_pool_json, populated_pool_serialization)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    pool.seed_to_pool(std::string("item1"));
    pool.seed_to_pool(std::string("item2"));


    auto res = pool.borrow_from_pool();
    EXPECT_TRUE(res.has_value());


    auto j = pool.to_json();
    EXPECT_TRUE(j.is_object());

    EXPECT_EQ(1u, j["size"].get<size_t>());
}

/// @brief Test to_json with counters
TEST(resource_pool_json, counter_tracking)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    pool.seed_to_pool(std::string("item"));

    // Borrow and return multiple times
    for (int i = 0; i < 5; ++i) {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
    }

    auto j = pool.to_json();
    EXPECT_TRUE(j.is_object());

    std::print(std::cerr, "contents of the stats:{}\n", j.dump());
    EXPECT_EQ(5u, j["returns"].get<uint64_t>());
}

/// @brief Test to_json with invalid returns
TEST(resource_pool_json, invalid_returns_tracking)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    pool.seed_to_pool(std::string("item1"));
    pool.seed_to_pool(std::string("item2"));

    // Invalidate one resource
    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        res.invalidate();
    }

    // Return one normally
    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
    }

    auto j = pool.to_json();
    EXPECT_TRUE(j.is_object());

    EXPECT_EQ(1u, j["abandons"].get<uint16_t>());
}

/// @brief Test to_json with JSON type resources
TEST(resource_pool_json, json_type_resources)
{
    siddiqsoft::arrp::resource_pool<nlohmann::json> pool {};

    nlohmann::json                                  j1 = {{"key", "value"}};
    nlohmann::json                                  j2 = {{"number", 42}};

    pool.seed_to_pool(std::move(j1));
    pool.seed_to_pool(std::move(j2));

    auto j = pool.to_json();
    EXPECT_TRUE(j.is_object());

    EXPECT_EQ(2u, j["size"].get<size_t>());
    EXPECT_TRUE(j.contains("items"));
}


// RESOURCE_POOL DESTRUCTOR AND LIFECYCLE TESTS


/// @brief Test destructor calls cleanup for all resources
TEST(resource_pool_lifecycle, destructor_cleanup)
{
    std::atomic_int cleanup_count {0};

    {
        siddiqsoft::arrp::resource_pool<std::string> pool {[&cleanup_count](auto& item) { cleanup_count++; }};

        pool.seed_to_pool(std::string("1"));
        pool.seed_to_pool(std::string("2"));
        pool.seed_to_pool(std::string("3"));

        // Borrow one (it will be checked out)
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());

        // Now we have 2 in pool, 1 checked out
        EXPECT_EQ(2u, pool.size());
    }

    // Destructor should clean up the 2 items in pool
    // The checked-out item will be cleaned up when it's returned
    EXPECT_GE(cleanup_count.load(), 2);
}

/// @brief Test pool behavior after destruction
TEST(resource_pool_lifecycle, post_destruction_behavior)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("item"));

    // Create a new pool after the first one
    siddiqsoft::arrp::resource_pool<std::string> pool2 {};
    EXPECT_EQ(siddiqsoft::arrp::pool_error::Ok, pool2.seed_to_pool(std::string("item2")));

    EXPECT_EQ(1u, pool2.size());
}


// EDGE CASES AND BOUNDARY CONDITIONS


/// @brief Test with minimum capacity
TEST(edge_cases, minimum_capacity)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {siddiqsoft::arrp::resource_pool_limits::MinimumCapacity};

    pool.seed_to_pool(std::string("item"));
    EXPECT_EQ(1u, pool.size());

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
    }

    EXPECT_EQ(1u, pool.size());
}

/// @brief Test with maximum capacity
TEST(edge_cases, maximum_capacity)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {siddiqsoft::arrp::resource_pool_limits::MaxCapacity};

    // Add one item
    pool.seed_to_pool(std::string("item"));
    EXPECT_EQ(1u, pool.size());

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
    }

    EXPECT_EQ(1u, pool.size());
}

/// @brief Test with capacity exceeding maximum
TEST(edge_cases, capacity_exceeds_maximum)
{
    // Request capacity larger than max - should be clamped
    siddiqsoft::arrp::resource_pool<std::string> pool {
            static_cast<uint8_t>(siddiqsoft::arrp::resource_pool_limits::MaxCapacity + 1)};

    // Pool should still work with clamped capacity
    pool.seed_to_pool(std::string("item"));
    EXPECT_EQ(1u, pool.size());
}

/// @brief Test with empty string resources
TEST(edge_cases, empty_string_resources)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    pool.seed_to_pool(std::string(""));
    pool.seed_to_pool(std::string(""));

    EXPECT_EQ(2u, pool.size());

    {
        auto res1 = pool.borrow_from_pool();
        auto res2 = pool.borrow_from_pool();

        EXPECT_TRUE(res1.has_value());
        EXPECT_TRUE(res2.has_value());
        EXPECT_EQ("", *res1);
        EXPECT_EQ("", *res2);
    }

    EXPECT_EQ(2u, pool.size());
}

/// @brief Test with very large strings
TEST(edge_cases, large_string_resources)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    std::string                                  large_str(1000000, 'x'); // 1MB string
    pool.seed_to_pool(large_str);

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(1000000u, res->size());
    }

    EXPECT_EQ(1u, pool.size());
}

/// @brief Test with nested containers
TEST(edge_cases, nested_containers)
{
    using NestedType = std::vector<std::vector<std::string>>;
    siddiqsoft::arrp::resource_pool<NestedType> pool {};

    NestedType                                  nested {{"a", "b", "c"}, {"d", "e", "f"}, {"g", "h", "i"}};

    pool.seed_to_pool(nested);

    {
        auto res = pool.borrow_from_pool();
        EXPECT_TRUE(res.has_value());
        EXPECT_EQ(3u, res->size());
        EXPECT_EQ(3u, (*res)[0].size());
    }

    EXPECT_EQ(1u, pool.size());
}


// CONCURRENT STRESS TESTS WITH EDGE CASES


/// @brief Test concurrent access with rapid invalidation
TEST(concurrent_edge_cases, rapid_invalidation)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 20; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_int           invalidated {0};
    std::atomic_int           returned {0};
    std::barrier              start_barrier {8};

    std::vector<std::jthread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < 50; ++i) {
                auto res = pool.borrow_from_pool();
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

    EXPECT_GT(invalidated.load(), 0);
    EXPECT_GT(returned.load(), 0);
    EXPECT_EQ(20u - invalidated.load(), pool.size());
}

/// @brief Test concurrent access with mixed resource types
TEST(concurrent_edge_cases, mixed_operations_stress)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 10; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    EXPECT_EQ(10, pool.size());

    std::atomic_int           borrows {0};
    std::atomic_int           adds {0};
    std::atomic_int           clears {0};
    std::atomic_int           json_calls {0};
    std::barrier              start_barrier {4};

    std::vector<std::jthread> threads;

    // Borrow thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 100; ++i) {
            auto res = pool.borrow_from_pool();
            if (res.has_value()) {
                borrows++;
            }
        }
    });

    // Add thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 50; ++i) {
            pool.seed_to_pool(std::format("new-{}", i));
            adds++;
        }
    });

    // Clear thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        // this small wait is critical to ensure we do not experience intermittent test fails.
        std::this_thread::sleep_for(std::chrono::milliseconds(90));
        for (int i = 0; i < 5; ++i) {
            pool.clear();
            clears++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // JSON thread
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 50; ++i) {
            auto json = pool.to_json();
            if (json.is_object()) {
                json_calls++;
            }
        }
    });

    threads.clear();

    EXPECT_GT(borrows.load(), 0);
    EXPECT_GT(adds.load(), 0);
    EXPECT_GT(clears.load(), 0);
    EXPECT_GT(json_calls.load(), 0);
}

/// @brief Test with alternating valid/invalid resources
TEST(concurrent_edge_cases, alternating_validity)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 30; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::vector<siddiqsoft::arrp::scoped_resource<std::string>> resources;

    // Borrow all
    for (int i = 0; i < 30; ++i) {
        auto res = pool.borrow_from_pool();
        if (res.has_value()) {
            resources.push_back(std::move(res));
        }
    }

    EXPECT_EQ(0u, pool.size());

    // Invalidate every other one
    for (size_t i = 0; i < resources.size(); i += 2) {
        resources[i].invalidate();
    }

    // Return all
    resources.clear();

    // Should have 15 resources back (the non-invalidated ones)
    EXPECT_EQ(15u, pool.size());
}


// SPECIAL MEMBER FUNCTION TESTS


/// @brief Test that resource_pool is not copyable
TEST(special_members, pool_not_copyable)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // This should not compile, but we can verify the type traits
    EXPECT_FALSE(std::is_copy_constructible_v<siddiqsoft::arrp::resource_pool<std::string>>);
    EXPECT_FALSE(std::is_copy_assignable_v<siddiqsoft::arrp::resource_pool<std::string>>);
}

/// @brief Test that resource_pool is not movable
TEST(special_members, pool_not_movable)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // This should not compile, but we can verify the type traits
    EXPECT_FALSE(std::is_move_constructible_v<siddiqsoft::arrp::resource_pool<std::string>>);
    EXPECT_FALSE(std::is_move_assignable_v<siddiqsoft::arrp::resource_pool<std::string>>);
}

/// @brief Test that scoped_resource is not copyable
TEST(special_members, scoped_resource_not_copyable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<siddiqsoft::arrp::scoped_resource<std::string>>);
    EXPECT_FALSE(std::is_copy_assignable_v<siddiqsoft::arrp::scoped_resource<std::string>>);
}

/// @brief Test that scoped_resource is move-only
TEST(special_members, scoped_resource_is_movable)
{
    EXPECT_TRUE(std::is_move_constructible_v<siddiqsoft::arrp::scoped_resource<std::string>>);
    EXPECT_TRUE(std::is_move_assignable_v<siddiqsoft::arrp::scoped_resource<std::string>>);
}


// FORMATTER TESTS


/// @brief Test std::format with resource_pool
TEST(formatters, resource_pool_format)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("item"));

    std::string formatted = std::format("{}", pool);
    EXPECT_FALSE(formatted.empty());
    EXPECT_TRUE(formatted.find("capacity") != std::string::npos);
}

/// @brief Test std::format with scoped_resource
TEST(formatters, scoped_resource_format)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("test-item"));

    {
        auto res = pool.borrow_from_pool();
        if (res.has_value()) {
            std::print(std::cerr, "{}\n", *res);
            std::string formatted = std::format("{}", *res);
            EXPECT_FALSE(formatted.empty());
        }
    }
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
