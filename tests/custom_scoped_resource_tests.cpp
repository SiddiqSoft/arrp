/*
    Custom Scoped Resource Tests
    Tests for resource_pool using custom scoped_resource<FILE*> wrapper

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
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/resource_pool.hpp"

namespace fs = std::filesystem;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

/**
 * @brief Get a temporary file path (cross-platform)
 * @return Path to a temporary file
 */
std::string get_temp_file_path()
{
    auto temp_dir  = fs::temp_directory_path();
    auto temp_file = temp_dir / "arrp_test_XXXXXX";
    return temp_file.string();
}

/**
 * @brief Create a temporary file for testing
 * @return Path to the temporary file
 */
std::string create_temp_file()
{
    std::string temp_path = get_temp_file_path();
    FILE*       file      = std::fopen(temp_path.c_str(), "w");
    if (file == nullptr) {
        throw std::runtime_error("Failed to create temporary file");
    }
    std::fclose(file);
    return temp_path;
}


/**
 * @brief Clean up a temporary file
 * @param path Path to the file to remove
 */
void cleanup_temp_file(const std::string& path)
{
    // Retry logic for Windows file locking issues
    int max_retries = 5;
    for (int i = 0; i < max_retries; ++i) {
        try {
            if (fs::exists(path)) {
                fs::remove(path);
                return;
            }
        }
        catch (const std::exception&) {
            if (i < max_retries - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
}

// ============================================================================
// TESTS WITH FILE* RESOURCE POOL
// ============================================================================

/**
 * @brief Test basic file resource pool creation
 */
TEST(custom_scoped_resource, basic_file_pool_creation)
{
    std::string temp_file = create_temp_file();

    std::cerr << std::format("{} - using temp_file:{}\n", __func__, temp_file);

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            std::cerr << std::format("{} - invoked for filehandle:{:p}\n", __func__, (void*)fh);
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        FILE*                                  file = std::fopen(temp_file.c_str(), "w+");
        ASSERT_NE(nullptr, file);

        // std::cerr << std::format("about to add to pool: {}\n", pool.to_json().value().get().dump());
        pool.add_to_pool(std::move(file));
        EXPECT_EQ(1u, pool.size().value_or(0));
        // std::cerr << std::format("after add to pool: {}\n", pool.to_json().value().get().dump());

        {
            auto file_result = pool.borrow_from_pool();
            EXPECT_TRUE(file_result.has_value());
            // std::cerr << std::format("after borrow to pool: {}\n", pool.to_json().value().get().dump());
        }

        std::cerr << std::format("after auto-return to pool: {}\n", pool.to_json().value().get().dump());

        EXPECT_EQ(1u, pool.size().value_or(0));
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test writing to file through resource
 */
TEST(custom_scoped_resource, write_to_file)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        FILE*                                  file = std::fopen(temp_file.c_str(), "w+");
        ASSERT_NE(nullptr, file);

        pool.add_to_pool(std::move(file));

        {
            auto file_result = pool.borrow_from_pool();
            EXPECT_TRUE(file_result.has_value());
            auto   fp      = *file_result.value();
            size_t written = std::fwrite("Hello, World!", 1, 13, fp);
            EXPECT_EQ(13u, written);
            std::fflush(fp);
        }

        // Verify file contents
        FILE* verify_file = std::fopen(temp_file.c_str(), "r");
        ASSERT_NE(nullptr, verify_file);

        char buffer[100] = {0};
        std::fread(buffer, 1, sizeof(buffer), verify_file);
        std::fclose(verify_file);

        EXPECT_STREQ("Hello, World!", buffer);
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test multiple file resources in pool
 */
TEST(custom_scoped_resource, multiple_file_resources)
{
    std::string temp_file1 = create_temp_file();
    std::string temp_file2 = create_temp_file();
    std::string temp_file3 = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing filehandle:{:p}\n", __func__, (void*)fh);
                fclose(fh);
            }
        }};

        pool.add_to_pool(std::fopen(temp_file1.c_str(), "w+"));
        pool.add_to_pool(std::fopen(temp_file2.c_str(), "w+"));
        pool.add_to_pool(std::fopen(temp_file3.c_str(), "w+"));

        EXPECT_EQ(3u, pool.size().value_or(0));

        // Borrow all files
        {
            auto res1 = pool.borrow_from_pool();
            auto res2 = pool.borrow_from_pool();
            auto res3 = pool.borrow_from_pool();

            EXPECT_EQ(0u, pool.size().value_or(0));

            if (res1.has_value()) std::fprintf(*res1.value(), "File 1");
            if (res2.has_value()) std::fprintf(*res2.value(), "File 2");
            if (res3.has_value()) std::fprintf(*res3.value(), "File 3");

            if (res1.has_value()) std::fflush(*res1.value());
            if (res2.has_value()) std::fflush(*res2.value());
            if (res3.has_value()) std::fflush(*res3.value());
        }

        // All files returned to pool
        EXPECT_EQ(3u, pool.size().value_or(0));
    }
    catch (...) {
        cleanup_temp_file(temp_file1);
        cleanup_temp_file(temp_file2);
        cleanup_temp_file(temp_file3);
        throw;
    }

    cleanup_temp_file(temp_file1);
    cleanup_temp_file(temp_file2);
    cleanup_temp_file(temp_file3);
}

