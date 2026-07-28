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
#include <deque>
#include <format>
#include <mutex>
#include <type_traits>
#include <memory>
#include <expected>

#include "private/common.hpp"
#include "private/scoped_resource.hpp"
#include "siddiqsoft/RunOnEnd.hpp"

namespace siddiqsoft::arrp
{
    /// @brief Thread-safe auto-returning resource pool
    ///
    /// @details
    /// Manages a pool of resources that are automatically returned when scoped_resource
    /// instances are destroyed. Supports both fixed-size and auto-growing pools.
    /// All operations are thread-safe using std::mutex or std::recursive_mutex.
    ///
    /// @tparam T The resource type (must be move-constructible and non-arithmetic)
    /// @tparam SRT The scoped resource type (defaults to scoped_resource<T>)
    ///
    /// @note Thread-safe: All operations are protected by mutex
    /// @note RAII pattern: Resources are automatically returned to pool
    /// @note Callback-based: Supports factory, cleanup, and return callbacks
    /// @note Statistics: Tracks borrow/return operations and resource counts
    ///
    /// @example
    /// @code
    /// // Create a pool with auto-grow policy
    /// siddiqsoft::arrp::resource_pool<MyResource> pool(
    ///     10,  // capacity
    ///     siddiqsoft::arrp::auto_add_policy::AutoGrow
    /// );
    ///
    /// // Borrow a resource
    /// auto resource = pool.borrow_from_pool();
    /// if (resource) {
    ///     resource->doSomething();
    /// }
    /// // Resource automatically returned to pool when scoped_resource is destroyed
    /// @endcode
    template <typename T, typename SRT = scoped_resource<T>>
        requires NonNumericMoveConstructible<T> && std::derived_from<SRT, scoped_resource<T>>
    class resource_pool
    {
    private:
        /// @brief Maximum number of resources that can be in the pool
        uint8_t m_capacity {0};

        /// @brief Flag indicating pool shutdown is in progress
        std::atomic_bool m_is_shutdown {false};

        /// @brief Number of resources currently checked out from the pool
        std::atomic_int16_t m_resources_checkedout {0};

        /// @brief Number of resources that have been invalidated
        /// @note Currently unused; reserved for future use
        std::atomic_uint16_t m_abandoned {0};

        /// @brief Counter for resources at maximum pool size
        std::atomic_uint64_t m_capacity_poolsize {0};

        std::atomic_uint64_t m_counter_adds {0}, m_counter_ondemand_adds {0}, m_counter_returns {0}, m_counter_borrows {0};

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
// It might be more expensive but the client might find this useful!
#warning "You're using std::recursive_mutex which is more expensive"
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
        std::function<std::expected<SRT, pool_error>(resource_pool&)> m_callback_to_add_new_raw_resource_to_pool {};

        /// @brief Callback on resource cleanup during pool destruction
        /// This method is invoked within a lock and inside of a for-loop across each
        /// resource in the internal deque.
        /// This approach allows the client to perform any final cleanup for the given resource.
        /// @warning MUST NOT call any pool methods to avoid deadlock
        std::function<void(T&&)> m_callback_on_resource_cleanup {};

        /// @brief Sets the capacity. This is internal and can only be called from the constructor.
        /// The capacity of the internal queue must not be altered once set.
        void set_capacity(uint8_t init_capacity)
        {
#if defined(DEBUG)
            std::cerr << std::format("{} - capacity: {}  init_capacity:{}\n", __func__, m_capacity, init_capacity);
#endif

            // We're going to be inside construction context and we're assured
            // of only one invocation!
            if (m_capacity == 0) {
                if (init_capacity > resource_pool_limits::MaxCapacity) {
                    m_capacity = resource_pool_limits::MaxCapacity;
                }
                else if (init_capacity < resource_pool_limits::MinimumCapacity) {
                    m_capacity = resource_pool_limits::MinimumCapacity;
                }
                else {
                    m_capacity = init_capacity;
                }

                // Updated the capacity in the stats..
                m_json["capacity"] = m_capacity;

#if defined(DEBUG)
                std::cerr << std::format("{} - capacity: {}  init_capacity:{}\n", __func__, m_capacity, init_capacity);
#endif
            }
        }

        /// @brief Internal method does not require explicit lock
        bool is_pool_starving() { return m_capacity > m_pool.size(); }
        auto is_there_a_pool_deficit() { return m_pool.size() < m_capacity; }
        auto loan_size() { return m_resources_checkedout.load(); }

