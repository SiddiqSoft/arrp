/*
    asynchrony-lib
    Add asynchrony to your apps

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
#include "../include/siddiqsoft/resource_pool.hpp"

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
 * @brief RAII wrapper for FILE* to ensure proper cleanup
 *
 * This wrapper ensures that FILE* resources are properly closed when
 * they go out of scope, even if an exception occurs.
 */
class FileHandle : public siddiqsoft::arrp::scoped_resource<FILE*>
{
public:
    std::string FileName {"dummy"};

private:
    // Helper lambda for cleanup callback
    static auto make_cleanup_callback() 
        -> std::function<std::expected<void, siddiqsoft::arrp::pool_error>(FILE*&&, bool)>
    {
        return [](FILE*&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
            if (res != nullptr) {
                std::fflush(res);
                if (isvalid) {
                    std::fclose(res);
                }
            }
            return {};
        };
    }

public:
    FileHandle() = delete;

    auto to_string() const -> std::string
    {
        return std::format("FileHandle - fn:{}   FILE* {:p}  isValid:{}\n",
                           FileName,
                           static_cast<void*>(m_rsrc),
                           m_is_valid);
    }

    // Constructor from FILE*
    explicit FileHandle(FILE*&& f, const std::string& fn = {}) noexcept
        : scoped_resource(make_cleanup_callback(), std::move(f))
        , FileName(fn)
    {
    }

    // Move constructor
    FileHandle(FileHandle&& other) noexcept
        : scoped_resource(std::move(other))
        , FileName(std::move(other.FileName))
    {
    }

    // Move assignment
    FileHandle& operator=(FileHandle&& other) noexcept
    {
        scoped_resource::operator=(std::move(other));
        FileName = std::move(other.FileName);
        std::cerr << std::format("  Assigned: {}", to_string());
        return *this;
    }

    // Delete copy operations
    FileHandle(const FileHandle&)            = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // Destructor
    ~FileHandle() = default;

    // Close the file
    void close()
    {
        if (m_rsrc != nullptr) {
            std::fflush(m_rsrc);
            std::fclose(m_rsrc);
            m_rsrc = nullptr;
        }
    }

    // Dereference to get FILE*
    FILE* operator*() const { return m_rsrc; }

    // Operator-> for convenience
    FILE* operator->() const { return m_rsrc; }

    // Boolean conversion
    explicit operator bool() const { return m_rsrc != nullptr; }
};

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

    EXPECT_EQ(0, file_pool.size().value_or(0));
    {
        auto fp = std::fopen(temp_file.c_str(), "w+");
        ASSERT_NE(nullptr, fp);
        file_pool.add_to_pool(std::move(fp));
    }

    EXPECT_EQ(1u, file_pool.size().value_or(0));

    // Borrow the file
    {
        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        auto file_wrapper = std::move(file_result.value());
        EXPECT_EQ(0u, file_pool.size().value_or(0));

        // Write to the file
        std::fprintf(*file_wrapper, "Hello, World!\n");
    }
    // File is automatically returned to pool

    EXPECT_EQ(1u, file_pool.size().value_or(0));

    // Borrow again and verify content
    {
        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        auto file_wrapper = std::move(file_result.value());
        std::rewind(*file_wrapper);

        char buffer[100] = {};
        ASSERT_NE(nullptr, std::fgets(buffer, sizeof(buffer), *file_wrapper));
        EXPECT_STREQ("Hello, World!\n", buffer);
    }

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test FileHandle explicit constructor with valid FILE*
 */
TEST(resource_pool_file, file_handle_explicit_constructor)
{
    const std::string temp_file = get_temp_file_path("arrp_test_explicit.txt");
    FILE*             fp         = std::fopen(temp_file.c_str(), "w");
    ASSERT_NE(nullptr, fp);

    FileHandle fh {std::move(fp), temp_file};
    EXPECT_TRUE(fh);

    fh.close();
    safe_remove_file(temp_file);
}

/**
 * @brief Test FileHandle move constructor
 */