/**
 * @brief Test file resource persistence across borrow/return cycles
 */
TEST(custom_scoped_resource, file_persistence_across_cycles)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));

        // First cycle: write data
        {
            auto file_result = pool.borrow_from_pool();
            if (file_result.has_value()) {
                auto fp = *file_result.value();
                std::fprintf(fp, "First write\n");
                std::fflush(fp);
            }
        }

        // Second cycle: append more data
        {
            auto file_result = pool.borrow_from_pool();
            if (file_result.has_value()) {
                auto fp = *file_result.value();
                std::fprintf(fp, "Second write\n");
                std::fflush(fp);
            }
        }

        // Verify both writes are in the file
        FILE* verify_file = std::fopen(temp_file.c_str(), "r");
        ASSERT_NE(nullptr, verify_file);

        char buffer[200] = {0};
        std::fread(buffer, 1, sizeof(buffer), verify_file);
        std::fclose(verify_file);

        EXPECT_TRUE(std::string(buffer).find("First write") != std::string::npos);
        EXPECT_TRUE(std::string(buffer).find("Second write") != std::string::npos);
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test concurrent file writes from multiple threads
 */
TEST(custom_scoped_resource, concurrent_file_writes)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        // Create multiple file handles
        for (int i = 0; i < 4; ++i) {
            pool.add_to_pool(std::fopen(temp_file.c_str(), "a+"));
        }

        EXPECT_EQ(4u, pool.size().value_or(0));

        std::atomic_int           write_count {0};
        std::barrier              start_barrier {4};

        std::vector<std::jthread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&, t]() {
                start_barrier.arrive_and_wait();
                for (int i = 0; i < 10; ++i) {
                    auto file_result = pool.borrow_from_pool();
                    if (file_result.has_value()) {
                        auto        fp  = *file_result.value();
                        std::string msg = std::format("Thread {} iteration {}\n", t, i);
                        std::fwrite(msg.c_str(), 1, msg.size(), fp);
                        std::fflush(fp);
                        write_count++;
                    }
                }
            });
        }

        threads.clear();

        EXPECT_EQ(4u, pool.size().value_or(0));
        EXPECT_GT(write_count.load(), 0);
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test file resource invalidation
 */
TEST(custom_scoped_resource, file_resource_invalidation)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));
        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));

        EXPECT_EQ(2u, pool.size().value_or(0));

        {
            auto file_result = pool.borrow_from_pool();
            EXPECT_TRUE(file_result.has_value());
            EXPECT_EQ(1u, pool.size().value_or(0));

            // Invalidate the resource
            file_result.value().invalidate();
        }

        // Resource should NOT be returned to pool
        EXPECT_EQ(1u, pool.size().value_or(0));
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test file resource move semantics
 */
TEST(custom_scoped_resource, file_resource_move_semantics)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));

        {
            auto resource1 = pool.borrow_from_pool();
            EXPECT_TRUE(resource1.has_value());

            // Move to resource2
            auto resource2 = std::move(resource1);
            EXPECT_TRUE(resource2.has_value());
        }

        // Only one resource should be returned
        EXPECT_EQ(1u, pool.size().value_or(0));
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test JSON serialization with file resources
 */
TEST(custom_scoped_resource, json_serialization)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));

        // Borrow and return
        {
            auto res = pool.borrow_from_pool();
            if (res.has_value()) {
                auto fp = *res.value();
                std::fprintf(fp, "test data");
                std::fflush(fp);
            }
        }

        auto json = pool.to_json();

        // Verify JSON structure
        if (json.has_value()) {
            auto& j = json.value().get();
            EXPECT_TRUE(j.contains("_typver"));
            EXPECT_TRUE(j.contains("capacity"));
            EXPECT_TRUE(j.contains("size"));
            EXPECT_TRUE(j.contains("autoadds"));
            EXPECT_TRUE(j.contains("borrows"));
            EXPECT_TRUE(j.contains("returns"));
            EXPECT_TRUE(j.contains("adds"));
        }
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test high-throughput file operations
 */
