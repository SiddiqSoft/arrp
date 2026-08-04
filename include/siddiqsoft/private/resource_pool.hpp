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
#include <semaphore>

#include "common.hpp"
#include "scoped_resource.hpp"
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
    /// @note Thread-safe: All operations are protected by mutex (std::mutex or std::recursive_mutex)
    /// @note RAII pattern: Resources are automatically returned to pool on destruction
    /// @note Callback-based: Supports factory, cleanup, and return callbacks
    /// @note Statistics: Tracks borrow/return operations and resource counts via atomic counters
    /// @note Move-only: Uses move semantics exclusively to prevent resource ownership ambiguity
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
        /// @details Clamped to range [MinimumCapacity, MaxCapacity] during construction
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
        /// @details Uses unsigned type to prevent negative values that could break pool logic.
        /// Incremented when resources are borrowed, decremented when returned.
        std::atomic_uint16_t m_resources_checkedout {0};

        /// @brief Number of resources that have been invalidated
        /// @details Tracks resources marked as invalid and not returned to pool
        std::atomic_uint16_t m_counter_abandons {0};

        /// @brief Peak counter tracking the maximum pool size reached
        /// @details Tracks the highest number of resources that have been in the pool at any time.
        /// This is clamped to m_capacity and used for statistics/monitoring purposes.
        std::atomic_uint64_t m_peak_poolsize {0};

        /// @brief Counters for pool statistics
        /// @details m_counter_seeds: Resources added via seed_to_pool()
        /// @details m_counter_ondemand_adds: Resources created on-demand via factory callback
        /// @details m_counter_returns: Resources returned to pool
        /// @details m_counter_borrows: Resources borrowed from pool
        std::atomic_uint64_t m_counter_seeds {0}, m_counter_ondemand_adds {0}, m_counter_returns {0}, m_counter_borrows {0};

        /// @brief Callback on resource cleanup during pool destruction
        /// @details This method is invoked within a lock and inside of a for-loop across each
        /// resource in the internal deque.
        /// This approach allows the client to perform any final cleanup for the given resource.
        /// @warning MUST NOT call any pool methods to avoid deadlock. Only perform cleanup operations.
        std::function<void(T&&)> m_callback_on_resource_cleanup {};

        /// @brief Sets the capacity. This is internal and can only be called from the constructor.
        /// @details The capacity of the internal queue must not be altered once set.
        void set_capacity(uint8_t init_capacity)
        {
#if defined(DEBUG)
            std::print(std::cerr, "{} - capacity: {}  init_capacity:{}\n", __func__, m_capacity, init_capacity);
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
#if defined(DEBUG)
                std::print(std::cerr, "{} - capacity: {}  init_capacity:{}\n", __func__, m_capacity, init_capacity);
#endif
            }
        }

        /// @brief Checks if the pool is starving (under capacity)
        /// @details Returns true if the total number of resources (in pool + checked out) is less than capacity
        /// @return true if pool is under capacity, false otherwise
        inline bool is_pool_starving() const { return m_resources_checkedout.load() + m_pool.size() < m_capacity; }

        /// @brief Checks if there is a deficit between configured capacity and current resources
        /// @return true if deficit exists, false otherwise
        inline auto is_there_a_pool_deficit() const { return deficit_size() != 0; }

        /// @brief Calculates the deficit between configured capacity and current total resources
        /// @details Computes how many more resources are needed to reach the configured capacity.
        /// The deficit is: capacity - (pool_size + checked_out_resources)
        /// @return Positive value indicates resources needed to reach capacity, zero means at capacity,
        ///         negative value indicates over-capacity (should not normally occur)
        inline int64_t deficit_size() const
        {
            return static_cast<int64_t>(m_capacity) - (static_cast<int64_t>(m_pool.size()) + m_resources_checkedout.load());
        }

        /// @brief The loan size is the difference between the borrows and returns and accounting for the abandons.
        /// @details We're trying to ensure that we have a zero-balance of borrow_from_pool() and the return_to_pool()
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


        /// @brief Constructs a resource pool with factory and cleanup callbacks
        ///
        /// @param init_capacity Initial capacity of the pool
        /// @param new_resource_callback Factory callback to create new resources
        /// @param on_shutdown_callback Optional cleanup callback invoked on destruction
        ///
        /// @note The factory callback is required and must return SRT. It must NOT call pool methods.
        /// @note The cleanup callback is optional and invoked for each resource during destruction
        /// @note Capacity is clamped to valid range [MinimumCapacity, MaxCapacity]
        resource_pool(uint8_t                    init_capacity        = resource_pool_limits::DefaultCapacity,
                      std::function<void(T&&)>&& on_shutdown_callback = {})
            : m_callback_on_resource_cleanup(std::move(on_shutdown_callback))
        {
            set_capacity(init_capacity);
        }

        /// @brief Constructs a resource pool with only cleanup callback
        ///
        /// @param on_shutdown_callback Cleanup callback invoked on destruction
        ///
        /// @note Uses default capacity and no auto-grow policy
        /// @note The cleanup callback is optional and invoked for each resource during destruction
        resource_pool(std::function<void(T&&)>&& on_shutdown_callback)
            : m_callback_on_resource_cleanup(std::move(on_shutdown_callback))
        {
            set_capacity(resource_pool_limits::DefaultCapacity);
        }


        /// @brief Destructor - cleans up all resources in the pool
        ///
        /// Sets the shutdown flag and delegates to clear() to clean up resources.
        /// The cleanup callback (if provided) is invoked for each resource during cleanup.
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
            std::print(std::cerr, "{} - invoked; shutdown set; now delegating to clear..\n", __func__);
