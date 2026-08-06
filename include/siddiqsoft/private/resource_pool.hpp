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
#include <functional>
#include <semaphore>

#include "common.hpp"
#include "resource_guard.hpp"

namespace siddiqsoft::arrp
{
    /// @brief Thread-safe auto-returning resource pool
    ///
    /// @details
    /// Manages a pool of resources that are automatically returned when resource_guard
    /// instances are destroyed. Resources are seeded explicitly or created by an
    /// optional factory used by try_borrow_create().
    ///
    /// @tparam T The resource type (must be move-constructible and non-arithmetic)
    /// @tparam SRT The scoped resource type (defaults to resource_guard<T>)
    ///
    /// @note Pool storage is synchronized with a mutex. Configure the factory before
    ///       calling borrowing methods concurrently.
    /// @note RAII pattern: Resources are automatically returned to pool on destruction
    /// @note Callback-based: Supports factory, cleanup, and return callbacks
    /// @note Statistics: Tracks borrow/return operations and resource counts via atomic counters
    /// @note Move-only: Uses move semantics exclusively to prevent resource ownership ambiguity
    ///
    /// @example
    /// @code
    /// siddiqsoft::arrp::resource_pool<MyResource> pool(10);
    /// pool.seed(MyResource {});
    ///
    /// // Borrow a resource
    /// auto resource = pool.try_borrow();
    /// if (resource) {
    ///     resource->doSomething();
    /// }
    /// // Resource automatically returned to pool when resource_guard is destroyed
    /// @endcode
    template <typename T, typename SRT = resource_guard<T>>
        requires NonNumericMoveConstructible<T> && std::derived_from<SRT, resource_guard<T>>
    class resource_pool final
    {
    private:
        /// @brief Configured capacity reported in statistics
        /// @details Clamped to range [MinimumCapacity, MaxCapacity] during construction.
        /// This value does not limit seed() or factory-created resources.
        uint8_t m_capacity {0};

        /// @brief Flag indicating pool shutdown is in progress
        /// @details Set to true in destructor before cleanup begins
        std::atomic_bool m_is_shutdown {false};

#if defined(arrp_USE_RECURSIVE_MUTEX) || defined(ARRP_USE_RECURSIVE_MUTEX)
        /// @brief Mutex protecting access to the resource pool
        /// @details Uses a recursive mutex for tests which relax some deadlocks
        /// otherwise the CI will fail. It is also up to the user to ensure
        /// that they do not call methods that cause deadlocks.
        /// @note Marked as mutable to allow usage within const methods
        /// It might be more expensive but the client might find this useful!
        mutable std::recursive_mutex m_pool_lock {};
#else
        /// @brief Mutex protecting access to the resource pool
        /// @details Uses a standard mutex for optimal performance
        /// @note Marked as mutable to allow usage within const methods
        mutable std::mutex m_pool_lock {};
#endif

        /// @brief Internal deque storing the pooled resources
        /// @details Uses FIFO ordering: resources are added to back, retrieved from front
        std::deque<T> m_pool {};

        /// @brief Semaphore to track available resources in the pool
        std::counting_semaphore<> m_pool_semaphore {0};

        /// @brief Number of resources currently checked out from the pool
        /// @details Incremented when resources are borrowed and decremented when a
        /// guard returns or abandons a resource while the pool is alive.
        std::atomic_uint16_t m_resources_checkedout {0};

        /// @brief Number of resources that have been invalidated
        /// @details Tracks resources marked as invalid and not returned to pool
        std::atomic_uint16_t m_counter_abandons {0};

        /// @brief Peak counter tracking the maximum pool size reached
        /// @details Tracks the highest number of available resources observed.
        std::atomic_uint64_t m_peak_poolsize {0};

        /// @brief Counters for pool statistics
        /// @details m_counter_seeds: Resources added via seed()
        /// @details m_counter_ondemand_adds: Resources created on-demand via factory callback
        /// @details m_counter_returns: Resources returned to pool
        /// @details m_counter_borrows: Resources borrowed from pool
        std::atomic_uint64_t m_counter_seeds {0}, m_counter_ondemand_adds {0}, m_counter_returns {0}, m_counter_borrows {0};