TEST(custom_scoped_resource, high_throughput_file_ops)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        // Pre-populate pool
        for (int i = 0; i < 4; ++i) {
            pool.add_to_pool(std::fopen(temp_file.c_str(), "a+"));
        }

        std::atomic_int           operations {0};
        std::barrier              start_barrier {4};

        std::vector<std::jthread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&, t]() {
                start_barrier.arrive_and_wait();
                for (int i = 0; i < 50; ++i) {
                    auto file_result = pool.borrow_from_pool();
                    if (file_result.has_value()) {
                        auto        fp  = *file_result.value();
                        std::string msg = std::format("Op {} from thread {}\n", i, t);
                        std::fwrite(msg.c_str(), 1, msg.size(), fp);
                        std::fflush(fp);
                        operations++;
                    }
                }
            });
        }

        threads.clear();

        EXPECT_EQ(4u, pool.size().value_or(0));
        EXPECT_GT(operations.load(), 0);
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test exception safety with file resources
 */
TEST(custom_scoped_resource, exception_safety)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));

        try {
            auto res = pool.borrow_from_pool();
            if (res.has_value()) {
                auto fp = *res.value();
                std::fprintf(fp, "Before exception");
            }
            throw std::runtime_error("Test exception");
        }
        catch (const std::runtime_error&) {
            // Exception caught
        }

        // Resource should still be returned to pool
        EXPECT_EQ(1u, pool.size().value_or(0));
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test capacity limits with file resources
 */
TEST(custom_scoped_resource, capacity_limits)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {
                2, {}, [temp_file](FILE*&& fh) {
                    if (fh != nullptr) {
                        std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                        fclose(fh);
                    }
                }};

        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));
        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));

        EXPECT_EQ(2u, pool.size().value_or(0));

        // Borrow both
        auto r1 = pool.borrow_from_pool();
        auto r2 = pool.borrow_from_pool();

        EXPECT_EQ(0u, pool.size().value_or(0));

        // Should fail when trying to borrow beyond capacity
        auto r3 = pool.borrow_from_pool();
        EXPECT_FALSE(r3.has_value());
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test rapid file open/close cycles
 */
TEST(custom_scoped_resource, rapid_file_cycles)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));

        for (int cycle = 0; cycle < 50; ++cycle) {
            {
                auto file_result = pool.borrow_from_pool();
                if (file_result.has_value()) {
                    auto        fp  = *file_result.value();
                    std::string msg = std::format("Cycle {}\n", cycle);
                    std::fwrite(msg.c_str(), 1, msg.size(), fp);
                    std::fflush(fp);
                }
            }
        }

        EXPECT_EQ(1u, pool.size().value_or(0));
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test file resource with clear operation
 */
TEST(custom_scoped_resource, clear_operation)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[temp_file](FILE*&& fh) {
            std::cerr << std::format("{} - invoked  filehandle:{:p}\n", __func__, (void*)fh);
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing `{}` filehandle:{:p}\n", __func__, temp_file, (void*)fh);
                fclose(fh);
            }
        }};

        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));
        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));
        pool.add_to_pool(std::fopen(temp_file.c_str(), "w+"));

        EXPECT_EQ(3u, pool.size().value_or(0));

        pool.clear();

        EXPECT_EQ(0u, pool.size().value_or(0));

        // After clear, checkout should fail
        auto res = pool.borrow_from_pool();
        EXPECT_FALSE(res.has_value());
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test FIFO ordering with file resources
 */
TEST(custom_scoped_resource, fifo_ordering)
{
    std::string temp_file1 = create_temp_file();
    std::string temp_file2 = create_temp_file();
    std::string temp_file3 = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*> pool {[](FILE*&& fh) {
            if (fh != nullptr) {
                std::cerr << std::format("{} - closing filehandle:{:p}\n", __func__, (void*)fh);
                fclose(fh);
            }
        }};

        // Create files with different content
        FILE* file1 = std::fopen(temp_file1.c_str(), "w+");
        FILE* file2 = std::fopen(temp_file2.c_str(), "w+");
        FILE* file3 = std::fopen(temp_file3.c_str(), "w+");

        ASSERT_NE(nullptr, file1);
        ASSERT_NE(nullptr, file2);
        ASSERT_NE(nullptr, file3);

        // Write different markers to each file
        std::fprintf(file1, "FILE1");
        std::fprintf(file2, "FILE2");
        std::fprintf(file3, "FILE3");

        std::fflush(file1);
        std::fflush(file2);
        std::fflush(file3);

        pool.add_to_pool(std::move(file1));
        pool.add_to_pool(std::move(file2));
        pool.add_to_pool(std::move(file3));

        EXPECT_EQ(3u, pool.size().value_or(0));

        // Checkout in FIFO order
        {
            auto res1 = pool.borrow_from_pool();
            auto res2 = pool.borrow_from_pool();
            auto res3 = pool.borrow_from_pool();

            EXPECT_EQ(0u, pool.size().value_or(0));
        }

        // All returned to pool
        EXPECT_EQ(3u, pool.size().value_or(0));
    }
    catch (...) {
        cleanup_temp_file(temp_file1);
        cleanup_temp_file(temp_file2);
        cleanup_temp_file(temp_file3);
        throw;
    }

    cleanup_temp_file(temp_file1);
    cleanup_temp_file(temp_file2);
    cleanup_temp_file(temp_file3);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