#endif
            // Delegate to the clear() method which itself acquires a lock
            // so we should make sure we clear the lock to set the shutdown flag.
            this->clear();
        }


        /// @brief Create a resource that is wired to invoke the return_to_pool in the destructor of the SRT class.
        /// @note The scoped_resource<T> cannot be directly instantiated and thus this method is the only means
        /// to create a custom resource.
        /// This approach also solves the issue where we hide the return_to_pool() as protected and making the
        /// resource_pool and scoped_resource friends.
        template <typename... Args>
        auto create_resource(Args&&... args) -> SRT
        {
            // Allow the compiler to use NRVO (move elision; do not use std::move here!)
            return SRT {[this](T&& src, bool isvalid) {
                            // this callback puts the resource back..
                            return this->return_to_pool(std::forward<T>(src), isvalid);
                        },
                        std::forward<Args>(args)...};
        }


        /// @brief Clears all resources from the pool
        ///
        /// Removes all resources from the pool and invokes the cleanup callback for each.
        /// This is called automatically during destruction. Can also be called manually.
        ///
        /// @return std::expected<void, pool_error> indicating success or error
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Cleanup callback is invoked for each resource if provided
        /// @note Exceptions from cleanup callback are caught and logged
        auto clear() -> pool_error
        {
            std::scoped_lock l(m_pool_lock);

            try {
                if (m_callback_on_resource_cleanup && !m_pool.empty()) {
                    while (!m_pool.empty()) {
                        RunOnEnd roe([&] {
                            m_pool.pop_front();
                            m_pool_semaphore.acquire();
                        });
                        // delegate to the cleanup.
                        // the delegate must not invoke any pool member to avoid deadlocks.
                        m_callback_on_resource_cleanup(std::move(m_pool.front()));
                    }
                }
            }
            catch (std::exception& ex) {
                std::print(std::cerr, "{} - exception while delegating to on_cleanup: {}\n", __func__, ex.what());
            }

            // This will clear the pool (of any remaining resources) and reset the size to zero.
            m_pool.clear();

            return pool_error::Ok;
        }

        /// @brief Gets the current size of the pool
        ///
        /// @return std::expected<size_t, pool_error> containing the number of available resources or error
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Returns error if pool is shutting down
        /// @note Does not include checked-out resources
        [[nodiscard]] auto size() const
        {
            std::scoped_lock l(m_pool_lock);
            return m_pool.size();
        }


        /// @brief Borrows a resource from the pool
        ///
        /// Attempts to get a resource from the pool. If the pool is empty but under capacity,
        /// the factory callback is invoked to create a new resource on-demand. If the pool is exhausted
        /// and no factory callback is available, returns an error.
        ///
        /// @return SRT containing the borrowed resource or error
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
        ///     std::print(std::cerr, "Failed to borrow resource\n");
        /// }
        /// @endcode
        [[nodiscard]] auto borrow_from_pool(std::chrono::nanoseconds timeout = {}) -> SRT
        {
            try {
                // @note We use a unique_lock vs a scoped_lock to allow ourselves
                // to create the resource outside the lock!

                // Now that we're inside the lock, we should check again to ensure that
                // the shutdown is not in progress..
                if (m_is_shutdown) return SRT {pool_error::ShutdownInitiated};

                // If the pool is empty, we can check if we're under capacity and if so, we can create a new resource on-demand.
                if (timeout.count() == 0 ? m_pool_semaphore.try_acquire() : m_pool_semaphore.try_acquire_for(timeout)) {
                    // We likely have a resource available.. grab a lock.
                    std::unique_lock l(m_pool_lock);
                    // We have a resource available.. just to be sure, we'll check the pool size inside the lock..
                    if (!m_pool.empty()) {
                        // Move the resource out of the pool while holding the lock.
                        T resource = std::move(m_pool.front());
                        m_pool.pop_front();

                        // Release the lock before constructing the scoped wrapper
                        // and updating the borrow counters.
                        l.unlock();

                        auto borrowed = create_resource(std::move(resource));
                        m_resources_checkedout++;
                        m_counter_borrows++;
                        return borrowed;
                    }
                    else {
                        return SRT {siddiqsoft::arrp::pool_error::NoMoreResources};
                    }
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
                std::print(std::cerr, "UNKNOWN Error in borrow_from_pool\n");
                return SRT {siddiqsoft::arrp::pool_error::Unknown};
            }

            return SRT {siddiqsoft::arrp::pool_error::NoMoreResources};
        }

        /// @brief Adds a resource to the pool by constructing it in-place
        ///
        /// @tparam Args Types of arguments to forward to T's constructor
        /// @param args Arguments to forward to T's constructor for in-place construction
        /// @return std::expected<void, pool_error> indicating success or error
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Resource is constructed in-place
        /// @note Returns error if pool is shutting down
        /// @note This method MUST NOT be invoked to "return"; use the return_to_pool() method otherwise the accounting and resource
        /// management will not work properly!
        template <typename... Args>
        auto seed_to_pool(Args&&... args) -> pool_error
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
        /// @return std::expected<void, pool_error> indicating success or error. Returns error if pool is shutting down.
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Resource is moved into the pool
        /// @note Returns error if pool is shutting down
        /// @note This method MUST NOT be invoked to "return"; use the return_to_pool() method otherwise the accounting and resource
        /// management will not work properly!
        auto seed_to_pool(T&& item) -> pool_error
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
        /// @note Returns if pool is shutting down
        /// @note This method MUST be invoked to "return" the resource back to pool otherwise the accounting and resource
        /// management will not work properly!
        void return_to_pool(T&& item, bool isvalid)
        {
            {
                std::unique_lock l(m_pool_lock);

                // Check inside the lock..
                if (m_is_shutdown) return;

                m_resources_checkedout--;

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
        }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
    public:
        /// @brief Serializes pool statistics to JSON
        ///
        /// Returns a JSON object containing pool statistics and configuration.
        /// Only available if nlohmann/json.hpp is included before this header file.
        ///
        /// @return std::expected<std::reference_wrapper<nlohmann::json>, pool_error> containing JSON or error
        ///
        /// @note Thread-safe: Uses exclusive lock
        /// @note Returns error if pool is shutting down
        /// @note Requires NLOHMANN_JSON_VERSION_MAJOR to be defined
        ///
        /// @par JSON Schema:
        /// @code{.json}
        /// {
        ///   "_typver": "siddiqsoft.arrp.resource_pool/0.0.0",
        ///   "capacity": <max_resources>,
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
        /// auto json_result = pool.to_json();
        /// if (json_result) {
        ///     std::cout << json_result.value().get().dump(2) << std::endl;
        /// }
        /// @endcode
        auto to_json() const -> nlohmann::json
        {
            nlohmann::json   stats;
            std::scoped_lock l(m_pool_lock);

            // Update the pool statistics
            stats["size"]     = m_pool.size();                  ///< Available resources in pool
            stats["deficit"]  = deficit_size();                 ///< Resources needed to reach capacity
            stats["capacity"] = m_capacity;                     ///< Maximum resources
            stats["peaksize"] = m_peak_poolsize.load();         ///< Peak pool size reached
            stats["abandons"] = m_counter_abandons.load();      ///< Invalidated resources
            stats["seeds"]    = m_counter_seeds.load();         ///< Resources added via seed_to_pool()
            stats["autoadds"] = m_counter_ondemand_adds.load(); ///< Resources created on-demand
            stats["returns"]  = m_counter_returns.load();       ///< Resources returned to pool
            stats["borrows"]  = m_counter_borrows.load();       ///< Resources borrowed from pool
            stats["loans"]    = loan_size();                    ///< Currently borrowed resources

            // This field is only available when there is a supported data-type
            if constexpr (std::is_same_v<T, nlohmann::json> || std::is_same_v<T, std::string>) {
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
    template <typename T, typename SRT = scoped_resource<T>>
        requires NonNumericMoveConstructible<T> && std::derived_from<SRT, scoped_resource<T>>
    static void to_json(nlohmann::json& dest, const siddiqsoft::arrp::resource_pool<T, SRT>& src)
    {
        dest = src.to_json();
    }
#endif


} // namespace siddiqsoft::arrp
#endif


template <typename T, typename SRT>
    requires siddiqsoft::arrp::NonNumericMoveConstructible<T> && std::derived_from<SRT, siddiqsoft::arrp::scoped_resource<T>>
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