TEST(resource_pool_file, file_handle_move_constructor)
{
    const std::string temp_file = get_temp_file_path("arrp_test_move_ctor.txt");
    FILE*             fp1        = std::fopen(temp_file.c_str(), "w");
    ASSERT_NE(nullptr, fp1);

    FileHandle fh1 {std::move(fp1), temp_file};
    EXPECT_TRUE(fh1);

    FileHandle fh2(std::move(fh1));
    EXPECT_TRUE(fh2);

    fh2.close();
    safe_remove_file(temp_file);
}

/**
 * @brief Test FileHandle move assignment operator
 */
TEST(resource_pool_file, file_handle_move_assignment)
{
    const std::string temp_file1 = get_temp_file_path("arrp_test_move_assign1.txt");
    const std::string temp_file2 = get_temp_file_path("arrp_test_move_assign2.txt");

    FILE*             fp1        = std::fopen(temp_file1.c_str(), "w");
    FILE*             fp2        = std::fopen(temp_file2.c_str(), "w");
    ASSERT_NE(nullptr, fp1);
    ASSERT_NE(nullptr, fp2);

    FileHandle fh1 {std::move(fp1), temp_file1};
    FileHandle fh2 {std::move(fp2), temp_file2};

    fh1 = std::move(fh2);

    EXPECT_TRUE(fh1);

    fh1.close();
    safe_remove_file(temp_file1);
    safe_remove_file(temp_file2);
}

/**
 * @brief Test FileHandle with read and write operations
 */
TEST(resource_pool_file, file_handle_read_write)
{
    const std::string                     temp_file = get_temp_file_path("arrp_test_rw.txt");

    siddiqsoft::arrp::resource_pool<FILE*> file_pool;

    // Write to file
    {
        FILE* fp = std::fopen(temp_file.c_str(), "w+");
        ASSERT_NE(nullptr, fp);

        file_pool.add_to_pool(std::move(fp));
        EXPECT_EQ(1u, file_pool.size().value_or(0));

        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        auto fw = std::move(file_result.value());
        std::fprintf(*fw, "Line 1\n");
        std::fprintf(*fw, "Line 2\n");
        std::fprintf(*fw, "Line 3\n");
        std::fflush(*fw);
    }

    EXPECT_EQ(1u, file_pool.size().value_or(0));

    // Read from file
    {
        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        auto fw = std::move(file_result.value());
        std::rewind(*fw);

        char buffer[100] = {};
        ASSERT_NE(nullptr, std::fgets(buffer, sizeof(buffer), *fw));
        EXPECT_STREQ("Line 1\n", buffer);

        ASSERT_NE(nullptr, std::fgets(buffer, sizeof(buffer), *fw));
        EXPECT_STREQ("Line 2\n", buffer);

        ASSERT_NE(nullptr, std::fgets(buffer, sizeof(buffer), *fw));
        EXPECT_STREQ("Line 3\n", buffer);
    }

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test FileHandle destructor cleanup
 */
TEST(resource_pool_file, file_handle_destructor_cleanup)
{
    const std::string temp_file = get_temp_file_path("arrp_test_dtor.txt");

    {
        FILE* fp = std::fopen(temp_file.c_str(), "w");
        ASSERT_NE(nullptr, fp);

        FileHandle fh {std::move(fp), temp_file};
        ASSERT_TRUE(fh);
        std::fprintf(*fh, "Destructor test\n");
        std::fflush(*fh);
        // fh goes out of scope and destructor is called
    }

    // File should be closed and readable
    FILE* fp = std::fopen(temp_file.c_str(), "r");
    ASSERT_NE(nullptr, fp);

    char buffer[100] = {};
    ASSERT_NE(nullptr, std::fgets(buffer, sizeof(buffer), fp));
    EXPECT_STREQ("Destructor test\n", buffer);

    std::fclose(fp);
    safe_remove_file(temp_file);
}

/**
 * @brief Test FileHandle with concurrent access
 */
TEST(resource_pool_file, file_handle_concurrent_access)
{
    const std::string                     temp_file = get_temp_file_path("arrp_test_concurrent.txt");

    siddiqsoft::arrp::resource_pool<FILE*> file_pool;

    std::cerr << std::format("About to add file `{}` to the pool..\n", temp_file);
    // Add a file to the pool
    FILE* fp = std::fopen(temp_file.c_str(), "w+");
    ASSERT_NE(nullptr, fp);
    std::cerr << std::format("About to add..{:p}\n", static_cast<void*>(fp));
    file_pool.add_to_pool(std::move(fp));
    EXPECT_EQ(1u, file_pool.size().value_or(0));

    std::atomic<int>         write_count {0};
    std::vector<std::thread> threads;

    std::cerr << std::format("About to kick off the threads to use pool with {} items.\n", file_pool.size().value_or(0));
    // Create multiple threads that write to the file
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&file_pool, &write_count, i]() {
            auto file_result = file_pool.borrow_from_pool();
            if (file_result.has_value()) {
                auto fw = std::move(file_result.value());
                std::fprintf(*fw, "Thread %d\n", i);
                ++write_count;
            }
        });
    }

    // Critical to wait for a second otherwise terminating will stop processing
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    std::cerr << std::format("About to terminated threads {}.\n", threads.size());
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_GE(write_count, 1);
    EXPECT_EQ(1u, file_pool.size().value_or(0));

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test resource pool with new resource callback constructor
 *
 * Tests the constructor that takes a callback to create resources on demand.
 * The callback is invoked when the pool needs a new resource and hasn't reached capacity.
 */
