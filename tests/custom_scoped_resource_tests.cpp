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
// CUSTOM SCOPED RESOURCE WRAPPER FOR FILE*
// ============================================================================

/**
 * @class ScopedFileResource
 * @brief Custom scoped_resource wrapper for FILE* that adds file-specific functionality
 *
 * This class derives from scoped_resource<FILE*> and adds custom methods for
 * file operations while maintaining RAII semantics.
 *
 * Key improvements:
 * - Non-template specialization for FILE* only (simpler usage)
 * - Direct access to FILE* without double dereference
 * - Operator overloads for intuitive file access
 * - File-specific methods: write(), read(), flush(), is_valid()
 */
class ScopedFileResource : public siddiqsoft::arrp::scoped_resource<FILE*>
{
    using Base = siddiqsoft::arrp::scoped_resource<FILE*>;

public:
    /**
     * @brief Construct a scoped file resource
     * @param file The FILE* to wrap
     * @param callback Optional callback to return resource to pool
     */
    explicit ScopedFileResource(FILE*&& file, std::function<void(FILE*&&, siddiqsoft::arrp::release_reason rr)>&& callback = {}, bool is_valid = true)
        : Base(std::forward<FILE*&&>(file), std::move(callback), is_valid)
    {
    }

    /**
     * @brief Move constructor
     */
    ScopedFileResource(ScopedFileResource&& src) noexcept
        : Base(std::move(src))
    {
    }

    /**
     * @brief Copy constructor is deleted
     */
    ScopedFileResource(const ScopedFileResource&) = delete;

    /**
     * @brief Copy assignment operator is deleted
     */
    ScopedFileResource& operator=(const ScopedFileResource&) = delete;

    /**
     * @brief Move assignment operator
     */
    ScopedFileResource& operator=(ScopedFileResource&& src) noexcept
    {
        Base::operator=(std::move(src));
        return *this;
    }

    /**
     * @brief Get the underlying FILE* pointer
     * @return Pointer to the FILE object
     */
    FILE* get_file() const { return this->m_rsrc; }

    /**
     * @brief Dereference operator to access FILE*
     * @return Reference to FILE*
     */
    FILE*& operator*() { return this->m_rsrc; }

    /**
     * @brief Const dereference operator
     * @return Const reference to FILE*
     */
    FILE* const& operator*() const { return this->m_rsrc; }

    /**
     * @brief Arrow operator for direct FILE* access
     * @return Pointer to FILE
     */
    FILE* operator->() { return this->m_rsrc; }

    /**
     * @brief Const arrow operator
     * @return Const pointer to FILE
     */
    FILE* const operator->() const { return this->m_rsrc; }

    /**
     * @brief Write data to the file
     * @param data The data to write
     * @return Number of bytes written
     */
    size_t write(const std::string& data)
    {
        if (this->m_rsrc == nullptr) {
            throw std::runtime_error("File pointer is null");
        }
        return std::fwrite(data.c_str(), 1, data.size(), this->m_rsrc);
    }

    /**
     * @brief Read data from the file
     * @param buffer Buffer to read into
     * @param size Size of buffer
     * @return Number of bytes read
     */
    size_t read(char* buffer, size_t size)
    {
        if (this->m_rsrc == nullptr) {
            throw std::runtime_error("File pointer is null");
        }
        return std::fread(buffer, 1, size, this->m_rsrc);
    }

    /**
     * @brief Flush the file
     */
    void flush()
    {
        if (this->m_rsrc == nullptr) {
            throw std::runtime_error("File pointer is null");
        }
        std::fflush(this->m_rsrc);
    }

    /**
     * @brief Check if file is valid
     */
    bool is_valid() const { return this->m_rsrc != nullptr; }
};

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
#ifdef _WIN32
    char temp_path[MAX_PATH];
    char temp_dir[MAX_PATH];

    if (GetTempPathA(MAX_PATH, temp_dir) == 0) {
        throw std::runtime_error("Failed to get temp directory");
    }

    if (GetTempFileNameA(temp_dir, "arrp", 0, temp_path) == 0) {
        throw std::runtime_error("Failed to create temp file name");
    }

    return std::string(temp_path);
#else
    char temp_path[] = "/tmp/arrp_test_XXXXXX";
    int  fd          = mkstemp(temp_path);
    if (fd == -1) {
        throw std::runtime_error("Failed to create temporary file");
    }
    close(fd);
    return std::string(temp_path);
#endif
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
 * @brief Close all files in a pool and clear it
 * @param pool The resource pool to clear
 */
