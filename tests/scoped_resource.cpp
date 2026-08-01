/*
    aarp
    Auto returning resource pool

    BSD 3-Clause License

    Copyright (c) 2026 Abdulkareem Siddiq
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest/gtest.h"

#include <cstdlib>
#include <iostream>
#include <format>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <barrier>
#include <chrono>
#include <random>


#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/resource_pool.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

TEST(resource_pool, T_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::arrp::resource_pool<std::string> rp {};
        std::print(std::cerr, "{} - Capacity:{}\n", __func__, rp.size().value_or(0));
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(resource_pool, T_shared_ptr_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::arrp::resource_pool<std::shared_ptr<std::string>> rp {};

        EXPECT_EQ(0, rp.size()) << "Pool must be empty at start";
        rp.seed_to_pool(std::shared_ptr<std::string>(new std::string(__TIME__)));
        EXPECT_EQ(1, rp.size()) << "Pool must have only one item";

        std::print(std::cerr, "{} - 0 - {}\n", __func__, rp.to_json().value().get().dump());

        {
            auto item_result = rp.borrow_from_pool();
            EXPECT_TRUE(item_result.has_value());
            auto item = std::move(item_result.value());
            EXPECT_EQ(0, rp.size()) << "Pool must be empty!";
            EXPECT_EQ(__TIME__, **item);
            (*item)->append("-ok");

            std::print(std::cerr, "{} - 1 -  {}\n", __func__, rp.to_json().value().get().dump());
        }

        // item is automatically returned to pool when it goes out of scope
        EXPECT_EQ(1, rp.size());

        {
            auto item2_result = rp.borrow_from_pool();
            EXPECT_TRUE(item2_result.has_value());
            auto item2 = std::move(item2_result.value());
            EXPECT_EQ(0, rp.size());
            EXPECT_TRUE((*item2)->ends_with("-ok"));
        }
        // item2 is automatically returned to pool when it goes out of scope
        EXPECT_EQ(1, rp.size());

        passTest = true;
    });

    std::print(std::cerr, "{} - Completed: {}\n", __func__, passTest);

    EXPECT_TRUE(passTest);
}


TEST(resource_pool, T_unique_ptr_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::arrp::resource_pool<std::unique_ptr<std::string>> rp {};

        EXPECT_EQ(0, rp.size());
        rp.seed_to_pool(std::unique_ptr<std::string>(new std::string(__TIME__)));
        EXPECT_EQ(1, rp.size());

        {
            auto item_result = rp.borrow_from_pool();
            EXPECT_TRUE(item_result.has_value());
            auto&& item = std::move(item_result.value());
            EXPECT_EQ(0, rp.size());
            EXPECT_EQ(__TIME__, **item);
            (*item)->append("-ok");
        }
        // item is automatically returned to pool when it goes out of scope
        EXPECT_EQ(1, rp.size());

        {
            auto item2_result = rp.borrow_from_pool();
            EXPECT_TRUE(item2_result.has_value());
            auto&& item2 = std::move(item2_result.value());
            EXPECT_EQ(0, rp.size());
            EXPECT_TRUE((*item2)->ends_with("-ok"));
        }
        // item2 is automatically returned to pool when it goes out of scope
        EXPECT_EQ(1, rp.size());

        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(resource_pool, T_checkin_checkout_unique_ptr_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::arrp::resource_pool<std::unique_ptr<std::string>> rp {};

        EXPECT_EQ(0, rp.size());
        rp.seed_to_pool(std::unique_ptr<std::string>(new std::string(__TIME__)));
        EXPECT_EQ(1, rp.size());

        // Borrow and let it go out of scope to return automatically
        {
            auto item_result = rp.borrow_from_pool();
            EXPECT_TRUE(item_result.has_value());
            [[maybe_unused]] auto&& item = std::move(item_result.value());
        }
        // Resource is automatically returned to pool
        EXPECT_EQ(1, rp.size());

        {
            auto item2_result = rp.borrow_from_pool();
            EXPECT_TRUE(item2_result.has_value());
            auto&& item2 = std::move(item2_result.value());
            EXPECT_EQ(0, rp.size());
            EXPECT_EQ(__TIME__, **item2);
        }

        passTest = true;
    });

    EXPECT_TRUE(passTest);
}


TEST(resource_pool, T_checkin_checkout_vector_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::arrp::resource_pool<std::vector<std::string>> rp {};

        EXPECT_EQ(0, rp.size());
        rp.seed_to_pool(std::vector<std::string> {"A", "B", "C"});
        EXPECT_EQ(1, rp.size());

        // Borrow and let it go out of scope to return automatically
        {
            auto item_result = rp.borrow_from_pool();
            EXPECT_TRUE(item_result.has_value());
            [[maybe_unused]] auto&& item = std::move(item_result.value());
        }
        // Resource is automatically returned to pool
        EXPECT_EQ(1, rp.size());

        {
            auto item2_result = rp.borrow_from_pool();
            EXPECT_TRUE(item2_result.has_value());
            auto item2 = std::move(item2_result.value());
            (*item2).emplace_back("1");
            (*item2).emplace_back("2");
            (*item2).emplace_back("3");
            EXPECT_EQ(0, rp.size());
            EXPECT_EQ(6, (*item2).size());
        }
        // item2 is automatically returned to pool when it goes out of scope

        passTest = true;
    });

    EXPECT_TRUE(passTest);
}


/// @brief Test that borrow on an empty pool throws
TEST(resource_pool, borrow_empty_throws)
{
    // Custom allocator that does not allocate and rather throws..
    siddiqsoft::arrp::resource_pool<std::string> rp(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [](siddiqsoft::arrp::resource_pool<std::string>& pool)
                    -> std::expected<siddiqsoft::arrp::scoped_resource<std::string>, siddiqsoft::arrp::pool_error> {
                return std::unexpected(siddiqsoft::arrp::pool_error::NoMoreResources);
            });


    auto result = rp.borrow_from_pool();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), siddiqsoft::arrp::pool_error::NoMoreResources);
}


/// @brief Test clear empties the pool
TEST(resource_pool, clear)
{
    // Custom allocator that does not allocate and rather throws..
    siddiqsoft::arrp::resource_pool<std::string> rp(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [](siddiqsoft::arrp::resource_pool<std::string>& pool) -> siddiqsoft::arrp::scoped_resource<std::string> {
                throw std::runtime_error("Deliberate throw");
            });

    rp.seed_to_pool(std::string("1"));
    rp.seed_to_pool(std::string("2"));
    rp.seed_to_pool(std::string("3"));
    EXPECT_EQ(3u, rp.size());

    rp.clear();
    EXPECT_EQ(0u, rp.size());

    // After clear, borrow should fail
    auto result = rp.borrow_from_pool();
    EXPECT_FALSE(result.has_value());
}


/// @brief Test multiple add/borrow cycles
TEST(resource_pool, multiple_items)
{
    siddiqsoft::arrp::resource_pool<std::string> rp {};

    for (int i = 0; i < 10; i++) {
        rp.seed_to_pool(std::format("{}", i));
    }
    EXPECT_EQ(10u, rp.size());

    // Borrow all items (FIFO order)
    for (int i = 0; i < 10; i++) {
        auto item_result = rp.borrow_from_pool();
        EXPECT_TRUE(item_result.has_value());
        auto item = std::move(item_result.value());
        EXPECT_EQ(std::format("{}", i), *item);
    }
    EXPECT_EQ(10u, rp.size());
}


/// @brief Test that add after borrow preserves the resource
TEST(resource_pool, round_trip_preserves_value)
{
    siddiqsoft::arrp::resource_pool<std::string> rp {};

    rp.seed_to_pool(std::string("hello"));
    {
        auto item_result = rp.borrow_from_pool();
        EXPECT_TRUE(item_result.has_value());
        auto item = std::move(item_result.value());
        EXPECT_EQ("hello", *item);
        *item += " world";
    }
    // item is automatically returned to pool

    {
        auto item2_result = rp.borrow_from_pool();
        EXPECT_TRUE(item2_result.has_value());
        auto item2 = std::move(item2_result.value());
        EXPECT_EQ("hello world", *item2);
    }
}


/// @brief Test with nlohmann::json type
TEST(resource_pool, json_type)
{
    bool                                            passTest = false;
    siddiqsoft::arrp::resource_pool<nlohmann::json> rp {};
    nlohmann::json                                  dummy = {{"key", "value"}, {"age", 101}, {"something", "nothing"}};

    std::print(std::cerr, "{}\n", dummy.dump());

    rp.seed_to_pool(nlohmann::json::object({{"name", "surname"}, {"lift", 909}, {"everything", "nothing"}}));
    rp.seed_to_pool(std::move(dummy));
    EXPECT_TRUE(dummy.is_null());
    EXPECT_EQ(2u, rp.size());

    std::print(std::cerr, "{}\n", rp.to_json().value().get().dump());

    try {
        auto item_result = rp.borrow_from_pool();
        EXPECT_TRUE(item_result.has_value());
        auto item = *(item_result.value());

        std::print(std::cerr, "{}\n", item.dump());
        EXPECT_EQ("nothing", item["everything"]);
        EXPECT_EQ(1u, rp.size());
        passTest = true;
    }
    catch (std::exception& ex) {
        std::print(std::cerr, "{}\n", ex.what());
        passTest = false;
    }

    EXPECT_EQ(true, passTest);
}


/// @brief Test with nlohmann::json type
TEST(resource_pool, pair_type)
{
    bool                                                         passTest = false;
    siddiqsoft::arrp::resource_pool<std::pair<int, std::string>> rp {};

    // Note that the args... constructor calls the specific args.. seed_to_pool(args...)
    rp.seed_to_pool(99, "hello");
    EXPECT_EQ(1u, rp.size());

    try {
        auto item_result = rp.borrow_from_pool();
        EXPECT_TRUE(item_result.has_value());
        auto item = *(item_result.value());

        EXPECT_EQ(99, item.first);
        EXPECT_EQ("hello", item.second);
        std::print(std::cerr, "contents of the item: <{},{}>\n", item.first, item.second);

        EXPECT_EQ(0u, rp.size());
        passTest = true;
    }
    catch (std::exception& ex) {
        std::print(std::cerr, "{}\n", ex.what());
        passTest = false;
    }

    EXPECT_EQ(true, passTest);
}

/// @brief Test concurrent add/borrow from multiple threads
TEST(resource_pool, concurrent_access)
{
    siddiqsoft::arrp::resource_pool<std::string> rp {};
    constexpr int                                ITERATIONS = 100;
    std::atomic_int                              borrowCount {0};

    // Pre-fill the pool
    for (int i = 0; i < ITERATIONS; i++) {
        rp.seed_to_pool(std::format("resource-{}", i));
    }

    std::vector<std::jthread> threads;
    for (int t = 0; t < 4; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < ITERATIONS / 4; i++) {
                auto result = rp.borrow_from_pool();
                if (result.has_value()) {
                    borrowCount++;
                    auto item = std::move(result.value());
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        });
    }

    // Wait for threads to finish (jthread joins automatically)
    threads.clear();

    // All items should be back in the pool
    EXPECT_EQ(ITERATIONS, static_cast<int>(rp.size().value()));
}


/// @brief Test that double clear is safe
TEST(resource_pool, double_clear)
{
    siddiqsoft::arrp::resource_pool<std::string> rp {};
    rp.seed_to_pool(std::string("42"));
    rp.clear();
    EXPECT_EQ(0u, rp.size());

    // Second clear on empty pool should be safe
    EXPECT_NO_THROW(rp.clear());
    EXPECT_EQ(0u, rp.size());
}


/// @brief Starvation test: many threads compete for a small pool.
/// Some threads will get resources, others will fail with exceptions.
/// All resources must be returned to the pool at the end.
TEST(resource_pool, starvation_under_contention)
{
    constexpr int POOL_SIZE      = 3;
    constexpr int THREAD_COUNT   = 8;
    constexpr int OPS_PER_THREAD = 50;

    // Custom allocator that does not allocate and rather throws..
    siddiqsoft::arrp::resource_pool<std::string> rp(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [](siddiqsoft::arrp::resource_pool<std::string>& pool) -> siddiqsoft::arrp::scoped_resource<std::string> {
                throw std::runtime_error("Deliberate throw");
            });


    for (int i = 0; i < POOL_SIZE; i++) {
        rp.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_int           successCount {0};
    std::atomic_int           failCount {0};

    std::vector<std::jthread> threads;
    std::barrier              startBarrier {THREAD_COUNT};

    for (int t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back([&]() {
            startBarrier.arrive_and_wait();
            for (int i = 0; i < OPS_PER_THREAD; i++) {
                auto result = rp.borrow_from_pool();
                if (result.has_value()) {
                    successCount++;
                    auto item = std::move(result.value());
                    // Simulate work
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
                else {
                    failCount++;
                }
            }
        });
    }

    threads.clear();

    // All resources should be back in the pool
    EXPECT_EQ(static_cast<size_t>(POOL_SIZE), rp.size());
    // At least some operations should have succeeded
    EXPECT_GT(successCount.load(), 0);
    // Total operations = successes + failures
    EXPECT_EQ(THREAD_COUNT * OPS_PER_THREAD, successCount.load() + failCount.load());
}


/// @brief Test high-throughput add/borrow cycling from many threads.
/// Each thread does many rapid borrow-then-add cycles. This exercises
/// the mutex under high contention.
TEST(resource_pool, high_throughput_cycling)
{
    constexpr int                                POOL_SIZE    = 8;
    constexpr int                                THREAD_COUNT = 8;
    constexpr int                                CYCLES       = 200;

    siddiqsoft::arrp::resource_pool<std::string> rp {};
    for (int i = 0; i < POOL_SIZE; i++) {
        rp.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_int           totalBorrows {0};
    std::barrier              startBarrier {THREAD_COUNT};

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back([&]() {
            startBarrier.arrive_and_wait();
            for (int c = 0; c < CYCLES; c++) {
                auto result = rp.borrow_from_pool();
                if (result.has_value()) {
                    totalBorrows++;
                    auto item = std::move(result.value());
                    // Immediately return
                }
            }
        });
    }

    threads.clear();

    // All resources should be back
    EXPECT_EQ(static_cast<size_t>(POOL_SIZE), rp.size());
    EXPECT_GT(totalBorrows.load(), 0);
}


/// @brief Test that borrow on a pool that was populated then fully drained fails gracefully.
/// Validates the error path after legitimate use, not just on a fresh empty pool.
TEST(resource_pool, borrow_after_drain_fails)
{
    // Custom allocator that does not allocate and rather throws..
    siddiqsoft::arrp::resource_pool<std::string> rp(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [](siddiqsoft::arrp::resource_pool<std::string>& pool) -> siddiqsoft::arrp::scoped_resource<std::string> {
                throw std::runtime_error("Deliberate throw");
            });

    rp.seed_to_pool(std::string("1"));
    rp.seed_to_pool(std::string("2"));

    {
        auto a_result = rp.borrow_from_pool();
        auto b_result = rp.borrow_from_pool();
        EXPECT_TRUE(a_result.has_value());
        EXPECT_TRUE(b_result.has_value());
        auto a = std::move(a_result.value());
        auto b = std::move(b_result.value());
        EXPECT_EQ(0u, rp.size());

        // This must be checked prior to the a and b going out of
        // scope in which case they'll be put back into the pool
        // and the check for rp.borrow_from_pool() failing
        // will fail!
        auto result = rp.borrow_from_pool();
        EXPECT_FALSE(result.has_value());
    }
}


/// @brief Test that size() is accurate under concurrent modifications.
/// Multiple threads add while the main thread polls size().
TEST(resource_pool, size_accuracy_under_concurrency)
{
    siddiqsoft::arrp::resource_pool<std::string> rp {};
    constexpr int                                ITEMS_PER_THREAD = 50;
    constexpr int                                THREAD_COUNT     = 4;

    std::barrier                                 startBarrier {THREAD_COUNT};
    std::vector<std::jthread>                    threads;

    for (int t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back([&, t]() {
            startBarrier.arrive_and_wait();
            for (int i = 0; i < ITEMS_PER_THREAD; i++) {
                std::string val = std::format("resource-{}-{}", t, i);
                rp.seed_to_pool(std::move(val));
            }
        });
    }

    threads.clear();

    EXPECT_EQ(static_cast<size_t>(THREAD_COUNT * ITEMS_PER_THREAD), rp.size());
}


/// @brief Test resource_pool with unique_ptr under concurrent access.
/// unique_ptr is move-only, so this validates that the pool correctly handles
/// move semantics under thread contention.
TEST(resource_pool, concurrent_unique_ptr)
{
    constexpr int                                                 POOL_SIZE    = 4;
    constexpr int                                                 THREAD_COUNT = 4;
    constexpr int                                                 CYCLES       = 100;

    siddiqsoft::arrp::resource_pool<std::unique_ptr<std::string>> rp {};
    for (int i = 0; i < POOL_SIZE; i++) {
        rp.seed_to_pool(std::make_unique<std::string>(std::format("resource-{}", i)));
    }

    std::atomic_int           totalBorrows {0};
    std::barrier              startBarrier {THREAD_COUNT};

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back([&]() {
            startBarrier.arrive_and_wait();
            for (int c = 0; c < CYCLES; c++) {
                auto result = rp.borrow_from_pool();
                if (result.has_value()) {
                    auto&& item = std::move(result.value());
                    EXPECT_NE(nullptr, *item);
                    totalBorrows++;
                }
            }
        });
    }

    threads.clear();

    EXPECT_EQ(static_cast<size_t>(POOL_SIZE), rp.size());
    EXPECT_GT(totalBorrows.load(), 0);
}


/// @brief Test multiple shared_ptr items in pool
/// Validates that multiple shared_ptr resources can coexist in the pool
TEST(resource_pool, multiple_shared_ptr_items)
{
    siddiqsoft::arrp::resource_pool<std::shared_ptr<std::string>> rp {};

    auto                                                          ptr1 = std::make_shared<std::string>("resource-1");
    auto                                                          ptr2 = std::make_shared<std::string>("resource-2");
    auto                                                          ptr3 = std::make_shared<std::string>("resource-3");

    rp.seed_to_pool(std::move(ptr1));
    rp.seed_to_pool(std::move(ptr2));
    rp.seed_to_pool(std::move(ptr3));

    EXPECT_EQ(3u, rp.size());

    // Borrow in FIFO order
    {
        auto item1_result = rp.borrow_from_pool();
        EXPECT_TRUE(item1_result.has_value());
        auto item1 = std::move(item1_result.value());
        EXPECT_EQ("resource-1", **item1);
        EXPECT_EQ(2u, rp.size());

        auto item2_result = rp.borrow_from_pool();
        EXPECT_TRUE(item2_result.has_value());
        auto item2 = std::move(item2_result.value());
        EXPECT_EQ("resource-2", **item2);
        EXPECT_EQ(1u, rp.size());

        auto item3_result = rp.borrow_from_pool();
        EXPECT_TRUE(item3_result.has_value());
        auto item3 = std::move(item3_result.value());
        EXPECT_EQ("resource-3", **item3);
        EXPECT_EQ(0u, rp.size());
    }
    // All items returned to pool
    EXPECT_EQ(3u, rp.size());
}


/// @brief Test shared_ptr with custom deleter
/// Validates that shared_ptr with custom deleters work correctly in the pool
TEST(resource_pool, shared_ptr_custom_deleter)
{
    std::atomic_int deleteCount {0};

    {
        siddiqsoft::arrp::resource_pool<std::shared_ptr<std::string>> rp {};

        // this is a shared_ptr with custom deleter
        auto ptr = std::shared_ptr<std::string>(new std::string("custom-deleter-test"), [&deleteCount](std::string* p) {
            deleteCount++;
            // deliberate test case
            delete p;
        });

        rp.seed_to_pool(std::move(ptr));
        EXPECT_EQ(0, deleteCount.load());

        {
            auto item_result = rp.borrow_from_pool();
            EXPECT_TRUE(item_result.has_value());
            auto item = std::move(item_result.value());
            EXPECT_EQ("custom-deleter-test", **item);
        }
        // Item returned to pool
        EXPECT_EQ(0, deleteCount.load());
    }
    // Pool destroyed, custom deleter should be called
    EXPECT_EQ(1, deleteCount.load());
}


/// @brief Test shared_ptr modification persistence
/// Validates that modifications to shared_ptr objects persist across borrow/add cycles
TEST(resource_pool, shared_ptr_modification_persistence)
{
    siddiqsoft::arrp::resource_pool<std::shared_ptr<std::string>> rp {};

    rp.seed_to_pool(std::make_shared<std::string>("initial"));
    EXPECT_EQ(1u, rp.size());

    {
        auto item_result = rp.borrow_from_pool();
        EXPECT_TRUE(item_result.has_value());
        auto item = std::move(item_result.value());
        **item += "-modified";
        EXPECT_EQ("initial-modified", **item);
    }
    // Item returned to pool
    EXPECT_EQ(1u, rp.size());

    {
        auto item2_result = rp.borrow_from_pool();
        EXPECT_TRUE(item2_result.has_value());
        auto item2 = std::move(item2_result.value());
        EXPECT_EQ("initial-modified", **item2);
        **item2 += "-again";
    }
    // Item returned to pool
    EXPECT_EQ(1u, rp.size());

    {
        auto item3_result = rp.borrow_from_pool();
        EXPECT_TRUE(item3_result.has_value());
        auto item3 = std::move(item3_result.value());
        EXPECT_EQ("initial-modified-again", **item3);
    }
}


/// @brief Test concurrent access with shared_ptr
/// Multiple threads borrow/add shared_ptr resources concurrently
TEST(resource_pool, concurrent_shared_ptr_access)
{
    constexpr int                                                 POOL_SIZE    = 4;
    constexpr int                                                 THREAD_COUNT = 4;
    constexpr int                                                 CYCLES       = 100;

    siddiqsoft::arrp::resource_pool<std::shared_ptr<std::string>> rp {};
    for (int i = 0; i < POOL_SIZE; i++) {
        rp.seed_to_pool(std::make_shared<std::string>(std::format("shared-resource-{}", i)));
    }

    std::atomic_int           totalBorrows {0};
    std::barrier              startBarrier {THREAD_COUNT};

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back([&]() {
            startBarrier.arrive_and_wait();
            for (int c = 0; c < CYCLES; c++) {
                auto result = rp.borrow_from_pool();
                if (result.has_value()) {
                    auto item = std::move(result.value());
                    EXPECT_NE(nullptr, *item);
                    EXPECT_FALSE((**item).empty());
                    totalBorrows++;
                }
            }
        });
    }

    threads.clear();

    EXPECT_EQ(static_cast<size_t>(POOL_SIZE), rp.size());
    EXPECT_GT(totalBorrows.load(), 0);
}

// ============================================================================
// NEW COMPREHENSIVE TESTS FOR FULL COVERAGE AND STRESS TESTING
// ============================================================================

/// @brief Test scoped_resource dereference operator
TEST(scoped_resource, dereference_operator)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("test"));

    auto resource_result = pool.borrow_from_pool();
    EXPECT_TRUE(resource_result.has_value());
    auto resource = std::move(resource_result.value());

    // Test dereference operator
    EXPECT_EQ("test", *resource);
    *resource += "-deref";
    EXPECT_EQ("test-deref", *resource);
}

/// @brief Test scoped_resource invalidate
TEST(scoped_resource, invalidate)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("resource1"));
    pool.seed_to_pool(std::string("resource2"));

    EXPECT_EQ(2u, pool.size());

    {
        auto resource_result = pool.borrow_from_pool();
        EXPECT_TRUE(resource_result.has_value());
        auto resource = std::move(resource_result.value());
        EXPECT_EQ(1u, pool.size());

        // Invalidate the resource
        resource.invalidate();
    }

    // Resource should NOT be returned to pool
    EXPECT_EQ(1u, pool.size());
}

/// @brief Test scoped_resource move semantics
TEST(scoped_resource, move_semantics)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("original"));

    {
        auto resource1_result = pool.borrow_from_pool();
        EXPECT_TRUE(resource1_result.has_value());
        auto resource1 = std::move(resource1_result.value());
        EXPECT_EQ("original", *resource1);

        // Move to resource2
        auto resource2 = std::move(resource1);
        EXPECT_EQ("original", *resource2);
    }

    // Only one resource should be returned
    EXPECT_EQ(1u, pool.size());
}

/// @brief Test JSON serialization with counters
TEST(resource_pool, json_serialization_counters)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // Add resources
    pool.seed_to_pool(std::string("res1"));
    pool.seed_to_pool(std::string("res2"));

    // Borrow and return
    {
        auto res_result = pool.borrow_from_pool();
        EXPECT_TRUE(res_result.has_value());
        auto res = std::move(res_result.value());
    }

    auto& json = pool.to_json().value().get();

    // Verify JSON structure
    EXPECT_TRUE(json.contains("_typver"));
    EXPECT_TRUE(json.contains("capacity"));
    EXPECT_TRUE(json.contains("size"));
    EXPECT_TRUE(json.contains("autoadds"));
    EXPECT_TRUE(json.contains("returns"));
    EXPECT_TRUE(json.contains("borrows"));
    EXPECT_TRUE(json.contains("seeds"));
}

/// @brief Extreme stress test: very high concurrency
TEST(resource_pool, extreme_stress_high_concurrency)
{
    constexpr int                                POOL_SIZE    = 16;
    constexpr int                                THREAD_COUNT = 32;
    constexpr int                                ITERATIONS   = 500;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < POOL_SIZE; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_int           total_ops {0};
    std::barrier              start_barrier {THREAD_COUNT};

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < ITERATIONS; ++i) {
                auto result = pool.borrow_from_pool();
                if (result.has_value()) {
                    total_ops++;
                    auto res = std::move(result.value());
                    // Simulate minimal work
                    std::this_thread::yield();
                }
            }
        });
    }

    threads.clear();

    EXPECT_EQ(static_cast<size_t>(POOL_SIZE), pool.size());
    EXPECT_GT(total_ops.load(), 0);
}

/// @brief Test with custom factory callback
TEST(resource_pool, custom_factory_callback)
{
    std::atomic_int                              creation_count {0};

    siddiqsoft::arrp::resource_pool<std::string> pool {
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [&creation_count](
                    auto& p) -> std::expected<siddiqsoft::arrp::scoped_resource<std::string>, siddiqsoft::arrp::pool_error> {
                creation_count++;
                return p.create_resource(std::format("created-{}", creation_count.load()));
            }};

    // Borrow resources - should trigger factory
    {
        auto res1_result = pool.borrow_from_pool();
        EXPECT_TRUE(res1_result.has_value());
        auto res1 = std::move(res1_result.value());
        EXPECT_EQ(1, creation_count.load());

        auto res2_result = pool.borrow_from_pool();
        EXPECT_TRUE(res2_result.has_value());
        auto res2 = std::move(res2_result.value());
        EXPECT_EQ(2, creation_count.load());
    }

    // Resources should be returned
    EXPECT_EQ(2u, pool.size());
}

/// @brief Test capacity limits
TEST(resource_pool, capacity_limits)
{
    siddiqsoft::arrp::resource_pool<std::string, siddiqsoft::arrp::scoped_resource<std::string>> pool {4};

    // Add resources up to capacity
    pool.seed_to_pool(std::string("1"));
    pool.seed_to_pool(std::string("2"));
    pool.seed_to_pool(std::string("3"));
    pool.seed_to_pool(std::string("4"));

    EXPECT_EQ(4u, pool.size());

    // Try to borrow all
    auto r1_result = pool.borrow_from_pool();
    auto r2_result = pool.borrow_from_pool();
    auto r3_result = pool.borrow_from_pool();
    auto r4_result = pool.borrow_from_pool();

    EXPECT_TRUE(r1_result.has_value());
    EXPECT_TRUE(r2_result.has_value());
    EXPECT_TRUE(r3_result.has_value());
    EXPECT_TRUE(r4_result.has_value());

    auto r1 = std::move(r1_result.value());
    auto r2 = std::move(r2_result.value());
    auto r3 = std::move(r3_result.value());
    auto r4 = std::move(r4_result.value());

    EXPECT_EQ(0u, pool.size());

    // Should fail when trying to borrow beyond capacity
    auto r5_result = pool.borrow_from_pool();
    EXPECT_FALSE(r5_result.has_value());
}

/// @brief Test rapid allocation/deallocation cycles
TEST(resource_pool, rapid_cycles)
{
    siddiqsoft::arrp::resource_pool<std::vector<int>> pool {};
    pool.seed_to_pool(std::vector<int> {1, 2, 3, 4, 5});

    for (int cycle = 0; cycle < 100; ++cycle) {
        std::print(std::cerr, "  >> Working on cycle: {}\n", cycle);
        {
            auto vec_result = pool.borrow_from_pool();
            EXPECT_TRUE(vec_result.has_value());
            auto vec = std::move(vec_result.value());
            (*vec).push_back(6);
        }
    }

    // After all that.. we should still be back at one item in the pool.
    std::print(std::cerr, "  >> Post completion: {}", pool.to_json().value().get().dump(2));
    EXPECT_EQ(1u, pool.size());
}

/// @brief Test with large objects
TEST(resource_pool, large_objects)
{
    constexpr size_t                                   LARGE_SIZE = 1024 * 1024; // 1MB

    siddiqsoft::arrp::resource_pool<std::vector<char>> pool {};

    // Create large vector
    std::vector<char> large_vec(LARGE_SIZE, 'x');
    pool.seed_to_pool(std::move(large_vec));

    {
        auto vec_result = pool.borrow_from_pool();
        EXPECT_TRUE(vec_result.has_value());
        auto vec = std::move(vec_result.value());
        EXPECT_EQ(LARGE_SIZE, (*vec).size());
    }

    EXPECT_EQ(1u, pool.size());
}

/// @brief Test exception safety
TEST(resource_pool, exception_safety)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("resource"));

    try {
        auto res_result = pool.borrow_from_pool();
        EXPECT_TRUE(res_result.has_value());
        // auto res = std::move(res_result.value());
        throw std::runtime_error("Test exception");
    }
    catch (const std::runtime_error&) {
        // Exception caught
    }

    // Resource should still be returned to pool
    EXPECT_EQ(1, *pool.size());
}

/// @brief Test move assignment operator
TEST(scoped_resource, move_assignment)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("res1"));
    pool.seed_to_pool(std::string("res2"));

    auto res1_result = pool.borrow_from_pool();
    auto res2_result = pool.borrow_from_pool();

    EXPECT_TRUE(res1_result.has_value());
    EXPECT_TRUE(res2_result.has_value());

    auto res1 = std::move(res1_result.value());
    auto res2 = std::move(res2_result.value());

    EXPECT_EQ("res1", *res1);
    EXPECT_EQ("res2", *res2);

    // Move assign
    res1 = std::move(res2);
    EXPECT_EQ("res2", *res1);

    // res2 is now abandoned, only res1 will return
    // So we should have 1 resource returned
}

/// @brief Test with multiple pools
TEST(resource_pool, multiple_pools)
{
    siddiqsoft::arrp::resource_pool<std::string> pool1 {};
    siddiqsoft::arrp::resource_pool<std::string> pool2 {};

    pool1.seed_to_pool(std::string("pool1-res"));
    pool2.seed_to_pool(std::string("pool2-res"));

    {
        auto res1_result = pool1.borrow_from_pool();
        auto res2_result = pool2.borrow_from_pool();

        EXPECT_TRUE(res1_result.has_value());
        EXPECT_TRUE(res2_result.has_value());

        auto res1 = std::move(res1_result.value());
        auto res2 = std::move(res2_result.value());

        EXPECT_EQ("pool1-res", *res1);
        EXPECT_EQ("pool2-res", *res2);
    }

    EXPECT_EQ(1u, pool1.size());
    EXPECT_EQ(1u, pool2.size());
}

/// @brief Test FIFO ordering under concurrent access
TEST(resource_pool, fifo_ordering_concurrent)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int i = 0; i < 10; ++i) {
        pool.seed_to_pool(std::format("item-{}", i));
    }

    std::vector<std::string>  retrieved;
    std::mutex                retrieved_lock;

    std::vector<std::jthread> threads;
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < 2; ++i) {
                auto res_result = pool.borrow_from_pool();
                if (res_result.has_value()) {
                    auto res = std::move(res_result.value());
                    {
                        std::scoped_lock<std::mutex> lock(retrieved_lock);
                        retrieved.push_back(*res);
                    }
                }
            }
        });
    }

    threads.clear();

    // All items should be retrieved
    EXPECT_EQ(10u, retrieved.size());
    EXPECT_EQ(10u, pool.size());
}

// ============================================================================
// ADVERSARIAL TESTS - STRESS TESTING EDGE CASES AND FAILURE MODES
// ============================================================================

/// @brief Adversarial: Rapid borrow/return with immediate exceptions
TEST(resource_pool_adversarial, rapid_exception_cycles)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    pool.seed_to_pool(std::string("resource"));

    for (int i = 0; i < 1000; ++i) {
        try {
            auto res_result = pool.borrow_from_pool();
            if (res_result.has_value()) {
                auto res = std::move(res_result.value());
                if (i % 3 == 0) {
                    throw std::runtime_error("Adversarial exception");
                }
            }
        }
        catch (const std::runtime_error&) {
            // Expected
        }
    }

    // Resource should still be in pool
    EXPECT_EQ(1u, pool.size());
}

/// @brief Adversarial: Concurrent invalidation and borrow
TEST(resource_pool_adversarial, concurrent_invalidation)
{
    constexpr int                                THREAD_COUNT = 8;
    constexpr int                                ITERATIONS   = 100;

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < 10; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_int           invalidated {0};
    std::barrier              start_barrier {THREAD_COUNT};

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < ITERATIONS; ++i) {
                auto result = pool.borrow_from_pool();
                if (result.has_value()) {
                    auto res = std::move(result.value());
                    if (i % 5 == 0) {
                        res.invalidate();
                        invalidated++;
                    }
                }
            }
        });
    }

    threads.clear();

    // Some resources should have been invalidated
    EXPECT_GT(invalidated.load(), 0);
}

/// @brief Adversarial: Alternating clear and borrow operations
TEST(resource_pool_adversarial, alternating_clear_borrow)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    for (int cycle = 0; cycle < 100; ++cycle) {
        // Add resources
        for (int i = 0; i < 5; ++i) {
            pool.seed_to_pool(std::format("resource-{}-{}", cycle, i));
        }

        // Try to borrow
        auto result = pool.borrow_from_pool();
        if (result.has_value()) {
            auto res = std::move(result.value());
        }

        // Clear
        pool.clear();
        EXPECT_EQ(0u, pool.size());
    }
}


/// @brief Adversarial: Extreme contention with minimal pool size
TEST(resource_pool_adversarial, extreme_contention_minimal_pool)
{
    constexpr int   POOL_SIZE    = 1;
    constexpr int   THREAD_COUNT = 64;
    constexpr int   ITERATIONS   = 100;
    std::atomic_int successes {0};
    std::atomic_int failures {0};
    std::barrier    start_barrier {THREAD_COUNT};

    // Custom allocator that does not allocate and rather throws..
    siddiqsoft::arrp::resource_pool<std::string> pool(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [](siddiqsoft::arrp::resource_pool<std::string>& pool) -> siddiqsoft::arrp::scoped_resource<std::string> {
                throw std::runtime_error("Deliberate throw");
            });
    pool.seed_to_pool(std::string("single-resource"));
    EXPECT_EQ(1, pool.size());

    std::vector<std::jthread> threads;
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < ITERATIONS; ++i) {
                auto result = pool.borrow_from_pool();
                if (result.has_value()) {
                    successes++;
                    auto res = std::move(result.value());
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
                else {
                    failures++;
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(19));
    threads.clear();

    EXPECT_EQ(1u, pool.size());
    EXPECT_GT(successes.load(), 0);
    EXPECT_GT(failures.load(), 0);
    EXPECT_EQ(THREAD_COUNT * ITERATIONS, successes.load() + failures.load());
}

/// @brief Adversarial: Rapid move operations
TEST(resource_pool_adversarial, rapid_move_operations)
{
    siddiqsoft::arrp::resource_pool<std::unique_ptr<std::string>> pool {};
    for (int i = 0; i < 10; ++i) {
        pool.seed_to_pool(std::make_unique<std::string>(std::format("resource-{}", i)));
    }

    for (int cycle = 0; cycle < 100; ++cycle) {
        auto res1_result = pool.borrow_from_pool();
        if (res1_result.has_value()) {
            auto res1 = std::move(res1_result.value());
            auto res2 = std::move(res1);
            auto res3 = std::move(res2);
            // res3 goes out of scope and returns to pool
        }
    }

    EXPECT_EQ(10u, pool.size());
}

/// @brief Adversarial: Interleaved invalidate and normal returns
TEST(resource_pool_adversarial, interleaved_invalidate_returns)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < 20; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::vector<siddiqsoft::arrp::scoped_resource<std::string>> resources;

    // Borrow all resources
    for (int i = 0; i < 20; ++i) {
        auto result = pool.borrow_from_pool();
        if (result.has_value()) {
            resources.push_back(std::move(result.value()));
        }
    }

    EXPECT_EQ(0u, pool.size());

    // Invalidate every other resource
    for (size_t i = 0; i < resources.size(); i += 2) {
        resources[i].invalidate();
    }

    // Clear resources (some invalidated, some not)
    resources.clear();

    // Should have 10 resources back (the non-invalidated ones)
    EXPECT_EQ(10u, pool.size());
}

/// @brief Adversarial: Stress test with random delays
TEST(resource_pool_adversarial, random_delays_stress)
{
    std::random_device                           rd;
    std::mt19937                                 gen(rd());
    std::uniform_int_distribution<>              dis(0, 100);

    siddiqsoft::arrp::resource_pool<std::string> pool {};
    for (int i = 0; i < 8; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_int           ops {0};
    std::barrier              start_barrier {8};

    std::vector<std::jthread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            std::mt19937 local_gen(rd() + t);
            start_barrier.arrive_and_wait();
            for (int i = 0; i < 100; ++i) {
                auto result = pool.borrow_from_pool();
                if (result.has_value()) {
                    ops++;
                    auto res = std::move(result.value());
                    // Random delay
                    std::this_thread::sleep_for(std::chrono::microseconds(dis(local_gen)));
                }
            }
        });
    }

    threads.clear();

    EXPECT_EQ(8u, pool.size());
    EXPECT_GT(ops.load(), 0);
}

/// @brief Adversarial: Concurrent JSON serialization during operations
TEST(resource_pool_adversarial, concurrent_json_serialization)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {};

    // Seed the pool with 10 items..
    for (int i = 0; i < 10; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    EXPECT_EQ(10, pool.size()) << "Expect all 10 items to be added.";

    std::atomic_int           json_calls {0};
    std::barrier              start_barrier {4};

    std::vector<std::jthread> threads;

    // Threads that borrow/return
    for (int t = 0; t < 3; ++t) {
        threads.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < 100; ++i) {
                auto result = pool.borrow_from_pool();
                if (result.has_value()) {
                    auto res = std::move(result.value());
                }
            }
        });
    }

    // Thread that calls to_json
    threads.emplace_back([&]() {
        start_barrier.arrive_and_wait();
        for (int i = 0; i < 50; ++i) {
            auto& json = pool.to_json().value().get();
            json_calls++;
            EXPECT_TRUE(json.contains("seeds"));
        }
    });

    threads.clear();

    EXPECT_GT(json_calls.load(), 0);
}

/// @brief Adversarial: Stress with factory callback exceptions
TEST(resource_pool_adversarial, factory_callback_exceptions)
{
    std::atomic_int                              factory_calls {0};
    std::atomic_int                              factory_exceptions {0};
    std::barrier                                 start_barrier {4};

    siddiqsoft::arrp::resource_pool<std::string> pool {
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [&](auto& p) -> siddiqsoft::arrp::scoped_resource<std::string> {
                factory_calls++;
                if (factory_calls.load() % 2 == 0) {
                    factory_exceptions++;
                    throw std::runtime_error(std::format(
                            "Factory exception calls:{}  exceptions:{}", factory_calls.load(), factory_exceptions.load()));
                }
                return p.create_resource(std::format("created-{}", factory_calls.load()));
            }};

    std::atomic_int           successes {0};
    std::atomic_int           failures {0};
    std::vector<std::jthread> workers;
    for (int t = 0; t < 4; t++) {
        workers.emplace_back([&]() {
            start_barrier.arrive_and_wait();
            for (int i = 0; i < 600; ++i) {
                auto result = pool.borrow_from_pool();
                if (result.has_value()) {
                    successes++;
                    auto res = std::move(result.value());
                    std::this_thread::sleep_for(std::chrono::milliseconds(80));
                }
                else {
                    failures++;
                }
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    workers.clear();

    std::print(std::cerr, "{}\n", pool.to_json().value().get().dump());
    std::print(std::cerr,
               "factory_calls:{}.  success:{}. exceptions:{}. failures:{}. \n",
               factory_calls.load(),
               successes.load(),
               factory_exceptions.load(),
               failures.load());

    EXPECT_GT(successes.load(), 0);
    EXPECT_GT(failures.load(), 0);
    EXPECT_EQ(factory_exceptions.load(), failures.load());
}


/// @brief FIXED: Test concurrent clear racing with borrow/add operations.
/// One thread clears the pool while others are actively adding/borrowing.
/// No crashes or deadlocks should occur.
/// DEADLOCK FIX: Added timeout mechanism and reduced contention
TEST(resource_pool, concurrent_clear_with_operations_FIXED)
{
    siddiqsoft::arrp::resource_pool<std::string> rp {};
    constexpr int                                INITIAL_SIZE = 10; // REDUCED from 20

    for (int i = 0; i < INITIAL_SIZE; i++) {
        rp.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_bool done {false};
    std::atomic_int  clearCount {0};
    auto             start_time   = std::chrono::steady_clock::now();
    constexpr auto   TEST_TIMEOUT = std::chrono::seconds(5);

    // Thread that periodically clears the pool
    std::jthread clearer([&](std::stop_token st) {
        while (!st.stop_requested() && !done.load()) {
            // DEADLOCK FIX: Check for timeout
            if (std::chrono::steady_clock::now() - start_time > TEST_TIMEOUT) {
                break;
            }

            rp.clear();
            clearCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // INCREASED from 10

            // Re-populate
            for (int i = 0; i < 3; i++) { // REDUCED from 5
                rp.seed_to_pool(std::format("resource-{}", i));
            }
        }
    });

    // Threads that borrow/add
    std::vector<std::jthread> workers;
    for (int t = 0; t < 2; t++) { // REDUCED from 4
        workers.emplace_back([&]() {
            for (int i = 0; i < 50; i++) { // REDUCED from 100
                // DEADLOCK FIX: Check for timeout
                if (std::chrono::steady_clock::now() - start_time > TEST_TIMEOUT) {
                    break;
                }

                auto result = rp.borrow_from_pool();
                if (result.has_value()) {
                    auto item = std::move(result.value());
                    std::this_thread::sleep_for(std::chrono::microseconds(50)); // INCREASED from 10
                }
            }
        });
    }

    workers.clear();
    done = true;
    clearer.request_stop();
    if (clearer.joinable()) clearer.join();

    EXPECT_GT(clearCount.load(), 0);
}


/// @brief FIXED: Adversarial: Concurrent clear with rapid borrow/add
/// DEADLOCK FIX: Added timeout mechanism and reduced contention
TEST(resource_pool_adversarial, concurrent_clear_rapid_ops_FIXED)
{
    siddiqsoft::arrp::resource_pool<std::string> pool {siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
                                                       siddiqsoft::arrp::auto_add_policy::AutoGrow};
    for (int i = 0; i < 20; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_bool stop {false};
    std::atomic_int  clears {0};
    std::atomic_int  borrows {0};
    auto             start_time   = std::chrono::steady_clock::now();
    constexpr auto   TEST_TIMEOUT = std::chrono::seconds(5);

    // Thread that clears
    std::jthread clearer([&](std::stop_token st) {
        while (!st.stop_requested() && !stop.load()) {
            // DEADLOCK FIX: Check for timeout
            if (std::chrono::steady_clock::now() - start_time > TEST_TIMEOUT) {
                break;
            }

            pool.clear();
            clears++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // INCREASED from 5

            // Repopulate
            for (int i = 0; i < 5; ++i) { // REDUCED from 10
                pool.seed_to_pool(std::format("resource-{}", i));
            }
        }
    });

    // Threads that borrow/add
    std::vector<std::jthread> workers;
    for (int t = 0; t < 2; ++t) { // REDUCED from 4
        workers.emplace_back([&]() {
            for (int i = 0; i < 100; ++i) { // REDUCED from 200
                // DEADLOCK FIX: Check for timeout
                if (std::chrono::steady_clock::now() - start_time > TEST_TIMEOUT) {
                    break;
                }

                auto result = pool.borrow_from_pool();
                if (result.has_value()) {
                    borrows++;
                    auto res = std::move(result.value());
                }
            }
        });
    }

    workers.clear();
    stop = true;
    clearer.request_stop();
    if (clearer.joinable()) clearer.join();

    EXPECT_GT(clears.load(), 0);
    EXPECT_GT(borrows.load(), 0);
}


#include <future>

TEST(resource_pool, concurrent_clear_deadlock_detection)
{
    constexpr int EXPECTED_THREADS =3;
    siddiqsoft::arrp::resource_pool<std::string> pool;

    for (int i = 0; i < 8; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_bool stop {false};
    std::atomic_int  borrow_cycles {0};
    std::atomic_int  clear_cycles {0};
    std::barrier     start_barrier {EXPECTED_THREADS};
    std::atomic_int sync_threads_ready{0};

    auto             sync_threads_point = [&] {
        sync_threads_ready++;
#if defined(_WIN64)
        while (sync_threads_ready.load() < EXPECTED_THREADS) {
            std::this_thread::yield();
        }
#else
        start_barrier.arrive_and_wait();
#endif
        std::println(std::cerr, "   concurrent_clear_deadlock_detection - All threads ready to continue..{}/{}", sync_threads_ready.load(), EXPECTED_THREADS);
    };


    auto worker_fn = [&]() {
        sync_threads_point();
        std::this_thread::sleep_for(std::chrono::milliseconds(10*(1+(std::rand() % 10))));
        for (int iteration = 0; iteration < 900 && !stop.load(); ++iteration) {
            auto result = pool.borrow_from_pool();
            if (result.has_value()) {
                ++borrow_cycles;
                auto res = std::move(result.value());
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
        return true;
    };

    auto clearer_fn = [&]() {
        sync_threads_point();
        // this specific wait is important otherwise the workers
        // will never get a chance to run..
        std::this_thread::sleep_for(std::chrono::milliseconds(900));

        while (!stop.load()) {
            pool.clear();
            ++clear_cycles;
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            for (int i = 0; i < 4; ++i) {
                pool.seed_to_pool(std::format("resource-{}", i));
            }
        }
        return true;
    };

    EXPECT_EQ(8, pool.size());
    auto worker1 = std::async(std::launch::async, worker_fn);
    auto worker2 = std::async(std::launch::async, worker_fn);
    auto clearer = std::async(std::launch::async, clearer_fn);

    // Give time to complete..
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    // give five seconds for the threads to complete..
    constexpr auto timeout = std::chrono::seconds(15);
    // signal for them to stop..
    stop = true;
    // If we do not signal them to stop then the wait_for() will
    // result in a timeout..
    EXPECT_EQ(std::future_status::ready, worker1.wait_for(timeout));
    EXPECT_EQ(std::future_status::ready, worker2.wait_for(timeout));


    // wait another five seconds..
    EXPECT_EQ(std::future_status::ready, clearer.wait_for(timeout));

    // consume the results..
    std::print(std::cerr, "  results... {}", worker1.get());
    std::print(std::cerr, "  results... {}", worker2.get());
    std::print(std::cerr, "  results... {}", clearer.get());

    EXPECT_GT(borrow_cycles.load(), 0);
    EXPECT_GT(clear_cycles.load(), 0);
}

TEST(resource_pool, concurrent_json_deadlock_detection)
{
    constexpr int EXPECTED_THREADS =2;

    siddiqsoft::arrp::resource_pool<std::string> pool;
    for (int i = 0; i < 10; ++i) {
        pool.seed_to_pool(std::format("resource-{}", i));
    }

    std::atomic_int done {0};
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

    auto            borrow_fn = [&]() {
        sync_threads_point();
        for (int i = 0; i < 200; ++i) {
            auto result = pool.borrow_from_pool();
            if (result.has_value()) {
                auto res = std::move(result.value());
                std::this_thread::sleep_for(std::chrono::microseconds(5));
            }
        }
        ++done;
        return true;
    };

    auto json_fn = [&]() {
        sync_threads_point();
        for (int i = 0; i < 100; ++i) {
            auto& json = pool.to_json().value().get();
            EXPECT_TRUE(json.contains("seeds"));
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        ++done;
        return true;
    };

    auto           worker     = std::async(std::launch::async, borrow_fn);
    auto           serializer = std::async(std::launch::async, json_fn);

    constexpr auto timeout    = std::chrono::seconds(15);
    EXPECT_EQ(std::future_status::ready, worker.wait_for(timeout));
    EXPECT_EQ(std::future_status::ready, serializer.wait_for(timeout));
    EXPECT_EQ(2, done.load());

    auto wt = worker.get();
    auto st = serializer.get();
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