        /// @brief Callback on resource cleanup during pool destruction
        /// @details Invoked under the pool lock for each resource available during
        /// clear() or destruction.
        /// @warning MUST NOT call any pool methods to avoid deadlock. Only perform cleanup operations.
        std::function<void(T&)> m_callback_on_resource_cleanup {};

        /// @brief Optional factory callback used to create resources on-demand
        /// @details Stored as a zero-argument callable returning `SRT`. A supplied
        /// callback may return either `T` or `SRT`.
        std::function<SRT()> m_factory_callback {};

        /// @brief Sets the capacity. This is internal and can only be called from the constructor.
        /// @details The configured capacity cannot be changed after construction.
        void set_capacity(uint8_t init_capacity)
        {
            if (m_capacity != 0) {
                return;
            }

            if (init_capacity > resource_pool_limits::MaxCapacity) {
                m_capacity = resource_pool_limits::MaxCapacity;
            }
            else if (init_capacity < resource_pool_limits::MinimumCapacity) {
                m_capacity = resource_pool_limits::MinimumCapacity;
            }
            else {
                m_capacity = init_capacity;
            }
        }

        /// @brief Shared lock type for pool operations
        using lock_type        = std::scoped_lock<decltype(m_pool_lock)>;
        using unique_lock_type = std::unique_lock<decltype(m_pool_lock)>;

        /// @brief Checks whether total resources are below the configured capacity
        /// @return true when available plus checked-out resources is below capacity
        inline bool is_pool_starving() const { return m_resources_checkedout.load() + m_pool.size() < m_capacity; }

        /// @brief Checks if there is a deficit between configured capacity and current resources
        /// @return true if deficit exists, false otherwise
        inline auto is_there_a_pool_deficit() const { return deficit_size() != 0; }

        /// @brief Calculates the difference between configured and current resources
        /// @return `capacity - (available resources + checked-out resources)`.
        ///         Negative values indicate that more resources were added than capacity.
        inline int64_t deficit_size() const
        {
            return static_cast<int64_t>(m_capacity) - (static_cast<int64_t>(m_pool.size()) + m_resources_checkedout.load());
        }

        /// @brief The loan size is the difference between the borrows and returns and accounting for the abandons.
        /// @details We're trying to ensure that we have a zero-balance of try_borrow() and the return_to_pool()
        /// calls by the client.
        /// @return A value representing the number of currently "borrowed" resources by the client.
        inline auto loan_size() const
        {
            auto loans = m_counter_borrows.load(); // total number of borrows (current counter)
            loans -= m_counter_returns.load();     // total number of returns (current counter)
            loans -= m_counter_abandons.load();    // adjust for any abandons
            return loans;
        }

    public:
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


        /// @brief Constructs a resource pool with an optional cleanup callback
        ///
        /// @param init_capacity Initial capacity of the pool
        /// @param on_shutdown_callback Optional cleanup callback invoked on destruction
        ///
        /// @note Register a factory separately with set_factory_callback().
        /// @note Capacity is clamped to [MinimumCapacity, MaxCapacity] but does not
        ///       enforce a maximum number of seeded or factory-created resources.
        resource_pool(uint8_t                   init_capacity        = resource_pool_limits::DefaultCapacity,
                      std::function<void(T&)>&& on_shutdown_callback = {})
            : m_callback_on_resource_cleanup(std::move(on_shutdown_callback))
        {
            set_capacity(init_capacity);
        }

        /// @brief Constructs a resource pool with only cleanup callback
        ///
        /// @param on_shutdown_callback Cleanup callback invoked on destruction
        ///
        /// @note Uses the default capacity. The cleanup callback is invoked for
        ///       resources available during clear() or destruction.
        resource_pool(std::function<void(T&)>&& on_shutdown_callback)
            : m_callback_on_resource_cleanup(std::move(on_shutdown_callback))
        {
            set_capacity(resource_pool_limits::DefaultCapacity);
        }