TEST(resource_pool_file, pool_with_new_resource_callback)
{
    const std::string temp_file = get_temp_file_path("arrp_test_callback.txt");

    std::atomic<int>  resource_creation_count {0};

    // Create a pool with a callback that creates FILE* resources on demand
    siddiqsoft::arrp::resource_pool<FILE*> file_pool(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [&](siddiqsoft::arrp::resource_pool<FILE*>& pool) -> std::expected<siddiqsoft::arrp::scoped_resource<FILE*>, siddiqsoft::arrp::pool_error> {
                resource_creation_count++;
                std::cerr << std::format(". . Adding new on-demand: {}...\n", temp_file.c_str());
                FILE* fp = std::fopen(temp_file.c_str(), "w+");
                if (fp == nullptr) {
                    return std::unexpected(siddiqsoft::arrp::pool_error::Unknown);
                }
                
                return siddiqsoft::arrp::scoped_resource<FILE*>(
                        [](FILE*&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                            if (res != nullptr) {
                                std::fflush(res);
                                if (isvalid) {
                                    std::fclose(res);
                                }
                            }
                            return {};
                        },
                        std::move(fp));
            });

    // Pool should be empty initially
    EXPECT_EQ(0u, file_pool.size().value_or(0));
    EXPECT_EQ(0, resource_creation_count);

    // First borrow should trigger resource creation via callback
    {
        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        EXPECT_EQ(1, resource_creation_count);
        EXPECT_EQ(0u, file_pool.size().value_or(0));

        auto file_wrapper = std::move(file_result.value());
        // Write to the file
        std::fprintf(*file_wrapper, "Created via callback\n");
    }
    // File is automatically returned to pool

    EXPECT_EQ(1u, file_pool.size().value_or(0));
    EXPECT_EQ(1, resource_creation_count);

    // Second borrow should reuse the resource from pool (no new creation)
    {
        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        EXPECT_EQ(1, resource_creation_count); // No new creation
        EXPECT_EQ(0u, file_pool.size().value_or(0));

        auto file_wrapper = std::move(file_result.value());
        // Verify content from previous write
        std::rewind(*file_wrapper);
        char buffer[100] = {};
        ASSERT_NE(nullptr, std::fgets(buffer, sizeof(buffer), *file_wrapper));
        EXPECT_STREQ("Created via callback\n", buffer);
    }

    EXPECT_EQ(1u, file_pool.size().value_or(0));
    EXPECT_EQ(1, resource_creation_count);

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test resource pool callback respects capacity limits
 *
 * Verifies that the callback is only invoked when the pool is under capacity.
 * Once capacity is reached, no new resources are created.
 */
TEST(resource_pool_file, pool_callback_respects_capacity)
{
    const std::string temp_file1 = get_temp_file_path("arrp_test_cap1.txt");
    const std::string temp_file2 = get_temp_file_path("arrp_test_cap2.txt");

    std::atomic<int>  resource_creation_count {0};

    // Create pool with capacity of 2
    siddiqsoft::arrp::resource_pool<FILE*> file_pool(
            2,
            [&](siddiqsoft::arrp::resource_pool<FILE*>& pool) -> std::expected<siddiqsoft::arrp::scoped_resource<FILE*>, siddiqsoft::arrp::pool_error> {
                resource_creation_count++;
                // Alternate between two files
                const char* filename = (resource_creation_count % 2 == 1) ? temp_file1.c_str() : temp_file2.c_str();
                FILE*       fp       = std::fopen(filename, "w+");
                if (fp == nullptr) {
                    return std::unexpected(siddiqsoft::arrp::pool_error::Unknown);
                }
                return siddiqsoft::arrp::scoped_resource<FILE*>(
                        [](FILE*&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                            if (res != nullptr) {
                                std::fflush(res);
                                if (isvalid) {
                                    std::fclose(res);
                                }
                            }
                            return {};
                        },
                        std::move(fp));
            });

    EXPECT_EQ(0u, file_pool.size().value_or(0));
    EXPECT_EQ(0, resource_creation_count);

    // First borrow creates resource 1
    auto file1_result = file_pool.borrow_from_pool();
    EXPECT_TRUE(file1_result.has_value());
    EXPECT_EQ(1, resource_creation_count);

    // Second borrow creates resource 2 (still under capacity)
    auto file2_result = file_pool.borrow_from_pool();
    EXPECT_TRUE(file2_result.has_value());
    EXPECT_EQ(2, resource_creation_count);

    // Return both resources
    file1_result = std::unexpected(siddiqsoft::arrp::pool_error::Unknown);
    file2_result = std::unexpected(siddiqsoft::arrp::pool_error::Unknown);

    EXPECT_EQ(2u, file_pool.size().value_or(0));
    EXPECT_EQ(2, resource_creation_count);

    // Cleanup
    safe_remove_file(temp_file1);
    safe_remove_file(temp_file2);
}

/**
 * @brief Test resource pool callback with multiple concurrent borrows
 *
 * Verifies that the callback is invoked correctly when multiple threads
 * borrow resources concurrently.
 */
TEST(resource_pool_file, pool_callback_concurrent_borrows)
{
    const std::string                            temp_file = get_temp_file_path("arrp_test_concurrent_cb.txt");

    std::atomic<int>                             resource_creation_count {0};

    siddiqsoft::arrp::resource_pool<FILE*> file_pool(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [&](siddiqsoft::arrp::resource_pool<FILE*>& pool) -> std::expected<siddiqsoft::arrp::scoped_resource<FILE*>, siddiqsoft::arrp::pool_error> {
                resource_creation_count++;
                FILE* fp = std::fopen(temp_file.c_str(), "w+");
                if (fp == nullptr) {
                    return std::unexpected(siddiqsoft::arrp::pool_error::Unknown);
                }
                return siddiqsoft::arrp::scoped_resource<FILE*>(
                        [](FILE*&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                            if (res != nullptr) {
                                std::fflush(res);
                                if (isvalid) {
                                    std::fclose(res);
                                }
                            }
                            return {};
                        },
                        std::move(fp));
            });

    std::atomic<int>                             successful_borrows {0};
    std::vector<std::thread>                     threads;

    // Create multiple threads that borrow resources
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&file_pool, &successful_borrows]() {
            auto file_result = file_pool.borrow_from_pool();
            if (file_result.has_value()) {
                auto fw = std::move(file_result.value());
                std::fprintf(*fw, "Thread borrow\n");
                successful_borrows++;
            }
        });
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // At least one thread should have successfully borrowed
    EXPECT_GE(successful_borrows, 1);
    // At least one resource should have been created
    EXPECT_GE(resource_creation_count, 1);

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test resource pool callback with manual add_to_pool
 *
 * Verifies that resources created via callback can be manually added back to pool.
 */
