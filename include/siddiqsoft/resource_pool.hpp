/*
    arrp
    Auto returning resource pool for modern C++

    BSD 3-Clause License

    Copyright (c) 2026 Abdulkareem Siddiq
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
       contributors may be used to endorse or promote products derived from
       this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#ifndef RESOURCE_POOL_HPP
#define RESOURCE_POOL_HPP

#include <atomic>
#include <concepts>
#include <cstdint>
#include <deque>
#include <format>
#include <mutex>
#include <stdexcept>
#include <type_traits>

#include "private/common.hpp"
#include "private/scoped_resource.hpp"
#include "siddiqsoft/RunOnEnd.hpp"

namespace siddiqsoft::arrp
{
    /**
     * @class resource_pool
     * @brief Thread-safe auto-returning resource pool for modern C++
     *
     * @tparam T The resource type to be pooled. Must be move-constructible and non-arithmetic.
     * @tparam SRT The scoped resource wrapper type. Defaults to scoped_resource<T>.
     * @tparam InitCapacity The initial capacity of the pool. Defaults to resource_pool_limits::DefaultCapacity.
     *
     * @details
     * The resource_pool class implements a thread-safe object pool pattern with automatic
     * resource management using RAII principles. Resources are automatically returned to the
     * pool when the scoped_resource wrapper goes out of scope, eliminating manual resource
     * management and reducing the risk of resource leaks.
     *
     * Key Features:
     * - **Thread-Safe**: All operations are protected by mutexes for concurrent access
     * - **RAII Pattern**: Resources are automatically returned via scoped_resource destructors
     * - **Capacity Management**: Enforces a maximum capacity limit to prevent unbounded growth
     * - **FIFO Ordering**: Resources are retrieved from the front and added to the back
     * - **Customizable Factory**: Supports custom resource creation callbacks
     * - **Diagnostic Counters**: Tracks borrow, return, and auto-add operations
     * - **JSON Serialization**: Provides pool state diagnostics via JSON (when nlohmann/json is available)
     *
     * Usage Example:
     * @code
     * // Create a pool with custom resource factory
     * auto pool = resource_pool<MyResource>(
     *     [](resource_pool& p) -> scoped_resource<MyResource> {
     *         return scoped_resource<MyResource>(
     *             MyResource{},
     *             [&p](MyResource&& res) { p.return_to_pool(std::move(res)); }
     *         );
     *     }
     * );
     *
     * // Borrow a resource from the pool
     * {
     *     auto resource = pool.borrow_from_pool();
     *     // Use the resource...
     *     // Resource is automatically returned when it goes out of scope
     * }
     * @endcode
     *
     * Thread Safety:
     * - All public methods are thread-safe
     * - Multiple threads can safely borrow and return resources concurrently
     * - The pool uses a standard mutex for optimal performance
     *
     * Constraints:
     * - Not copy-constructible or move-constructible
     * - Not copy-assignable or move-assignable
     * - Resources must be move-constructible and non-arithmetic types
     * - InitCapacity must not exceed resource_pool_limits::MaxCapacity
     * - Capacity is limited to 255 resources (uint8_t)
     *
     * @warning Factory callbacks MUST NOT call any methods on the pool (borrow_from_pool,
     * return_to_pool, clear, etc.) as this will cause deadlock. The callback should only
     * create and return a new resource.
     *
     * @note The pool does not own resources directly; it manages scoped_resource wrappers
     * @note Resources are stored in a FIFO deque for predictable ordering
     * @note The pool is designed for long-lived objects that are expensive to create
     * @note Counters use uint64_t and will wrap around after ~18 quintillion operations
     */
    template <typename T, typename SRT = scoped_resource<T>, uint8_t InitCapacity = resource_pool_limits::DefaultCapacity>
        requires((InitCapacity <= resource_pool_limits::MaxCapacity)) && NonNumericMoveConstructible<T> &&
                std::derived_from<SRT, scoped_resource<T>>
    class resource_pool
    {
    private:
        /// @brief Maximum number of resources that can be in the pool
        uint8_t m_capacity {InitCapacity};

        /// @brief Number of resources currently checked out from the pool
        std::atomic_uint16_t m_resources_checkedout {0};

        /// @brief Number of resources that have been invalidated
        /// @note Currently unused; reserved for future use
        std::atomic_uint16_t m_invalidated_resources {0};

        /// @brief Counter for borrow operations from the pool
        /// @note Only counts successful borrow operations
        std::atomic_uint64_t m_counter_borrow_from_pool {0};

        /// @brief Counter for ondemand resource additions via factory callback
        std::atomic_uint64_t m_counter_ondemand_adds {0};

        /// @brief Counter for return operations to the pool
        std::atomic_uint64_t m_counter_return_to_pool {0};

        /// @brief Counter for automatic resource additions via factory callback
        std::atomic_uint64_t m_counter_auto_returned {0};

        /// @brief Internal deque storing the pooled resources
        /// @details Uses FIFO ordering: resources are added to back, retrieved from front
        std::deque<T> m_pool {};

#if defined(arrp_USE_RECURSIVE_MUTEX) || defined(ARRP_USE_RECURSIVE_MUTEX)
        /// @brief Mutex protecting access to the resource pool
        /// @details Uses a recursive mutex for tests which relax some deadlocks
        /// otherwise the CI will fail. It is also up to the user to ensure
        /// that they do not call methods that cause deadlocks.
        /// @note Marked as mutable to allow usage within const methods
        mutable std::recursive_mutex m_pool_lock {};
#else
        /// @brief Mutex protecting access to the resource pool
        /// @details Uses a standard mutex for optimal performance
        /// @note Marked as mutable to allow usage within const methods
        mutable std::mutex m_pool_lock {};
#endif

        /// @brief Callback to create and add new resources to the pool
        /// @details Invoked when the pool needs a resource and is within capacity limits.
        /// The client cannot directly add resources; instead, they provide this factory callback.
        /// @warning MUST NOT call any pool methods to avoid deadlock
        std::function<SRT(resource_pool&)> m_callback_to_add_new_raw_resource_to_pool {};

        /// @brief Callback invoked when a resource is invalidated
        /// @details Optional callback for custom cleanup when resources are marked invalid
        std::function<void(SRT&&)> m_callback_resource_invalidated {};

        /// @brief Callback invoked when the pool is about to shutdown
        /// @details Optional callback for cleanup operations during pool destruction
        std::function<void()> m_callback_pool_shutdown {};

    public:
        /**
         * @brief Construct a resource pool with optional custom resource factory
         *
         * @param new_resource_callback Optional callback function to create new resources.
         *        If not provided, a default factory creates T{} wrapped in SRT{}.
         *
         * @details
         * Initializes the resource pool with the specified capacity. If a custom callback
         * is provided, it will be used to create resources when the pool needs them.
         * Otherwise, a default factory is used that creates default-constructed resources.
         *
         * The default factory automatically wires up the auto-return callback so resources
         * are returned to the pool when the scoped_resource wrapper is destroyed.
         *
         * @note The pool starts empty; resources are created on-demand up to the capacity limit
         * @note The callback is stored and called later, not during construction
         *
         * @example
         * @code
         * // Using default factory
         * auto pool1 = resource_pool<MyResource>();
         *
         * // Using custom factory
         * auto pool2 = resource_pool<MyResource>(
         *     [](resource_pool& p) -> scoped_resource<MyResource> {
         *         return scoped_resource<MyResource>(
         *             MyResource::create(),
         *             [&p](MyResource&& res) { p.return_to_pool(std::move(res)); }
         *         );
         *     }
         * );
         * @endcode
         */
        resource_pool(std::function<SRT(resource_pool&)>&& new_resource_callback = {})
        {
            if (new_resource_callback) {
                m_callback_to_add_new_raw_resource_to_pool = std::move(new_resource_callback);
            }
            else {
                m_callback_to_add_new_raw_resource_to_pool = [this](resource_pool& pool) -> SRT {
                    // Create a SRT element and wire up the auto-return callback to return
                    // the resource back to this object.
                    return SRT {T {}, [this](T&& src) {
                                    this->m_counter_auto_returned++;
                                    this->return_to_pool(std::forward<T&&>(src));
                                }};
                    // Allow the compiler to use NRVO (move elision; do not use std::move here!)
                    // return temp;
                };
            }
        }

        // Not copy-able, not movable
        /// @brief Copy constructor is deleted
        resource_pool(resource_pool&) = delete;

        /// @brief Move constructor is deleted
        resource_pool(resource_pool&& src) = delete;

        /// @brief Copy assignment operator is deleted
        resource_pool& operator=(resource_pool&) = delete;

        /// @brief Move assignment operator is deleted
        resource_pool& operator=(resource_pool&& src) = delete;

        /**
         * @brief Destructor - clears all resources from the pool
         *
         * @details
         * Invokes the pool shutdown callback if registered, then clears all remaining
         * resources from the pool. Any resources that are currently checked out are
         * NOT affected by this operation.
         *
         * @note Safe to call even if the pool is empty
         * @note All remaining pooled resources are destroyed
         * @note Checked-out resources will be returned to the pool when their
         *       scoped_resource wrappers are destroyed
         */
        ~resource_pool()
        {
            if (m_callback_pool_shutdown) m_callback_pool_shutdown();
            clear();
        }

        /**
         * @brief Clear all items from the pool
         *
         * @details
         * Removes and destroys all resources currently in the pool.
         * Thread-safe operation. Safe to call on an empty pool.
         *
         * @thread_safety Thread-safe. Protected by mutex.
         *
         * @note All resources are destroyed when cleared
         * @note Any checked-out resources are NOT affected
         * @note This operation does not affect the capacity limit
         *
         * @example
         * @code
         * pool.clear();  // Remove all pooled resources
         * @endcode
         */
        void clear()
        {
            std::scoped_lock l(m_pool_lock);
            m_pool.clear();
        }

        /**
         * @brief Get the current size of the pool
         *
         * @return The number of available resources currently in the pool
         *
         * @details
         * Returns the number of resources available for checkout. This does not include
         * resources that are currently checked out.
         *
         * @thread_safety Thread-safe. Protected by mutex.
         *
         * @note This prevents TOCTOU (Time-of-Check-Time-of-Use) race conditions
         *       by returning the size directly without separate empty checks
         * @note Size may change immediately after this call due to concurrent access
         * @note The returned size is a snapshot at the moment of the call
         * @note Capacity is limited to 255 resources (uint8_t)
         *
         * @example
         * @code
         * auto available = pool.size();
         * std::cout << "Available resources: " << available << std::endl;
         * @endcode
         */
        [[nodiscard]] size_t size() const
        {
            std::scoped_lock l(m_pool_lock);
            return m_pool.size();
        }

        /**
         * @brief Borrow a resource from the pool
         *
         * @return A scoped_resource wrapper containing the borrowed resource
         *
         * @throws std::runtime_error If unable to obtain a resource (pool at capacity
         *         and no factory callback available)
         *
         * @details
         * Attempts to obtain a resource from the pool. The operation follows this logic:
         * 1. If the pool is non-empty, return the first resource (FIFO)
         * 2. If the pool is empty but under capacity, create a new resource via factory
         * 3. If at capacity and no resources available, throw std::runtime_error
         *
         * The returned scoped_resource automatically returns the resource to the pool
         * when it goes out of scope, implementing the RAII pattern.
         *
         * @thread_safety Thread-safe. Protected by mutex. Resource creation happens
         *                outside the lock to minimize contention.
         *
         * @note Increments the borrow counter only on successful borrow
         * @note Resources are returned in FIFO order
         * @note The resource is marked as valid upon return
         * @note If the factory callback is not set, the pool cannot grow beyond
         *       the initial resources
         *
         * @example
         * @code
         * try {
         *     auto resource = pool.borrow_from_pool();
         *     // Use the resource...
         *     // Automatically returned when resource goes out of scope
         * } catch (const std::runtime_error& e) {
         *     std::cerr << "Failed to borrow resource: " << e.what() << std::endl;
         * }
         * @endcode
         */
        [[nodiscard]] auto borrow_from_pool() -> SRT
        {
            try {
                // @note We use a unique_lock vs a scoped_lock to allow ourselves
                // to create the resource outside the lock!
                std::unique_lock l(m_pool_lock);

                if (!m_pool.empty()) {
                    // The pool is non-empty; return from the pool
                    // Return first element from the pool and pop it on scope end
                    RunOnEnd pop_guard([&]() {
                        m_pool.pop_front();
                    });

                    m_resources_checkedout++;
                    ++m_counter_borrow_from_pool;


                    // Make a wrapper..
                    // Create a SRT element and wire up the auto-return callback to return
                    // the resource back to this object.
                    return SRT {std::move(m_pool.front()), [this](T&& src) {
                                    this->m_counter_auto_returned++;
                                    this->return_to_pool(std::forward<T&&>(src));
                                }};
                    // Allow the compiler to use NRVO (move elision; do not use std::move here!)
                    // return temp;

                    // The pop_front() happens within this scope and within the lock!
                }
                else if ((m_capacity > m_pool.size() + m_resources_checkedout) && m_callback_to_add_new_raw_resource_to_pool) {
                    // We have no more items in the pool (we're starting up or everything is
                    // checked out) but we have not reached the limit. The limit is number
                    // of m_resources_checkedout + pool.size() < m_capacity We are
                    // under-capacity.. so we can return to the caller a new item..
                    m_resources_checkedout++;
                    // We should unlock the resource and ..
                    l.unlock();

                    ++m_counter_borrow_from_pool;
                    // Update the attempted delegated calls to add new raw resource to pool.
                    ++m_counter_ondemand_adds;

                    // ..delegate the new resource acquisition
                    // outside the lock.
                    return m_callback_to_add_new_raw_resource_to_pool(*this);
                }
                else if (m_capacity > m_pool.size() + m_resources_checkedout) {
                    // We're under-capacity.. but no dynamic resource provider
                }
            } // scope end
            catch (const std::exception& ex) {
                std::cerr << std::format("Error in borrow_from_pool: {}\n", ex.what());
                throw;
            }
            catch (...) {
                std::cerr << "Unknown exception in borrow_from_pool\n";
                throw;
            }

            auto msg = std::format(
                    "Pool Size:{}  checkedout:{}  capacity:{}", m_pool.size(), m_resources_checkedout.load(), m_capacity);
            throw std::runtime_error(msg);
        }

        /**
         * @brief Return a resource to the pool
         *
         * @param raw_resource R-Value reference to the resource to return to the pool
         *
         * @details
         * Adds a resource back to the pool, making it available for future checkout
         * operations. This is typically called automatically by the scoped_resource
         * destructor, but can also be called manually if needed.
         *
         * The resource is added to the back of the deque (FIFO ordering) and the
         * checked-out counter is decremented.
         *
         * @thread_safety Thread-safe. Protected by mutex.
         *
         * @note Increments the return counter
         * @note Resources are added to the back of the deque (FIFO)
         * @note This method is typically not called directly; use borrow_from_pool() instead
         * @note Only valid resources should be checked in (not moved-out or invalid)
         * @note When using shared_ptr, always use std::move to transfer ownership to the pool
         * @note Safe to call multiple times with the same resource (though not recommended)
         *
         * @example
         * @code
         * // Typically called automatically:
         * {
         *     auto resource = pool.borrow_from_pool();
         *     // Use resource...
         * }  // Automatically returned here
         *
         * // Manual return (advanced usage):
         * auto resource = pool.borrow_from_pool();
         * // ... use resource ...
         * pool.return_to_pool(std::move(*resource));
         * @endcode
         */
        void return_to_pool(T&& raw_resource)
        {
            ++m_counter_return_to_pool;

            std::scoped_lock l(m_pool_lock);

            m_pool.push_back(std::move(raw_resource));
            if (m_resources_checkedout > 0) m_resources_checkedout--;

#if defined(DEBUG) && defined(NLOHMANN_JSON_VERSION_MAJOR)
#endif
        }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        /**
         * @brief Serialize pool state to JSON
         *
         * @return nlohmann::json object with pool statistics
         *
         * @details
         * Returns a JSON object containing diagnostic information about the pool state.
         * Useful for monitoring, debugging, and logging pool behavior.
         *
         * The returned JSON object contains:
         * - `_typver`: Version identifier for the serialization format
         * - `capacity`: Maximum number of resources the pool can hold
         * - `size`: Number of resources currently in the pool
         * - `load`: Total resources (in pool + checked out)
         * - `invalidated`: Number of invalidated resources
         * - `checkedout`: Number of resources currently checked out
         * - `counters`: Object containing operation counters:
         *   - `autoadd`: Number of automatic resource additions
         *   - `return`: Number of return operations
         *   - `borrow`: Number of borrow operations
         *
         * @thread_safety Thread-safe. Protected by mutex.
         *
         * @note Only available when nlohmann/json is included
         * @note Provides a snapshot of the pool state at the moment of the call
         * @note Useful for performance monitoring and debugging
         *
         * @example
         * @code
         * auto state = pool.to_json();
         * std::cout << state.dump(2) << std::endl;
         * // Output:
         * // {
         * //   "_typver": "siddiqsoft.arrp.resource_pool/0.0.0",
         * //   "capacity": 10,
         * //   "size": 5,
         * //   "load": 8,
         * //   "invalidated": 0,
         * //   "checkedout": 3,
         * //   "counters": {
         * //     "autoadd": 3,
         * //     "return": 5,
         * //     "borrow": 8
         * //   }
         * // }
         * @endcode
         */
        nlohmann::json to_json() const
        {
            auto             myPoolSize = this->size();

            return {{"_typver", "siddiqsoft.arrp.resource_pool/0.0.0"},
                    {"capacity", m_capacity},
                    {"size", myPoolSize},
                    {"load", myPoolSize + m_resources_checkedout.load()},
                    {"invalidated", m_invalidated_resources.load()},
                    {"checkedout", m_resources_checkedout.load()},
                    {"counters",
                     {{"autoreturns", m_counter_auto_returned.load()},
                      {"newitems", m_counter_ondemand_adds.load()},
                      {"return", m_counter_return_to_pool.load()},
                      {"borrow", m_counter_borrow_from_pool.load()}}}};
        }
#endif
    };

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
    /**
     * @brief JSON serialization adapter for resource_pool
     *
     * @tparam T The resource type
     * @tparam SRT The scoped resource wrapper type
     * @tparam InitCapacity The initial pool capacity
     *
     * @param dest Destination JSON object to populate
     * @param src Source resource_pool object to serialize
     *
     * @details
     * Enables automatic JSON serialization of resource_pool objects via nlohmann::json.
     * This function is called by the JSON library when serializing a resource_pool.
     *
     * @note Only available when nlohmann/json is included
     * @note Delegates to the resource_pool::to_json() method
     *
     * @example
     * @code
     * resource_pool<MyResource> pool;
     * nlohmann::json j = pool;  // Automatically calls this function
     * std::cout << j.dump(2) << std::endl;
     * @endcode
     */
    template <typename T, typename SRT = scoped_resource<T>, uint8_t InitCapacity = arrp::resource_pool_limits::DefaultCapacity>
        requires((InitCapacity <= arrp::resource_pool_limits::MaxCapacity)) && NonNumericMoveConstructible<T> &&
                std::derived_from<SRT, scoped_resource<T>>
    static void to_json(nlohmann::json& dest, const siddiqsoft::arrp::resource_pool<T, SRT, InitCapacity>& src)
    {
        dest = src.to_json();
    }
#endif


} // namespace siddiqsoft::arrp
#endif