        /// @brief Destructor - cleans up all resources in the pool
        ///
        /// Sets the shutdown flag and delegates to clear() to clean up resources.
        /// The cleanup callback (if provided) is invoked for each resource during cleanup.
        ///
        /// @note Exceptions derived from std::exception in the cleanup callback are
        ///       caught and written to stderr.
        /// @warning Guards borrowed from this pool must be destroyed before the pool.
        ~resource_pool()
        {
            {
                std::scoped_lock l(m_pool_lock);
                m_is_shutdown = true;
            }
#if defined(DEBUG)
            std::print(std::cerr, "{} - invoked; shutdown set; now delegating to clear..\n", __func__);
#endif
            // Delegate to the clear() method which itself acquires a lock
            // so we should make sure we clear the lock to set the shutdown flag.
            this->clear();
        }

    protected:
        /// @brief Creates a scoped resource wired to return to this pool on destruction.
        /// @note The callback constructor of resource_guard is private; this helper
        ///       supplies the callback for the default scoped-resource type.
        template <typename... Args>
        auto make_resource_guard(Args&&... args) -> SRT
        {
            // Allow the compiler to use NRVO (move elision; do not use std::move here!)
            return SRT {[this](T&& src, bool isvalid) {
                            // this callback puts the resource back..
                            return this->return_to_pool(std::forward<T>(src), isvalid);
                        },
                        std::forward<Args>(args)...};
        }

        /// @brief Function type for a callback that produces a scoped resource.
        /// @tparam Args Callback argument types.
        template <typename... Args>
        using resource_callback_t = std::function<SRT(Args...)>;

        /// @brief Invokes a callback and converts its result to the scoped resource type.
        /// @tparam F Callable type returning T or SRT.
        /// @tparam Args Argument types passed to the callable.
        /// @param f Callback to invoke.
        /// @param args Arguments passed to the callback.
        /// @return The callback's scoped resource, or a guard created around its T result.
        template <typename F, typename... Args>
        auto create_from_callback(F&& f, Args&&... args) -> SRT
        {
            using result_t = std::invoke_result_t<F, Args...>;

            if constexpr (std::is_same_v<result_t, SRT>) {
                return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
            }
            else if constexpr (std::is_same_v<result_t, T>) {
                return make_resource_guard(std::invoke(std::forward<F>(f), std::forward<Args>(args)...));
            }
            else {
                static_assert(std::is_same_v<result_t, SRT> || std::is_same_v<result_t, T>, "Callback must return either SRT or T");
            }
        }

    public:
        /// @brief Sets the factory used by try_borrow_create() when no resource is available.
        /// @tparam F Callable type invokable with no arguments returning `SRT` or `T`
        /// @warning Configure the factory before concurrent borrow attempts. The
        ///          callback must not call methods on this pool.
        template <typename F>
        void set_factory_callback(F&& f)
        {
            auto factory       = std::forward<F>(f);
            m_factory_callback = [this, factory = std::move(factory)]() mutable -> SRT {
                return create_from_callback(factory);
            };
        }

        /// @brief Clears all resources from the pool
        ///
        /// Removes currently available resources and invokes the cleanup callback for each.
        /// Borrowed resources can return after clear() completes.
        ///
        /// @return pool_error::Ok
        ///
        /// @note The cleanup callback runs under the pool lock. Exceptions derived
        ///       from std::exception are caught and written to stderr.
        auto clear() -> pool_error
        {
            std::scoped_lock l(m_pool_lock);

            try {
                // Drain semaphore and pool together so their counts stay in sync.
                // The cleanup callback, if set, runs under the lock per item.
                while (!m_pool.empty()) {
                    m_pool_semaphore.acquire();
                    auto item = std::move(m_pool.front());
                    m_pool.pop_front();
                    if (m_callback_on_resource_cleanup) {
                        // delegate to the cleanup.
                        // the delegate must not invoke any pool member to avoid deadlocks.
                        m_callback_on_resource_cleanup(item);
                    }
                }
            }
            catch (std::exception& ex) {
                std::print(std::cerr, "{} - exception while delegating to on_cleanup: {}\n", __func__, ex.what());
            }

            return pool_error::Ok;
        }

        /// @brief Gets the current size of the pool
        ///
        /// @return Number of currently available resources
        ///
        /// @note Does not include checked-out resources
        [[nodiscard]] auto size() const
        {
            std::scoped_lock l(m_pool_lock);
            return m_pool.size();
        }

