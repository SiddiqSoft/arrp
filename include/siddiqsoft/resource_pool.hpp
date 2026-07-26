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
#include <memory>
#include <expected>

#include "private/common.hpp"
#include "private/scoped_resource.hpp"
#include "siddiqsoft/RunOnEnd.hpp"

namespace siddiqsoft::arrp
{
    template <typename T, typename SRT = scoped_resource<T>>
        requires NonNumericMoveConstructible<T> && std::derived_from<SRT, scoped_resource<T>>
    class resource_pool
    {
    private:
        /// @brief Maximum number of resources that can be in the pool
        uint8_t          m_capacity {0};

        std::atomic_bool m_is_shutdown {false};

        /// @brief Number of resources currently checked out from the pool
        std::atomic_int16_t m_resources_checkedout {0};

        /// @brief Number of resources that have been invalidated
        /// @note Currently unused; reserved for future use
        std::atomic_uint16_t m_abandoned {0};

        /// @brief Counter for resources successfully returned to the pool
        /// @details Tracks how many times a resource was returned and added back to the pool
        std::atomic_uint64_t m_counter_valid_returns {0};

        /// @brief Counter for resources returned as invalid during this session
        /// @details Tracks how many times a resource was returned with an abandoned/invalid reason
        std::atomic_uint64_t m_counter_invalid_returns {0};

        /// @brief Counter for borrow operations from the pool
        /// @note Only counts successful borrow operations
        std::atomic_uint64_t m_counter_checkout {0};

        /// @brief Counter for ondemand resource additions via factory callback
        std::atomic_uint64_t m_counter_ondemand_adds {0};

        /// @brief Counter for return operations to the pool
        std::atomic_uint64_t m_counter_checkin {0};

        std::atomic_uint64_t m_capacity_poolsize {0};

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

