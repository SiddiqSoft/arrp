/*
    aarp
    Auto returning resource pool

    BSD 3-Clause License

    Copyright (c) 2021, Siddiq Software LLC
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

#include <chrono>
#include <exception>
#include <iostream>
#include <format>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <barrier>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <filesystem>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/private/resource_pool.hpp"

// Helper function to get a platform-independent temporary file path
inline std::string get_temp_file_path(const std::string& filename)
{
    auto temp_dir  = std::filesystem::temp_directory_path();
    auto temp_file = temp_dir / filename;
    return temp_file.string();
}

// Helper function to safely remove a file
inline void safe_remove_file(const std::string& filepath)
{
    std::error_code ec;
    std::filesystem::remove(filepath, ec);
}

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

/**
 * @brief Test basic FILE* resource pool creation and usage
 *
 * Demonstrates creating a resource pool with FILE* handles and
 * borrowing/returning files.
 */
TEST(resource_pool_file, basic_file_pool)
{
    // Create a temporary file for testing
    const std::string temp_file = get_temp_file_path("arrp_test_basic.txt");

    // Create resource pool for FILE* handles
    siddiqsoft::arrp::resource_pool<FILE*> file_pool;

    EXPECT_EQ(0, file_pool.size());
    {
        auto fp = std::fopen(temp_file.c_str(), "w+");
        ASSERT_NE(nullptr, fp);
        file_pool.seed_to_pool(std::move(fp));
    }

    EXPECT_EQ(1u, file_pool.size());

    // Borrow the file
    {
        siddiqsoft::arrp::scoped_resource<FILE*> file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        auto file_wrapper = std::move(file_result);
        EXPECT_EQ(0u, file_pool.size());

        // Write to the file
        std::print(*file_wrapper, "Hello, World!\n");
    }
    // File is automatically returned to pool

    EXPECT_EQ(1u, file_pool.size());

    // Borrow again and verify content
    {
        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        auto file_wrapper = std::move(file_result);
        std::rewind(*file_wrapper);

        std::array<char, 100> buffer {};
        ASSERT_NE(nullptr, std::fgets(buffer.data(), sizeof(buffer), *file_wrapper));
        EXPECT_STREQ("Hello, World!\n", buffer.data());
    }

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test FILE* resource pool with concurrent access
 */
TEST(resource_pool_file, concurrent_file_access)
{
    const std::string                      temp_file = get_temp_file_path("arrp_test_concurrent.txt");

    siddiqsoft::arrp::resource_pool<FILE*> file_pool;

    std::print(std::cerr, "About to add file `{}` to the pool..\n", temp_file);
    // Add a file to the pool
    FILE* fp = std::fopen(temp_file.c_str(), "w+");
    ASSERT_NE(nullptr, fp);
    std::print(std::cerr, "About to add..{:p}\n", static_cast<void*>(fp));
    file_pool.seed_to_pool(std::move(fp));
    EXPECT_EQ(1u, file_pool.size());

    std::atomic<int>         write_count {0};
    std::vector<std::thread> threads;

    std::print(std::cerr, "About to kick off the threads to use pool with {} items.\n", file_pool.size());
    // Create multiple threads that write to the file
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&file_pool, &write_count, i]() {
            auto file_result = file_pool.borrow_from_pool();
            if (file_result.has_value()) {
                auto fw = std::move(file_result);
                std::print(*fw, "Thread %d\n", i);
                ++write_count;
            }
        });
    }

    // Critical to wait for a second otherwise terminating will stop processing
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    std::print(std::cerr, "About to terminated threads {}.\n", threads.size());
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_GE(write_count, 1);
    EXPECT_EQ(1u, file_pool.size());

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test FILE* resource pool with read and write operations
 */
TEST(resource_pool_file, file_read_write_operations)
{
    const std::string                      temp_file = get_temp_file_path("arrp_test_rw.txt");

    siddiqsoft::arrp::resource_pool<FILE*> file_pool;

    // Write to file
    {
        FILE* fp = std::fopen(temp_file.c_str(), "w+");
        ASSERT_NE(nullptr, fp);

        file_pool.seed_to_pool(std::move(fp));
        EXPECT_EQ(1u, file_pool.size());

        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        std::print(*file_result, "Line 1\n");
        std::print(*file_result, "Line 2\n");
        std::print(*file_result, "Line 3\n");
        std::fflush(*file_result);
    }

    EXPECT_EQ(1u, file_pool.size());

    // Read from file
    {
        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        auto fw = std::move(file_result);
        std::rewind(*fw);

        std::array<char, 100> buffer {};
        ASSERT_NE(nullptr, std::fgets(buffer.data(), sizeof(buffer), *fw));
        EXPECT_STREQ("Line 1\n", buffer.data());

        ASSERT_NE(nullptr, std::fgets(buffer.data(), sizeof(buffer), *fw));
        EXPECT_STREQ("Line 2\n", buffer.data());

        ASSERT_NE(nullptr, std::fgets(buffer.data(), sizeof(buffer), *fw));
        EXPECT_STREQ("Line 3\n", buffer.data());
    }

    // Cleanup
    safe_remove_file(temp_file);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