    protected:
        /// @brief Borrows a resource from the pool
        ///
        /// Attempts to get a resource from the pool. When creation is requested and
        /// no resource is available, invokes the registered factory callback.
        ///
        /// @return SRT containing the borrowed resource or error
        ///
        /// @note A positive timeout waits for the availability semaphore; zero does
        ///       not wait. The factory must not call pool methods.
        ///
        /// @example
        /// @code
        /// auto resource = pool.try_borrow();
        /// if (resource) {
        ///     // Use resource
        ///     resource->doSomething();
        /// } else {
        ///     // Handle error
        ///     std::print(std::cerr, "Failed to borrow resource\n");
        /// }
        /// @endcode
        [[nodiscard]] auto borrow_impl(std::chrono::nanoseconds timeout = {}, bool createIfEmptyTimeout = false) -> SRT
        {
            try {
                // @note We use a unique_lock vs a scoped_lock to allow ourselves
                // to create the resource outside the lock!

                // Fast pre-check — the authoritative shutdown check is performed
                // under the lock below after semaphore acquisition.
                if (m_is_shutdown) return SRT {pool_error::ShutdownInitiated};

                // Wait for an available resource, then optionally create one when
                // try_borrow_create() was requested.
                if (timeout.count() == 0 ? m_pool_semaphore.try_acquire() : m_pool_semaphore.try_acquire_for(timeout)) {
                    // We likely have a resource available.. grab a lock.
                    std::unique_lock l(m_pool_lock);

                    // Authoritative shutdown check under the lock: the destructor may
                    // have run between the pre-semaphore check and taking this lock.
                    if (m_is_shutdown) {
                        m_pool_semaphore.release();
                        return SRT {pool_error::ShutdownInitiated};
                    }

                    // We have a resource available.. just to be sure, we'll check the pool size inside the lock..
                    if (!m_pool.empty()) {
                        // Move the resource out of the pool while holding the lock.
                        auto borrowed = make_resource_guard(std::move(m_pool.front()));
                        // Remove from the deque.. only one client may have exclusive use..
                        m_pool.pop_front();

                        // Release the lock before constructing the scoped wrapper
                        // and updating the borrow counters.
                        l.unlock();

                        m_resources_checkedout++;
                        m_counter_borrows++;
                        return borrowed;
                    }
                    else if (createIfEmptyTimeout && m_factory_callback) {
                        std::println(std::cerr, "{} - Empty pool; asked to create new if empty..", __func__);
                        // Release the lock before invoking arbitrary user code to
                        // prevent deadlock if the factory calls back into the pool.
                        auto cb = m_factory_callback;
                        l.unlock();
                        auto ondemand = create_from_callback(cb);
                        m_counter_ondemand_adds++;
                        m_resources_checkedout++;
                        m_counter_borrows++;
                        return ondemand;
                    }
                    else {
                        // Permit was acquired but no resource exists; release it so
                        // the semaphore count stays in sync with the pool size.
                        m_pool_semaphore.release();
                        return SRT {siddiqsoft::arrp::pool_error::NoMoreResources};
                    }
                }
                else if (createIfEmptyTimeout && m_factory_callback) {
                    std::println(std::cerr, "{} - We exhausted timeout; asked to create new if empty..", __func__);
                    // No lock needed here — factory is invoked without holding m_pool_lock
                    // to avoid deadlock if the factory calls back into the pool.
                    auto ondemand = create_from_callback(m_factory_callback);
                    m_counter_ondemand_adds++;
                    m_resources_checkedout++;
                    m_counter_borrows++;
                    return ondemand;
                }
                else if (timeout.count() > 0) {
                    return SRT {siddiqsoft::arrp::pool_error::Timeout};
                }
                else {
                    return SRT {siddiqsoft::arrp::pool_error::NoMoreResources};
                }
            } // scope end
            catch (std::exception& ex) {
                return SRT {siddiqsoft::arrp::pool_error::Unknown};
            }
            catch (...) {
                std::println(std::cerr, "UNKNOWN Error in borrow");
                return SRT {siddiqsoft::arrp::pool_error::Unknown};
            }

            return SRT {siddiqsoft::arrp::pool_error::NoMoreResources};
        }