    public:
        /// @brief Default callback that does not auto-grow the resource pool
        /// @details Returns an error indicating no more resources are available
        static inline std::function<std::expected<SRT, pool_error>(resource_pool&)> CallbackDoNotAutoAddResource =
                [](resource_pool&) -> std::expected<SRT, pool_error> {
            return std::unexpected(pool_error::NoMoreResources);
        };

        /// @brief Constructs a resource pool with factory and cleanup callbacks
        ///
        /// @param init_capacity Initial capacity of the pool
        /// @param new_resource_callback Factory callback to create new resources
        /// @param on_shutdown_callback Optional cleanup callback invoked on destruction
        ///
        /// @note The factory callback is required and must return std::expected<SRT, pool_error>
        /// @note The cleanup callback is optional and invoked for each resource during destruction
        /// @note Capacity is clamped to valid range [MinimumCapacity, MaxCapacity]
        resource_pool(uint8_t                                                         init_capacity,
                      std::function<std::expected<SRT, pool_error>(resource_pool&)>&& new_resource_callback,
                      std::function<void(T&&)>&&                                      on_shutdown_callback = {})
            : m_callback_to_add_new_raw_resource_to_pool(new_resource_callback ? std::move(new_resource_callback)
                                                                               : CallbackDoNotAutoAddResource)
            , m_callback_on_resource_cleanup(std::move(on_shutdown_callback))
        {
#if defined(DEBUG)
            std::cerr << std::format(
                    "{}(x,y,z) - Invoked; init_capacity:{} with new resource callback and optional cleanup callback\n",
                    __func__,
                    init_capacity);
#endif
            set_capacity(init_capacity);
        }

        /// @brief Constructs a resource pool with only cleanup callback
        ///
        /// @param on_shutdown_callback Cleanup callback invoked on destruction
        ///
        /// @note Uses default capacity and no auto-grow policy
        /// @note The cleanup callback is optional and invoked for each resource during destruction
        resource_pool(std::function<void(T&&)>&& on_shutdown_callback)
            : m_callback_to_add_new_raw_resource_to_pool(CallbackDoNotAutoAddResource)
            , m_callback_on_resource_cleanup(std::move(on_shutdown_callback))
        {
#if defined(DEBUG)
            std::cerr << std::format("{}(z) - Invoked;  with new cleanup callback\n", __func__);
#endif
            set_capacity(resource_pool_limits::DefaultCapacity);
        }

        /// @brief Constructs a resource pool with capacity and auto-grow policy
        ///
        /// @param init_capacity Initial capacity of the pool (defaults to DefaultCapacity)
        /// @param add_policy Auto-grow policy (defaults to NoGrow)
        ///
        /// @note Capacity is clamped to valid range [MinimumCapacity, MaxCapacity]
        /// @note If add_policy is AutoGrow, resources are created on-demand
        /// @note If add_policy is NoGrow, pool returns error when exhausted
        resource_pool(uint8_t         init_capacity = resource_pool_limits::DefaultCapacity,
                      auto_add_policy add_policy    = auto_add_policy::NoGrow)
        {
#if defined(DEBUG)
            std::cerr << std::format(
                    "{}(x,b) - Invoked; init_capacity:{} with new add_policy: {}\n", __func__, init_capacity, add_policy);
#endif
            set_capacity(init_capacity);

            if (add_policy == auto_add_policy::NoGrow) {
                m_callback_to_add_new_raw_resource_to_pool = CallbackDoNotAutoAddResource;
            }
            else if (add_policy == auto_add_policy::AutoGrow) {
                // This method is declared here as lambda to capture the this pointer
                // whereas if we attempted to declared it earlier as a static inline then the
                // this pointer would not be captured.
                m_callback_to_add_new_raw_resource_to_pool = [this](resource_pool& pool) -> std::expected<SRT, pool_error> {
                    // Create a SRT element and wire up the auto-return callback to return
                    // the resource back to this object.
                    return SRT {[this](T&& src, bool isvalid) -> std::expected<void, pool_error> {
                                    // this callback puts the resource back..
                                    return this->return_to_pool(std::forward<T&&>(src), isvalid);
                                },
                                T {}};
                    // Allow the compiler to use NRVO (move elision; do not use std::move here!)
                };
            }
        }

        /// @brief Copy constructor is deleted
        /// @details resource_pool is not copyable to prevent resource duplication
        resource_pool(resource_pool&) = delete;

        /// @brief Move constructor is deleted
        /// @details resource_pool is not movable to maintain resource ownership
        resource_pool(resource_pool&& src) = delete;

