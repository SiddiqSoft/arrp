/*
    scoped_resource

    BSD 3-Clause License

    Copyright (c) 2026, Abdulkareem Siddiq
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
#include <chrono>
#ifndef SCOPED_RESOURCE_HPP
#define SCOPED_RESOURCE_HPP

#include <atomic>
#include <concepts>
#include <cstdint>
#include <format>
#include <functional>
#include <stdexcept>
#include <type_traits>

#include "common.hpp"

namespace siddiqsoft::arrp
{
    /**
     * @concept NonNumericMoveConstructible
     * @brief Concept for types that are move-constructible but not arithmetic
     *
     * @details
     * This concept ensures that a type T satisfies two requirements:
     * 1. T is move-constructible (std::move_constructible<T>)
     * 2. T is not an arithmetic type (integers, floats, etc.)
     *
     * This prevents wrapping primitive types which would be inefficient and
     * defeats the purpose of resource pooling.
     *
     * @example
     * @code
     * // Valid types
     * class MyResource { ... };
     * std::unique_ptr<int> ptr;
     * std::shared_ptr<MyResource> shared;
     *
     * // Invalid types
     * int x;                    // Arithmetic type
     * double d;                 // Arithmetic type
     * std::string str;          // Actually valid, but for example
     * @endcode
     */
    template <typename T>
    concept NonNumericMoveConstructible = std::move_constructible<T> && !std::is_arithmetic_v<T>;


    template <typename T>
        requires NonNumericMoveConstructible<T>
    class scoped_resource
    {
        // Allow resource_pool to access protected members
        template <typename U, typename SRT, uint8_t IC>
            requires((IC <= resource_pool_limits::MaxCapacity)) && NonNumericMoveConstructible<U> &&
                    std::derived_from<SRT, scoped_resource<U>>
        friend class resource_pool;

    protected:
        /// @brief The actual resource being wrapped
        /// @details Stores the resource object that will be managed by this wrapper
        T m_rsrc {};

        /// @brief Callback function to return the resource to the pool
        /// @details Called by destructor when resource is valid. Typically returns the
        ///          resource to the resource_pool for reuse.
        std::function<void(T&&, siddiqsoft::arrp::release_reason)> m_putback_callback {};

        /// @brief Tracks whether the resource is valid and should be returned to pool
        /// @details Prevents returning uninitialized or moved-out resources
        /// - true: resource will be returned to pool on destruction
        /// - false: resource will NOT be returned to pool on destruction
        bool m_is_valid {false};

    public:
        scoped_resource() = delete;

        explicit scoped_resource(T&&                                                             src,
                                 std::function<void(T&&, siddiqsoft::arrp::release_reason rr)>&& f        = {},
                                 bool                                                            is_valid = true)
            : m_rsrc(std::move(src))
            , m_putback_callback(std::move(f))
            , m_is_valid(is_valid)
        {
        }

        explicit scoped_resource(const T&) = delete;

        scoped_resource(scoped_resource&& src) noexcept
            : m_rsrc(std::move(src.m_rsrc))
            , m_putback_callback(std::move(src.m_putback_callback))
            , m_is_valid(src.m_is_valid)
        {
            // Reset to ensure that the source does not double return!
            src.m_putback_callback = {};
            src.m_is_valid         = false;
        }

        scoped_resource& operator=(scoped_resource&& src) noexcept
        {
            if (this != &src) {
                // Make sure that the src does not return anymore by resetting its callback
                m_putback_callback     = std::move(src.m_putback_callback);
                src.m_putback_callback = {};
                // Move from src
                m_rsrc     = std::move(src.m_rsrc);
                m_is_valid = src.m_is_valid;
                // Make sure we do not have the src checkin
                src.m_is_valid = false;
            }
            return *this;
        }

        scoped_resource& operator=(T&& src)
        {
            m_rsrc     = std::move(src);
            m_is_valid = true;
            return *this;
        }

        /**
         * @brief Copy assignment operator is deleted
         *
         * @details
         * Copy assignment is not allowed to maintain move-only semantics
         * and prevent resource ownership ambiguity.
         */
        scoped_resource& operator=(const scoped_resource&) = delete;

        auto             operator*() -> T& { return m_rsrc; }

        explicit         operator T&() { return m_rsrc; }

        ~scoped_resource()
        {
            // Only return resource if it's valid and callback exists
            // This prevents returning uninitialized or moved-out resources to the pool
            if (m_putback_callback) {
                m_putback_callback(std::move(m_rsrc),
                                   m_is_valid ? siddiqsoft::arrp::release_reason::Valid
                                              : siddiqsoft::arrp::release_reason::Abandoned);
                m_is_valid         = false;
                m_putback_callback = {};
            }
        }

        void invalidate() { m_is_valid = false; }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        nlohmann::json to_json() const
        {
            return {{"_typver", "siddiqsoft.arrp.scoped_resource/0.0.0"}, {"capacity", m_is_valid}, {"value", m_rsrc}};
        }
#endif
    };
} // namespace siddiqsoft::arrp

#endif