TEST(resource_pool_file, pool_callback_manual_add)
{
    const std::string                            temp_file = get_temp_file_path("arrp_test_manual_cb.txt");
    std::atomic<int>                             resource_creation_count {0};

    siddiqsoft::arrp::resource_pool<FILE*> file_pool(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [&](siddiqsoft::arrp::resource_pool<FILE*>& pool) -> std::expected<siddiqsoft::arrp::scoped_resource<FILE*>, siddiqsoft::arrp::pool_error> {
                resource_creation_count++;
                FILE* fp = std::fopen(temp_file.c_str(), "w+");
                if (fp == nullptr) {
                    return std::unexpected(siddiqsoft::arrp::pool_error::Unknown);
                }
                return siddiqsoft::arrp::scoped_resource<FILE*>(
                        [](FILE*&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                            if (res != nullptr) {
                                std::fflush(res);
                                if (isvalid) {
                                    std::fclose(res);
                                }
                            }
                            return {};
                        },
                        std::move(fp));
            });

    // Borrow a resource
    auto file_result = file_pool.borrow_from_pool();
    EXPECT_TRUE(file_result.has_value());
    EXPECT_EQ(1, resource_creation_count);
    EXPECT_EQ(0u, file_pool.size().value_or(0));

    auto file_wrapper = std::move(file_result.value());
    // Write to file
    std::fprintf(*file_wrapper, "Manual add test\n");

    // Let it go out of scope to automatically return to pool
    // (scoped_resource will call the callback to return it)

    EXPECT_EQ(1u, file_pool.size().value_or(0));

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test resource pool callback creates multiple resources sequentially
 *
 * Verifies that the callback creates new resources as needed when previous
 * resources are borrowed.
 */
TEST(resource_pool_file, pool_callback_sequential_creation)
{
    const std::string                            temp_file = get_temp_file_path("arrp_test_seq_cb.txt");

    std::atomic<int>                             resource_creation_count {0};

    siddiqsoft::arrp::resource_pool<FILE*> file_pool(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [&](siddiqsoft::arrp::resource_pool<FILE*>& pool) -> std::expected<siddiqsoft::arrp::scoped_resource<FILE*>, siddiqsoft::arrp::pool_error> {
                resource_creation_count++;
                FILE* fp = std::fopen(temp_file.c_str(), "w+");
                if (fp == nullptr) {
                    return std::unexpected(siddiqsoft::arrp::pool_error::Unknown);
                }
                return siddiqsoft::arrp::scoped_resource<FILE*>(
                        [](FILE*&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                            if (res != nullptr) {
                                std::fflush(res);
                                if (isvalid) {
                                    std::fclose(res);
                                }
                            }
                            return {};
                        },
                        std::move(fp));
            });

    // Borrow first resource
    auto file1_result = file_pool.borrow_from_pool();
    EXPECT_TRUE(file1_result.has_value());
    EXPECT_EQ(1, resource_creation_count);
    auto file1 = std::move(file1_result.value());
    std::fprintf(*file1, "Resource 1\n");

    // Borrow second resource (first is still borrowed)
    auto file2_result = file_pool.borrow_from_pool();
    EXPECT_TRUE(file2_result.has_value());
    EXPECT_EQ(2, resource_creation_count);
    auto file2 = std::move(file2_result.value());
    std::fprintf(*file2, "Resource 2\n");

    // Return first resource (goes out of scope)
    file1 = std::move(siddiqsoft::arrp::scoped_resource<FILE*>(
            [](FILE*&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                if (res != nullptr) {
                    std::fflush(res);
                    if (isvalid) {
                        std::fclose(res);
                    }
                }
                return {};
            },
            nullptr));
    EXPECT_EQ(1u, file_pool.size().value_or(0));

    // Borrow again - should reuse first resource
    auto file3_result = file_pool.borrow_from_pool();
    EXPECT_TRUE(file3_result.has_value());
    EXPECT_EQ(2, resource_creation_count); // No new creation
    EXPECT_EQ(0u, file_pool.size().value_or(0));

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test resource pool callback reuses resources efficiently
 *
 * Verifies that resources created via callback are properly reused
 * and not recreated unnecessarily.
 */
TEST(resource_pool_file, pool_callback_resource_reuse)
{
    const std::string                            temp_file = get_temp_file_path("arrp_test_reuse_cb.txt");
    std::atomic<int>                             resource_creation_count {0};

    siddiqsoft::arrp::resource_pool<FILE*> file_pool(
            siddiqsoft::arrp::resource_pool_limits::DefaultCapacity,
            [&](siddiqsoft::arrp::resource_pool<FILE*>& pool) -> std::expected<siddiqsoft::arrp::scoped_resource<FILE*>, siddiqsoft::arrp::pool_error> {
                resource_creation_count++;
                FILE* fp = std::fopen(temp_file.c_str(), "w+");
                if (fp == nullptr) {
                    return std::unexpected(siddiqsoft::arrp::pool_error::Unknown);
                }
                return siddiqsoft::arrp::scoped_resource<FILE*>(
                        [](FILE*&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                            if (res != nullptr) {
                                std::fflush(res);
                                if (isvalid) {
                                    std::fclose(res);
                                }
                            }
                            return {};
                        },
                        std::move(fp));
            });

    // Perform multiple borrow/return cycles
    for (int i = 0; i < 5; ++i) {
        auto file_result = file_pool.borrow_from_pool();
        EXPECT_TRUE(file_result.has_value());
        auto file_wrapper = std::move(file_result.value());
        std::fprintf(*file_wrapper, "Cycle %d\n", i);
    }

    // Should only have created one resource
    EXPECT_EQ(1, resource_creation_count);
    EXPECT_EQ(1u, file_pool.size().value_or(0));

    // Cleanup
    safe_remove_file(temp_file);
}

/**
 * @brief Test resource pool callback with capacity constraint
 *
 * Verifies that callback respects the capacity limit and doesn't create
 * more resources than allowed.
 */
TEST(resource_pool_file, pool_callback_capacity_constraint)
{
    const std::string temp_file = get_temp_file_path("arrp_test_cap_constraint.txt");
    std::atomic<int>  resource_creation_count {0};

    // Create pool with capacity of 3
    siddiqsoft::arrp::resource_pool<FILE*> file_pool(
            3,
            [&](siddiqsoft::arrp::resource_pool<FILE*>& pool) -> std::expected<siddiqsoft::arrp::scoped_resource<FILE*>, siddiqsoft::arrp::pool_error> {
                resource_creation_count++;
                FILE* fp = std::fopen(temp_file.c_str(), "w+");
                if (fp == nullptr) {
                    return std::unexpected(siddiqsoft::arrp::pool_error::Unknown);
                }
                return siddiqsoft::arrp::scoped_resource<FILE*>(
                        [](FILE*&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                            if (res != nullptr) {
                                std::fflush(res);
                                if (isvalid) {
                                    std::fclose(res);
                                }
                            }
                            return {};
                        },
                        std::move(fp));
            });

    // Borrow 3 resources (should create all 3)
    auto file1_result = file_pool.borrow_from_pool();
    EXPECT_TRUE(file1_result.has_value());
    EXPECT_EQ(1, resource_creation_count);

    auto file2_result = file_pool.borrow_from_pool();
    EXPECT_TRUE(file2_result.has_value());
    EXPECT_EQ(2, resource_creation_count);

    auto file3_result = file_pool.borrow_from_pool();
    EXPECT_TRUE(file3_result.has_value());
    EXPECT_EQ(3, resource_creation_count);

    // Try to borrow 4th resource - should fail because at capacity
    auto file4_result = file_pool.borrow_from_pool();
    EXPECT_FALSE(file4_result.has_value());
    EXPECT_EQ(3, resource_creation_count); // No new creation

    // Cleanup
    safe_remove_file(temp_file);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