        /// @brief Copy assignment operator is deleted
        /// @details resource_pool is not copyable to prevent resource duplication
        resource_pool& operator=(resource_pool&) = delete;

        /// @brief Move assignment operator is deleted
        /// @details resource_pool is not movable to maintain resource ownership
        resource_pool& operator=(resource_pool&& src) = delete;

        /// @brief Destructor - cleans up all resources in the pool
        ///
        /// Sets the shutdown flag and delegates to clear() to clean up resources.
        /// The cleanup callback is invoked for each resource if provided.
        ///
        /// @note Noexcept: Exceptions during cleanup are caught and logged
        /// @note All resources are cleaned up before destruction completes
        ~resource_pool()
        {
            {
                std::scoped_lock l(m_pool_lock);
                m_is_shutdown = true;
            }
#if defined(DEBUG)
            std::cerr << std::format("{} - invoked; shutdown set; now delegating to clear..\n", __func__);
#endif
            // Delegate to the clear() method which itself acquires a lock
            // so we should make sure we clear the lock to set the shutdown flag.
            this->clear();
        }

        /// @brief Clears all resources from the pool
        ///
        /// Removes all resources from the pool and invokes the cleanup callback for each.
        /// This is called automatically during destruction.
        ///
        /// @return std::expected<void, pool_error> indicating success or error
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Cleanup callback is invoked for each resource if provided
        /// @note Exceptions from cleanup callback are caught and logged
        auto clear() -> std::expected<void, pool_error>
        {
            std::scoped_lock l(m_pool_lock);

#if defined(DEBUG)
            std::cerr << std::format("{} - invoked; size:{} is shutdown? {}\n", __func__, m_pool.size(), m_is_shutdown.load());
#endif

            try {
                if (m_callback_on_resource_cleanup && !m_pool.empty()) {
                    while (!m_pool.empty()) {
                        RunOnEnd roe([&] { m_pool.pop_front(); });
                        // delegate to the cleanup.
                        // the delegate must not invoke any pool member to avoid deadlocks.
                        m_callback_on_resource_cleanup(std::move(m_pool.front()));
                    }
                }
            }
            catch (std::exception& ex) {
                std::cerr << std::format("{} - exception while delegating to on_cleanup: {}\n", __func__, ex.what());
            }

            m_pool.clear();

            return {};
        }

        /// @brief Gets the current size of the pool
        ///
        /// @return std::expected<size_t, pool_error> containing the pool size or error
        ///
        /// @note Thread-safe: Uses shared read lock
        /// @note Returns error if pool is shutting down
        [[nodiscard]] auto size() const -> std::expected<size_t, pool_error>
        {
            if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

            std::scoped_lock l(m_pool_lock);
            return m_pool.size();
        }

        /// @brief Borrows a resource from the pool
        ///
        /// Attempts to get a resource from the pool. If the pool is empty but under capacity,
        /// the factory callback is invoked to create a new resource. If the pool is exhausted
        /// and no factory callback is available, returns an error.
        ///
        /// @return std::expected<SRT, pool_error> containing the borrowed resource or error
        ///
        /// @note Thread-safe: Uses exclusive lock for pool access
        /// @note Factory callback is invoked outside the lock to prevent deadlocks
        /// @note Increments checkout counter
        /// @note Returns error if pool is shutting down
        ///
        /// @example
        /// @code
        /// auto resource = pool.borrow_from_pool();
        /// if (resource) {
        ///     // Use resource
        ///     resource->doSomething();
        /// } else {
        ///     // Handle error
        ///     std::cerr << "Failed to borrow resource" << std::endl;
        /// }
        /// @endcode
        [[nodiscard]] auto borrow_from_pool() -> std::expected<SRT, pool_error>
        {
            if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);


            // Create a guard to decrement m_resources_checkedout if the factory callback throws
            // This ensures we don't leak the borrow_from_pool count if the factory fails
            auto checkout_guard = [this]() {
                if (m_resources_checkedout > 0) {
                    m_resources_checkedout--;
                }
            };


