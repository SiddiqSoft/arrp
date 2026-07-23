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

#include "private/common.hpp"
#include "private/scoped_resource.hpp"
#include "siddiqsoft/RunOnEnd.hpp"

namespace siddiqsoft::arrp
{
    template <typename T, typename SRT = scoped_resource<T>, uint8_t InitCapacity = resource_pool_limits::DefaultCapacity>
        requires((InitCapacity <= resource_pool_limits::MaxCapacity)) && NonNumericMoveConstructible<T> &&
                std::derived_from<SRT, scoped_resource<T>>
    class resource_pool
    {
    private:
        /// @brief Maximum number of resources that can be in the pool
        uint8_t m_capacity {InitCapacity};

        /// @brief Number of resources currently checked out from the pool
        /// @note This is a signed number allowing us to detect "bad" loans
        ///       otherwise known as abandoned/invalid resources which were
        ///       not added back to the pool.
        std::atomic_int16_t m_loans {0};

        /// @brief Number of resources currently checked out from the pool
        std::atomic_int16_t m_resources_checkedout {0};

        /// @brief Number of resources that have been invalidated
        /// @note Currently unused; reserved for future use
        std::atomic_uint16_t m_invalidated_resources {0};

        /// @brief Counter for borrow operations from the pool
        /// @note Only counts successful borrow operations
        std::atomic_uint64_t m_counter_checkout {0};

        /// @brief Counter for ondemand resource additions via factory callback
        std::atomic_uint64_t m_counter_ondemand_adds {0};

        /// @brief Counter for return operations to the pool
        std::atomic_uint64_t m_counter_checkin {0};

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
        std::function<SRT(resource_pool&)> m_callback_to_add_new_raw_resource_to_pool {};

    public:
        /// @brief This callback is the default and does not grow the resource; it throws a runtime_error
        static inline std::function<SRT(resource_pool&)> CallbackDoNotAutoAddResource = [](resource_pool&) -> SRT {
            throw std::runtime_error("No items in the pool; add something first.");
        };

        resource_pool(std::function<SRT(resource_pool&)>&& new_resource_callback)
        {
            if (new_resource_callback) {
                m_callback_to_add_new_raw_resource_to_pool = std::move(new_resource_callback);
            }
            else {
                // This is just in case someone sends an empty callback!
                m_callback_to_add_new_raw_resource_to_pool = CallbackDoNotAutoAddResource;
            }
        }