    public:
        /// @brief Borrows an available resource without creating one.
        /// @param timeout Maximum time to wait; zero performs a non-blocking attempt.
        /// @return A valid scoped resource, or an invalid one with NoMoreResources,
        ///         Timeout, ShutdownInitiated, or Unknown set as its error.
        [[nodiscard]] auto try_borrow(std::chrono::nanoseconds timeout = {}) -> SRT { return borrow_impl(timeout); }

        /// @brief Borrows an available resource or creates one through the factory.
        /// @param timeout Maximum time to wait; zero performs a non-blocking attempt.
        /// @return A valid scoped resource, or an invalid one when shutdown or an
        ///         implementation or factory error prevents borrowing.
        [[nodiscard]] auto try_borrow_create(std::chrono::nanoseconds timeout = {}) -> SRT { return borrow_impl(timeout, true); }

        /// @brief Adds a resource to the pool by constructing it in-place
        ///
        /// @tparam Args Types of arguments to forward to T's constructor
        /// @param args Arguments to forward to T's constructor for in-place construction
        /// @return pool_error::Ok, or pool_error::ShutdownInitiated during destruction
        ///
        /// @note Resource is constructed in-place
        /// @note Does not enforce the configured capacity. Do not use this to return
        ///       a borrowed resource; guards return resources automatically.
        template <typename... Args>
        auto seed(Args&&... args) -> pool_error
        {
            {
                std::scoped_lock l(m_pool_lock);

                // Check inside the lock..
                if (m_is_shutdown) return pool_error::ShutdownInitiated;

                m_pool.emplace_back(std::forward<Args>(args)...);
                m_counter_seeds++;

                // Update peak pool size for statistics
                auto current_size = m_pool.size();
                if (current_size > m_peak_poolsize.load()) {
                    m_peak_poolsize = current_size;
                }
            }

            // Signal the semaphore outside the lock to avoid potential deadlocks.
            m_pool_semaphore.release(); // resource is available, increment semaphore

            return pool_error::Ok;
        }

        /// @brief Adds a resource to the pool by moving it
        ///
        /// @param item The resource to add (moved)
        /// @return pool_error::Ok, or pool_error::ShutdownInitiated during destruction
        ///
        /// @note Resource is moved into the pool
        /// @note Does not enforce the configured capacity. Do not use this to return
        ///       a borrowed resource; guards return resources automatically.
        auto seed(T&& item) -> pool_error
        {
            {
                std::scoped_lock l(m_pool_lock);

                // Check inside the lock..
                if (m_is_shutdown) return pool_error::ShutdownInitiated;

                m_pool.emplace_back(std::move(item));
                m_counter_seeds++;

                // Update peak pool size for statistics
                auto current_size = m_pool.size();
                if (current_size > m_peak_poolsize.load()) {
                    m_peak_poolsize = current_size;
                }
            }

            // Signal the semaphore outside the lock to avoid potential deadlocks.
            m_pool_semaphore.release(); // resource is available, increment semaphore

            return pool_error::Ok;
        }