        /// @brief Sets the capacity. This is internal and can only be called from the constructor.
        /// The capacity of the internal queue must not be altered once set.
        void set_capacity(uint8_t init_capacity)
        {
#if defined(DEBUG)
            std::cerr << std::format("{} - capacity: {}  init_capacity:{}\n", __func__, m_capacity, init_capacity);
#endif

            // We're going to be inside construction context and we're assured
            // of only one inovcation!
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
        /// @brief This callback is the default and does not grow the resource; it throws a runtime_error
        static inline std::function<std::expected<SRT, pool_error>(resource_pool&)> CallbackDoNotAutoAddResource =
                [](resource_pool&) -> std::expected<SRT, pool_error> {
            return std::unexpected(pool_error::NoMoreResources);
        };

        resource_pool(uint8_t init_capacity, std::function<std::expected<SRT, pool_error>(resource_pool&)>&& new_resource_callback)
            : m_callback_to_add_new_raw_resource_to_pool(new_resource_callback ? std::move(new_resource_callback)
                                                                               : CallbackDoNotAutoAddResource)

        {
            set_capacity(init_capacity);
        }

        /// @brief This is the default constructor.. the policy is to not auto-grow.
        /// The client code can as for AutoGrow in which case we will use the lambda
        /// to get the derived-class to build its custom pool.
        resource_pool(uint8_t         init_capacity = resource_pool_limits::DefaultCapacity,
                      auto_add_policy add_policy    = auto_add_policy::NoGrow)
        {
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

        // Not copy-able, not movable
        /// @brief Copy constructor is deleted
        resource_pool(resource_pool&) = delete;

        /// @brief Move constructor is deleted
        resource_pool(resource_pool&& src) = delete;

        /// @brief Copy assignment operator is deleted
        resource_pool& operator=(resource_pool&) = delete;

        /// @brief Move assignment operator is deleted
        resource_pool& operator=(resource_pool&& src) = delete;

        ~resource_pool()
        {
            std::scoped_lock l(m_pool_lock);
            m_is_shutdown = true;
            // The destructor is being called. No need to worry about obtaining exclusive lock
            // The fact that we're inside the destructor is pretty much exclusive.
            m_pool.clear();
        }

        auto clear() -> std::expected<void, pool_error>
        {
            if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

            std::scoped_lock l(m_pool_lock);

            // reset all stats..

            m_pool.clear();
        }

        [[nodiscard]] auto size() const -> std::expected<size_t, pool_error>
        {
            if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

            std::scoped_lock l(m_pool_lock);
            return m_pool.size();
        }

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

            m_counter_checkout++;

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
                    // This should not be counted as a loan.. we did not dole out from the pool..
                    // Moreover, it is not possible to determine if the new resource was properly
                    // allocated.

                    // We have no more items in the pool (we're starting up or everything is
                    // checked out) but we have not reached the limit. The limit is number
                    // of m_resources_checkedout + pool.size() < m_capacity We are
                    // under-capacity.. so we can return to the caller a new item..
                    m_resources_checkedout++;

                    // We should unlock the resource and ..
                    l.unlock();

                    // Update the attempted delegated calls to add new raw resource to pool.
                    ++m_counter_ondemand_adds;

                    // ..delegate the new resource acquisition
                    // outside the lock.
                    return m_callback_to_add_new_raw_resource_to_pool(*this);
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

        template <typename ...Args>
        auto add_to_pool(Args... args) -> std::expected<void, pool_error>
        {
            std::scoped_lock l(m_pool_lock);

            // Check inside the lock..
            if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

            m_pool.push_back(T{std::forward<Args>(args)...});

            ++m_counter_valid_returns;
            m_resources_checkedout--;
            m_capacity_poolsize++;

            if (m_capacity_poolsize.load() > m_capacity) m_capacity_poolsize = m_capacity;

            return {};
        }

    protected:
        auto return_to_pool(T&& item, bool isvalid = true) -> std::expected<void, pool_error>
        {
            if (isvalid) {
                std::scoped_lock l(m_pool_lock);

                // Check inside the lock..
                if (m_is_shutdown) return std::unexpected(pool_error::ShutdownInitiated);

                m_pool.push_back(std::move(item));

                ++m_counter_valid_returns;
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
                ++m_counter_invalid_returns;
                m_resources_checkedout--;
            }
            ++m_counter_checkin;

            return {};
        }

    public:
#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        auto to_json() -> std::expected<std::reference_wrapper<nlohmann::json>, siddiqsoft::arrp::pool_error>
        {
            {
                std::scoped_lock l(m_pool_lock);

                if (m_is_shutdown) return std::unexpected(siddiqsoft::arrp::pool_error::ShutdownInitiated);

                // Update the poolsize..
                m_json["size"]            = m_pool.size();
                m_json["deficit"]         = size_t(m_capacity) - m_pool.size();
                m_json["capsize"]         = m_capacity_poolsize.load();
                m_json["load"]            = m_pool.size() + m_resources_checkedout.load();
                m_json["abandoned"]       = m_abandoned.load();
                m_json["loans"]           = loan_size();
                m_json["in"]              = m_counter_checkin.load();
                m_json["out"]             = m_counter_checkout.load();
                m_json["valid_returns"]   = m_counter_valid_returns.load();
                m_json["invalid_returns"] = m_counter_invalid_returns.load();
                m_json["checked_out"]     = m_resources_checkedout.load();
                // This stage requires the type T have a json serializer
                m_json["items"] = m_pool;
            }

            return std::ref(m_json);
        }

    private:
        nlohmann::json m_json {{"_typver", "siddiqsoft.arrp.resource_pool/0.0.0"},
                               {"capacity", m_capacity},
                               {"size", 0},
                               {"load", 0},
                               {"deficit", 0},
                               {"abandoned", 0},
                               {"loans", 0},
                               {"in", 0},
                               {"out", 0}};
#endif
    };

#if defined(NLOHMANN_JSON_VERSION_MAJOR)

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
/// @note Only available if nlohmann/json is included
template <typename T, typename SRT>
    requires siddiqsoft::arrp::NonNumericMoveConstructible<T> && std::derived_from<SRT, siddiqsoft::arrp::scoped_resource<T>>
struct std::formatter<siddiqsoft::arrp::resource_pool<T, SRT>> : std::formatter<char>
{
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