            try {
                // @note We use a unique_lock vs a scoped_lock to allow ourselves
                // to create the resource outside the lock!
                std::unique_lock l(m_pool_lock);

                // Now that we're inside the lock, we should check again to ensure that
                // the shutdown is not in progress..
                if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

                if (!m_pool.empty()) {
                    // The pool is non-empty; return from the pool
                    // Return first element from the pool and pop it on scope end
                    RunOnEnd pop_guard([&]() {
                        m_pool.pop_front();
                        m_resources_checkedout++;
                        m_counter_borrows++;
                    });

                    // Make a wrapper..
                    // Create a SRT element and wire up the auto-return callback to return
                    // the resource back to this object.
                    return SRT {[this](T&& src, bool isvalid) -> std::expected<void, pool_error> {
                                    // this callback puts the resource back..
                                    return this->return_to_pool(std::forward<T&&>(src), isvalid);
                                },
                                std::move(m_pool.front())};
                    // Allow the compiler to use NRVO (move elision; do not use std::move here!)
                    // The pop_front() happens within this scope and within the lock!
                }
                else if (is_pool_starving() && m_callback_to_add_new_raw_resource_to_pool) {
                    // We have no more items in the pool (we're starting up or everything is
                    // checked out) but we have not reached the limit. The limit is number
                    // of m_resources_checkedout + pool.size() < m_capacity We are
                    // under-capacity.. so we can return to the caller a new item..
                    m_resources_checkedout++;

                    // We should unlock the resource and ..
                    l.unlock();

                    // Update the attempted delegated calls to add new raw resource to pool.

                    // ..delegate the new resource acquisition
                    // outside the lock.
                    return m_callback_to_add_new_raw_resource_to_pool(*this).and_then(
                            [&](auto item) -> std::expected<SRT, pool_error> {
                                // This is a borrow even though it was not from our pool..
                                m_counter_borrows++;
                                // Guarenteed to count after the invocation to the callback
                                m_counter_ondemand_adds++;
                                // We're not performing any change to the original, just return it back.
                                return item;
                            });
                }
                else if (is_pool_starving()) {
                    // We're under-capacity.. but no dynamic resource provider
                    return std::unexpected(siddiqsoft::arrp::pool_error::UnderCapacityNoAutoGrow);
                }
            } // scope end
            catch (std::exception& ex) {
                checkout_guard();
#if defined(DEBUG_TRACE)
                std::cerr << std::format("Error in borrow_from_pool: {}\n", ex.what());
#endif
                return std::unexpected(pool_error::Unknown);
            }
            catch (...) {
                checkout_guard();
                std::cerr << std::format("UNKNOWN Error in borrow_from_pool\n");
                return std::unexpected(pool_error::Unknown);
            }

            return std::unexpected(pool_error::NoMoreResources);
        }


        /// @brief Adds a resource to the pool by constructing it in-place
        ///
        /// @tparam Args Types of arguments to forward to T's constructor
        /// @param args Arguments to forward to T's constructor
        /// @return std::expected<void, pool_error> indicating success or error
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Resource is constructed in-place
        /// @note Decrements checkout counter
        /// @note Returns error if pool is shutting down
        template <typename... Args>
        auto add_to_pool(Args&&... args) -> std::expected<void, pool_error>
        {
            std::scoped_lock l(m_pool_lock);

            // Check inside the lock..
            if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

            m_pool.emplace_back(T {std::forward<Args&&>(args)...});
            m_counter_adds++;
            m_resources_checkedout--;
            m_capacity_poolsize++;

            if (m_capacity_poolsize.load() > m_capacity) m_capacity_poolsize = m_capacity;

            return {};
        }

        /// @brief Adds a resource to the pool by moving it
        ///
        /// @param item The resource to add (moved)
        /// @return std::expected<void, pool_error> indicating success or error
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Resource is moved into the pool
        /// @note Decrements checkout counter
        /// @note Returns error if pool is shutting down
        auto add_to_pool(T&& item) -> std::expected<void, pool_error>
        {
            std::scoped_lock l(m_pool_lock);

            // Check inside the lock..
            if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

            m_pool.emplace_back(std::move(item));
            m_counter_adds++;
            m_resources_checkedout--;
            m_capacity_poolsize++;

            if (m_capacity_poolsize.load() > m_capacity) m_capacity_poolsize = m_capacity;

            return {};
        }