    protected:
        /// @brief Returns a resource to the pool
        ///
        /// Called by resource_guard destructor to return the resource to the pool.
        /// If the resource is valid, it's added back to the pool for reuse.
        /// If invalid, it's discarded and the abandoned counter is incremented.
        ///
        /// @param item The resource to return (moved)
        /// @param isvalid Whether the resource is valid and should be reused
        /// @note Valid resources increment returns; invalid resources increment
        ///       abandons. Returns after shutdown are discarded.
        void return_to_pool(T&& item, bool isvalid)
        {
            std::unique_lock l(m_pool_lock);

            // Always decrement the checkout counter — the resource is leaving our
            // custody regardless of whether the pool is still alive.
            m_resources_checkedout--;

            // Check inside the lock..
            if (m_is_shutdown) return;

            if (isvalid) {
                m_pool.push_back(std::move(item));
                m_counter_returns++;

                // Update peak pool size for statistics
                auto current_size = m_pool.size();
                if (current_size > m_peak_poolsize.load()) {
                    m_peak_poolsize = current_size;
                }

                l.unlock();
                // Signal the semaphore outside the lock to avoid potential deadlocks.
                if (isvalid) m_pool_semaphore.release(); // resource is available, increment semaphore
            }
            else {
                m_counter_abandons++;
            }
        }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
    public:
        /// @brief Serializes pool statistics to JSON
        ///
        /// Returns a JSON object containing pool statistics and configuration.
        /// Only available if nlohmann/json.hpp is included before this header file.
        ///
        /// @return A JSON object containing a snapshot of pool statistics
        ///
        /// @note Available only when nlohmann/json.hpp was included before this header.
        ///
        /// @par JSON Schema:
        /// @code{.json}
        /// {
        ///   "_typver": "siddiqsoft.arrp.resource_pool/0.0.0",
        ///   "capacity": <configured_capacity>,
        ///   "size": <available_resources>,
        ///   "deficit": <resources_needed>,
        ///   "peaksize": <peak_size_reached>,
        ///   "abandons": <invalidated_resources>,
        ///   "seeds": <resources_added_via_seed>,
        ///   "autoadds": <resources_created_ondemand>,
        ///   "returns": <resources_returned>,
        ///   "borrows": <resources_borrowed>,
        ///   "loans": <currently_borrowed>
        /// }
        /// @endcode
        ///
        /// @example
        /// @code
        /// auto stats = pool.to_json();
        /// std::cout << stats.dump(2) << '\n';
        /// @endcode
        auto to_json() const -> nlohmann::json
        {
            nlohmann::json   stats;
            std::scoped_lock l(m_pool_lock);

            // Update the pool statistics
            stats["_typver"]  = "siddiqsoft.arrp.resource_pool/0.0.0"; ///< Type and version number of the class
            stats["size"]     = m_pool.size();                         ///< Available resources in pool
            stats["deficit"]  = deficit_size();                        ///< Resources needed to reach capacity
            stats["capacity"] = m_capacity;                            ///< Maximum resources
            stats["peaksize"] = m_peak_poolsize.load();                ///< Peak pool size reached
            stats["abandons"] = m_counter_abandons.load();             ///< Invalidated resources
            stats["seeds"]    = m_counter_seeds.load();                ///< Resources added via seed()
            stats["autoadds"] = m_counter_ondemand_adds.load();        ///< Resources created on-demand
            stats["returns"]  = m_counter_returns.load();              ///< Resources returned to pool
            stats["borrows"]  = m_counter_borrows.load();              ///< Resources borrowed from pool
            stats["loans"]    = loan_size();                           ///< Currently borrowed resources

            // This field is only available when there is a supported data-type
            if constexpr (std::is_same_v<T, nlohmann::json> || std::is_same_v<T, std::string> || std::is_arithmetic_v<T>) {
                stats["items"] = m_pool;
            }

            return stats;
        }
#endif
    };

#if defined(NLOHMANN_JSON_VERSION_MAJOR)

    /// @brief Converts resource_pool to JSON
    /// @tparam T The resource type
    /// @tparam SRT The scoped resource type
    /// @param dest Destination JSON object
    /// @param src Source resource_pool
    template <typename T, typename SRT = resource_guard<T>>
        requires NonNumericMoveConstructible<T> && std::derived_from<SRT, resource_guard<T>>
    static void to_json(nlohmann::json& dest, const siddiqsoft::arrp::resource_pool<T, SRT>& src)
    {
        dest = src.to_json();
    }
#endif


} // namespace siddiqsoft::arrp
#endif


template <typename T, typename SRT>
    requires siddiqsoft::arrp::NonNumericMoveConstructible<T> && std::derived_from<SRT, siddiqsoft::arrp::resource_guard<T>>
struct std::formatter<siddiqsoft::arrp::resource_pool<T, SRT>> : std::formatter<std::string>
{
    auto format(const siddiqsoft::arrp::resource_pool<T, SRT>& pool, auto& ctx) const
    {
#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        return std::format_to(ctx.out(), "{}", pool.to_json().dump());
#else
        return std::format_to(ctx.out(), "--to--be--implemented--");
#endif
    }
};