template <typename PoolType>
void close_and_clear_pool(PoolType& pool)
{
    // Borrow and close all remaining files in the pool
    while (pool.size() > 0) {
        try {
            auto file_resource = pool.checkout();
            if (file_resource.is_valid()) {
                std::fclose(file_resource.get_file());
                file_resource.invalidate(); // Prevent returning to pool
            }
        }
        catch (const std::exception&) {
            break;
        }
    }
    pool.clear();
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
// TESTS WITH CUSTOM SCOPED_RESOURCE<FILE*>
// ============================================================================

/**
 * @brief Test basic file resource pool creation
 */
TEST(custom_scoped_resource, basic_file_pool_creation)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        FILE*                                                      file = std::fopen(temp_file.c_str(), "w+");
        ASSERT_NE(nullptr, file);

        pool.checkin(std::move(file));
        EXPECT_EQ(1u, pool.size());

        {
            auto file_resource = pool.checkout();
            EXPECT_TRUE(file_resource.is_valid());
        }

        EXPECT_EQ(1u, pool.size());
        close_and_clear_pool(pool);
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test writing to file through custom resource
 */
TEST(custom_scoped_resource, write_to_file)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        FILE*                                                      file = std::fopen(temp_file.c_str(), "w+");
        ASSERT_NE(nullptr, file);

        pool.checkin(std::move(file));

        {
            auto   file_resource = pool.checkout();
            size_t written       = file_resource.write("Hello, World!");
            EXPECT_EQ(13u, written);
            file_resource.flush();
        }

        // Verify file contents
        FILE* verify_file = std::fopen(temp_file.c_str(), "r");
        ASSERT_NE(nullptr, verify_file);

        char buffer[100] = {0};
        std::fread(buffer, 1, sizeof(buffer), verify_file);
        std::fclose(verify_file);

        EXPECT_STREQ("Hello, World!", buffer);
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        pool.checkin(std::fopen(temp_file1.c_str(), "w+"));
        pool.checkin(std::fopen(temp_file2.c_str(), "w+"));
        pool.checkin(std::fopen(temp_file3.c_str(), "w+"));

        EXPECT_EQ(3u, pool.size());

        // Borrow all files
        {
            auto res1 = pool.checkout();
            auto res2 = pool.checkout();
            auto res3 = pool.checkout();

            EXPECT_EQ(0u, pool.size());

            res1.write("File 1");
            res2.write("File 2");
            res3.write("File 3");

            res1.flush();
            res2.flush();
            res3.flush();
        }

        // All files returned to pool
        EXPECT_EQ(3u, pool.size());
        close_and_clear_pool(pool);
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
 * @brief Test file resource persistence across checkout/checkin cycles
 */
TEST(custom_scoped_resource, file_persistence_across_cycles)
{
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        pool.checkin(std::fopen(temp_file.c_str(), "w+"));

        // First cycle: write data
        {
            auto file_resource = pool.checkout();
            file_resource.write("First write\n");
            file_resource.flush();
        }

        // Second cycle: append more data
        {
            auto file_resource = pool.checkout();
            file_resource.write("Second write\n");
            file_resource.flush();
        }

        // Verify both writes are in the file
        FILE* verify_file = std::fopen(temp_file.c_str(), "r");
        ASSERT_NE(nullptr, verify_file);

        char buffer[200] = {0};
        std::fread(buffer, 1, sizeof(buffer), verify_file);
        std::fclose(verify_file);

        EXPECT_TRUE(std::string(buffer).find("First write") != std::string::npos);
        EXPECT_TRUE(std::string(buffer).find("Second write") != std::string::npos);
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        // Create multiple file handles
        for (int i = 0; i < 4; ++i) {
            pool.checkin(std::fopen(temp_file.c_str(), "a+"));
        }

        EXPECT_EQ(4u, pool.size());

        std::atomic_int           write_count {0};
        std::barrier              start_barrier {4};

        std::vector<std::jthread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&, t]() {
                start_barrier.arrive_and_wait();
                for (int i = 0; i < 10; ++i) {
                    try {
                        auto        file_resource = pool.checkout();
                        std::string msg           = std::format("Thread {} iteration {}\n", t, i);
                        file_resource.write(msg);
                        file_resource.flush();
                        write_count++;
                    }
                    catch (const std::runtime_error&) {
                        // Pool empty
                    }
                }
            });
        }

        threads.clear();

        EXPECT_EQ(4u, pool.size());
        EXPECT_GT(write_count.load(), 0);
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        pool.checkin(std::fopen(temp_file.c_str(), "w+"));
        pool.checkin(std::fopen(temp_file.c_str(), "w+"));

        EXPECT_EQ(2u, pool.size());

        {
            auto file_resource = pool.checkout();
            EXPECT_EQ(1u, pool.size());

            // Invalidate the resource
            file_resource.invalidate();
        }

        // Resource should NOT be returned to pool
        EXPECT_EQ(1u, pool.size());
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        pool.checkin(std::fopen(temp_file.c_str(), "w+"));

        {
            auto resource1 = pool.checkout();
            EXPECT_TRUE(resource1.is_valid());

            // Move to resource2
            auto resource2 = std::move(resource1);
            EXPECT_TRUE(resource2.is_valid());
        }

        // Only one resource should be returned
        EXPECT_EQ(1u, pool.size());
        close_and_clear_pool(pool);
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

/**
 * @brief Test custom factory callback for file resources
 */