    protected:
        /// @brief Returns a resource to the pool
        ///
        /// Called by scoped_resource destructor to return the resource to the pool.
        /// If the resource is valid, it's added back to the pool for reuse.
        /// If invalid, it's discarded and the abandoned counter is incremented.
        ///
        /// @param item The resource to return (moved)
        /// @param isvalid Whether the resource is valid and should be reused
        /// @return std::expected<void, pool_error> indicating success or error
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Increments appropriate counter (valid_returns or invalid_returns)
        /// @note Decrements checkout counter
        /// @note Returns error if pool is shutting down
        auto return_to_pool(T&& item, bool isvalid = true) -> std::expected<void, pool_error>
        {
            if (isvalid) {
                std::scoped_lock l(m_pool_lock);

                // Check inside the lock..
                if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

                m_pool.push_back(std::move(item));
                m_counter_returns++;
                m_resources_checkedout--;
                m_capacity_poolsize++;

                if (m_capacity_poolsize.load() > m_capacity) m_capacity_poolsize = m_capacity;
            } // lock scope end
            else {
                // Resource was invalidated; do not add back to the pool.
                // We need to decrement the borrow_from_pool count under lock to ensure thread safety
                std::scoped_lock l(m_pool_lock);

                // check inside the lock..
                if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

                m_abandoned++;
                m_resources_checkedout--;
            }

            return {};
        }

    public:
#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        /// @brief Serializes pool statistics to JSON
        ///
        /// Returns a JSON object containing pool statistics and configuration.
        /// Only available if nlohmann/json.hpp is included before this header.
        ///
        /// @return std::expected<std::reference_wrapper<nlohmann::json>, pool_error> containing JSON or error
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Returns error if pool is shutting down
        /// @note Requires NLOHMANN_JSON_VERSION_MAJOR to be defined
        ///
        /// @example
        /// @code
        /// auto json_result = pool.to_json();
        /// if (json_result) {
        ///     std::cout << json_result.value().get().dump(2) << std::endl;
        /// }
        /// @endcode
        auto to_json() -> std::expected<std::reference_wrapper<nlohmann::json>, siddiqsoft::arrp::pool_error>
        {
            {
                std::scoped_lock l(m_pool_lock);

                if (m_is_shutdown) return std::unexpected(siddiqsoft::arrp::pool_error::ShutdownInitiated);

                // Update the poolsize..
                m_json["size"]      = m_pool.size();
                m_json["deficit"]   = size_t(m_capacity) - m_pool.size();
                m_json["capsize"]   = m_capacity_poolsize.load();
                m_json["abandoned"] = m_abandoned.load();
                m_json["adds"]      = m_counter_adds.load();
                m_json["autoadds"]  = m_counter_ondemand_adds++;
                m_json["returns"]   = m_counter_returns.load();
                m_json["borrows"]   = m_counter_borrows.load();
                if constexpr (std::is_same_v<T, nlohmann::json> || std::is_same_v<T, std::string>) {
                    m_json["items"] = m_pool;
                }
            }

            return std::ref(m_json);
        }

    private:
        /// @brief JSON object for statistics
        nlohmann::json m_json {{"_typver", "siddiqsoft.arrp.resource_pool/0.0.0"}, {"capacity", m_capacity}};
#endif
    };

#if defined(NLOHMANN_JSON_VERSION_MAJOR)

    /// @brief Converts resource_pool to JSON
    /// @tparam T The resource type
    /// @tparam SRT The scoped resource type
    /// @param dest Destination JSON object
    /// @param src Source resource_pool
    template <typename T, typename SRT = scoped_resource<T>>
        requires NonNumericMoveConstructible<T> && std::derived_from<SRT, scoped_resource<T>>
    static void to_json(nlohmann::json& dest, const siddiqsoft::arrp::resource_pool<T, SRT>& src)
    {
        dest = src.to_json();
    }
#endif


} // namespace siddiqsoft::arrp
#endif


/// @brief Specialization of std::formatter for resource_pool
/// @details Provides formatted output for resource_pool instances using std::format
/// @tparam T The resource type
/// @tparam SRT The scoped resource type
/// @note Only available if nlohmann/json is included
template <typename T, typename SRT>
    requires siddiqsoft::arrp::NonNumericMoveConstructible<T> && std::derived_from<SRT, siddiqsoft::arrp::scoped_resource<T>>
struct std::formatter<siddiqsoft::arrp::resource_pool<T, SRT>> : std::formatter<char>
{
    /// @brief Parse format specification (empty for this type)
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    /// @brief Format the resource_pool
    /// @param pool The resource_pool to format
    /// @param ctx Format context
    /// @return Iterator to end of formatted output
    template <typename FormatContext>
    auto format(siddiqsoft::arrp::resource_pool<T, SRT>& pool, FormatContext& ctx) const
    {
#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        if (auto jv = pool.to_json(); jv.has_value()) {
            return std::format_to(ctx.out(), "{}", jv.value().get().dump());
        }

        return std::format_to(ctx.out(), "Error from to_json() invocation.");

#else
        return std::format_to(ctx.out(), "{{ json format requires nlohmann/json library }}");
#endif
    }
};