        /// @brief This is the default constructor.. the policy is to not auto-grow.
        /// The client code can as for AutoGrow in which case we will use the lambda
        /// to get the derived-class to build its custom pool.
        resource_pool(auto_add_policy add_policy = auto_add_policy::NoGrow)
        {
            if (add_policy == auto_add_policy::NoGrow) {
                m_callback_to_add_new_raw_resource_to_pool = CallbackDoNotAutoAddResource;
            }
            else if (add_policy == auto_add_policy::AutoGrow) {
                // This method is declared here as lambda to capture the this pointer
                // whereas if we attempted to declared it earlier as a static inline then the
                // this pointer would not be captured.
                m_callback_to_add_new_raw_resource_to_pool = [this](resource_pool& pool) -> SRT {
                    // Create a SRT element and wire up the auto-return callback to return
                    // the resource back to this object.
                    return SRT {T {},
                                [this](T&& src, siddiqsoft::arrp::release_reason rr) { // this callback puts the resource back..
                                    this->m_counter_auto_returned++;
                                    this->checkin(std::forward<T&&>(src), rr);
                                }};
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
            // The destructor is being called. No need to worry about obtaining exclusive lock
            // The fact that we're inside the destructor is pretty much exclusive.
            m_pool.clear();
        }

        void clear() noexcept
        {
            std::scoped_lock l(m_pool_lock);

            // reset all stats..
            m_resources_checkedout.exchange(0, std::memory_order_release);
            m_invalidated_resources.exchange(0, std::memory_order_release);
            m_counter_checkout.exchange(0, std::memory_order_release);
            m_counter_ondemand_adds.exchange(0, std::memory_order_release);
            m_counter_checkin.exchange(0, std::memory_order_release);
            m_counter_auto_returned.exchange(0, std::memory_order_release);

            m_pool.clear();
        }

        [[nodiscard]] size_t size() const noexcept
        {
            std::scoped_lock l(m_pool_lock);
            return m_pool.size();
        }

        [[nodiscard]] auto checkout() -> SRT
        {
            // Create a guard to decrement m_resources_checkedout if the factory callback throws
            // This ensures we don't leak the checkout count if the factory fails
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

                if (!m_pool.empty()) {
                    // The pool is non-empty; return from the pool
                    // Return first element from the pool and pop it on scope end
                    RunOnEnd pop_guard([&]() {
                        m_pool.pop_front();
                        m_loans++;
                    });

                    m_resources_checkedout++;

                    // Make a wrapper..
                    // Create a SRT element and wire up the auto-return callback to return
                    // the resource back to this object.
                    return SRT {std::move(m_pool.front()),
                                [this](T&& src, siddiqsoft::arrp::release_reason rr) { // this callback puts the resource back..
                                    this->m_counter_auto_returned++;
                                    this->checkin(std::forward<T&&>(src), rr);
                                }};
                    // Allow the compiler to use NRVO (move elision; do not use std::move here!)
                    // The pop_front() happens within this scope and within the lock!
                }
                else if ((m_capacity > m_pool.size() + m_resources_checkedout) && m_callback_to_add_new_raw_resource_to_pool) {
                    // This should not be counted as a loan.. we did not dole out from the pool..
                    // Moreover, it is not possible to determine if the new resource was properly
                    // allocated.
                    // The best time to account for m_loans would be from the pool itself.

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
                else if (m_capacity > m_pool.size() + m_resources_checkedout) {
                    // We're under-capacity.. but no dynamic resource provider
                    throw std::runtime_error(std::format("We're under-capacity.. but no dynamic resource provider\n"));
                }
            } // scope end
            catch (std::exception& ex) {
                checkout_guard();
#if defined(DEBUG_TRACE)
                std::cerr << std::format("Error in checkout: {}\n", ex.what());
#endif
                throw;
            }
            catch (...) {
                checkout_guard();
                std::cerr << std::format("UNKNOWN Error in checkout\n");
                throw;
            }

#if defined(DEBUG)
            auto msg = std::format("Starving: {}", this->to_json().dump());
            throw std::runtime_error(msg);
#else
            throw std::runtime_error("Starving; add more resources");
#endif
        }

        auto is_there_a_pool_deficit() { return m_pool.size() < m_capacity; }

        void checkin(T&& item, release_reason reason = release_reason::Unknown)
        {
            ++m_counter_checkin;

            if (!siddiqsoft::arrp::is_release_reason_abandoned(reason)) {
                std::scoped_lock l(m_pool_lock);

                m_pool.push_back(std::move(item));
                m_loans--; // we're returning a borrowed item..

                m_resources_checkedout--;
            } // lock scope end
            else if (siddiqsoft::arrp::is_release_reason_abandoned(reason)) {
                // Resource was invalidated; do not add back to the pool.
                // We need to decrement the checkout count under lock to ensure thread safety
                {
                    std::scoped_lock l(m_pool_lock);
                    m_invalidated_resources++;
                    m_resources_checkedout--;
                }

#if defined(DEBUG)
                std::cerr << std::format("Resource was invalidated! {}\n", this->to_json().dump());
#endif
            }
        }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        auto to_json() -> nlohmann::json&
        {
            {
                std::scoped_lock l(m_pool_lock);

                // Update the poolsize..
                m_json["size"]      = m_pool.size();
                m_json["deficit"]   = m_capacity - m_pool.size();
                m_json["load"]      = m_pool.size() + m_resources_checkedout.load();
                m_json["abandoned"] = m_invalidated_resources.load();
                m_json["loans"]     = m_loans.load();
                m_json["in"]   = m_counter_checkin.load();
                m_json["out"]  = m_counter_checkout.load();
            }

            return m_json;
        }

    private:
        nlohmann::json m_json {{"_typver", "resource_pool/0.0.0"},
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