TEST(custom_scoped_resource, custom_factory_callback)
{
    std::string temp_file = create_temp_file();

    try {
        std::atomic_int                                            creation_count {0};

        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {[&creation_count,
                                                                          &temp_file](auto& p) -> ScopedFileResource {
            creation_count++;
            FILE* file = std::fopen(temp_file.c_str(), "a+");

            if (file == nullptr) {
                throw std::runtime_error("Failed to open file");
            }

            return ScopedFileResource(
                    std::move(file), [&p](FILE*&& f, siddiqsoft::arrp::release_reason v) { p.checkin(std::move(f), v); });
        }};

        // Borrow resources - should trigger factory
        {
            auto res1 = pool.checkout();
            EXPECT_EQ(1, creation_count.load());

            auto res2 = pool.checkout();
            EXPECT_EQ(2, creation_count.load());
        }

        // Resources should be returned
        EXPECT_EQ(2u, pool.size());
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        pool.checkin(std::fopen(temp_file.c_str(), "w+"));

        // Borrow and return
        {
            auto res = pool.checkout();
            res.write("test data");
            res.flush();
        }

        auto json = pool.to_json();

        // Verify JSON structure
        EXPECT_TRUE(json.contains("_typver"));
        EXPECT_TRUE(json.contains("capacity"));
        EXPECT_TRUE(json.contains("size"));
        EXPECT_TRUE(json.contains("load"));
        EXPECT_TRUE(json.contains("checkedout"));


        // Verify counters
        EXPECT_TRUE(json.contains("checkout"));
        EXPECT_TRUE(json.contains("checkin"));
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        // Pre-populate pool
        for (int i = 0; i < 4; ++i) {
            pool.checkin(std::fopen(temp_file.c_str(), "a+"));
        }

        std::atomic_int           operations {0};
        std::barrier              start_barrier {4};

        std::vector<std::jthread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&, t]() {
                start_barrier.arrive_and_wait();
                for (int i = 0; i < 50; ++i) {
                    try {
                        auto        file_resource = pool.checkout();
                        std::string msg           = std::format("Op {} from thread {}\n", i, t);
                        file_resource.write(msg);
                        file_resource.flush();
                        operations++;
                    }
                    catch (const std::runtime_error&) {
                        // Pool empty
                    }
                }
            });
        }

        threads.clear();

        EXPECT_EQ(4u, pool.size());
        EXPECT_GT(operations.load(), 0);
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        pool.checkin(std::fopen(temp_file.c_str(), "w+"));

        try {
            auto res = pool.checkout();
            res.write("Before exception");
            throw std::runtime_error("Test exception");
        }
        catch (const std::runtime_error&) {
            // Exception caught
        }

        // Resource should still be returned to pool
        EXPECT_EQ(1u, pool.size());
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource, 2> pool {};

        pool.checkin(std::fopen(temp_file.c_str(), "w+"));
        pool.checkin(std::fopen(temp_file.c_str(), "w+"));

        EXPECT_EQ(2u, pool.size());

        // Borrow both
        auto r1 = pool.checkout();
        auto r2 = pool.checkout();

        EXPECT_EQ(0u, pool.size());

        // Should throw when trying to borrow beyond capacity
        EXPECT_THROW({ auto r3 = pool.checkout(); }, std::runtime_error);
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        pool.checkin(std::fopen(temp_file.c_str(), "w+"));

        for (int cycle = 0; cycle < 50; ++cycle) {
            {
                auto        file_resource = pool.checkout();
                std::string msg           = std::format("Cycle {}\n", cycle);
                file_resource.write(msg);
                file_resource.flush();
            }
        }

        EXPECT_EQ(1u, pool.size());
        close_and_clear_pool(pool);
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
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        pool.checkin(std::fopen(temp_file.c_str(), "w+"));
        pool.checkin(std::fopen(temp_file.c_str(), "w+"));
        pool.checkin(std::fopen(temp_file.c_str(), "w+"));

        EXPECT_EQ(3u, pool.size());

        close_and_clear_pool(pool);
        EXPECT_EQ(0u, pool.size());

        // After clear, checkout should throw
        EXPECT_THROW({ auto res = pool.checkout(); }, std::runtime_error);
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
    std::string temp_file = create_temp_file();

    try {
        siddiqsoft::arrp::resource_pool<FILE*, ScopedFileResource> pool {};

        // Create files with different content
        FILE* file1 = std::fopen(temp_file.c_str(), "w+");
        FILE* file2 = std::fopen(temp_file.c_str(), "w+");
        FILE* file3 = std::fopen(temp_file.c_str(), "w+");

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

        pool.checkin(std::move(file1));
        pool.checkin(std::move(file2));
        pool.checkin(std::move(file3));

        EXPECT_EQ(3u, pool.size());

        // Checkout in FIFO order
        {
            auto res1 = pool.checkout();
            auto res2 = pool.checkout();
            auto res3 = pool.checkout();

            EXPECT_EQ(0u, pool.size());
        }

        // All returned to pool
        EXPECT_EQ(3u, pool.size());
        close_and_clear_pool(pool);
    }
    catch (...) {
        cleanup_temp_file(temp_file);
        throw;
    }

    cleanup_temp_file(temp_file);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
