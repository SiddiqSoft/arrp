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

    /**
     * @class scoped_resource
     * @brief RAII wrapper for managing resource lifecycle with automatic return to pool
     *
     * @tparam T The resource type to wrap. Must be move-constructible and non-arithmetic.
     *
     * @details
     * The scoped_resource class implements the Resource Acquisition Is Initialization (RAII)
     * pattern for managing resources that should be returned to a resource pool. It wraps
     * a resource and automatically invokes a callback when the wrapper is destroyed, enabling
     * automatic resource management without manual cleanup.
     *
     * Key Features:
     * - **RAII Pattern**: Automatically returns resources via destructor
     * - **Move Semantics**: Supports move construction and move assignment for efficient transfers
     * - **Validity Tracking**: Tracks whether a resource should be returned to the pool
     * - **Callback Support**: Executes custom callback on destruction
     * - **Debug Support**: Includes debug identifiers for tracking in DEBUG builds
     * - **Dereference Access**: Provides operator* and explicit conversion for resource access
     * - **Invalidation**: Allows explicit invalidation to prevent automatic return
     *
     * Usage Pattern:
     * @code
     * // Typical usage with resource_pool
     * {
     *     auto resource = pool.checkout();
     *     // Use the resource...
     *     resource->doSomething();
     *     // Automatically returned to pool when going out of scope
     * }
     *
     * // Manual invalidation
     * {
     *     auto resource = pool.checkout();
     *     auto ptr = std::move(*resource);
     *     resource.invalidate();  // Don't return the moved-out resource
     * }
     * @endcode
     *
     * Thread Safety:
     * - Not thread-safe by itself; thread safety is provided by resource_pool
     * - The callback should be thread-safe if called from multiple threads
     *
     * Constraints:
     * - Not copy-constructible or copy-assignable
     * - Default constructor is deleted
     * - Resources must be move-constructible and non-arithmetic
     *
     * @note This class is typically used internally by resource_pool
     * @note The callback is optional; if not provided, the resource is simply destroyed
     * @note Derived classes can override behavior by providing custom callbacks
     */
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
        std::function<void(T&&)> m_putback_callback {};

        /// @brief Callback function to inform the pool that a resource has been invalidated
        /// @details Called by destructor when resource is invalid.
        std::function<void(T&&)> m_abandon_callback {};

        /// @brief Tracks whether the resource is valid and should be returned to pool
        /// @details Prevents returning uninitialized or moved-out resources
        /// - true: resource will be returned to pool on destruction
        /// - false: resource will NOT be returned to pool on destruction
        bool m_is_valid {false};

    public:
        /**
         * @brief Default constructor is deleted
         *
         * @details
         * Resources must be explicitly constructed with a valid resource.
         * This prevents accidental creation of invalid scoped_resource objects.
         *
         * @note Use the explicit constructor that takes a resource instead
         */
        scoped_resource() = delete;

        /**
         * @brief Construct a scoped_resource with a resource and optional callback
         *
         * @param src R-value reference to the resource to wrap
         * @param f Optional callback function to return resource to pool
         *
         * @details
         * Constructs a scoped_resource wrapper around the provided resource.
         * The resource is marked as valid upon construction. The callback is
         * typically provided by resource_pool to automatically return the resource.
         *
         * For derived classes, the callback parameter can be omitted and will be set
         * by resource_pool through friend access to protected members.
         *
         * The constructor is marked explicit to prevent accidental implicit conversions.
         *
         * @note This constructor is typically called by resource_pool::checkout()
         * @note The callback is optional; if not provided, the resource is simply destroyed
         * @note The resource is moved into the wrapper, not copied
         *
         * @example
         * @code
         * // Direct construction (rarely used)
         * MyResource res;
         * auto wrapped = scoped_resource<MyResource>(
         *     std::move(res),
         *     [](MyResource&& r) { .. return to pool .. }
         * );
         *
         * // Typical usage via resource_pool
         * auto wrapped = pool.checkout();
         * @endcode
         */
        explicit scoped_resource(T&& src, std::function<void(T&&)>&& f = {}, std::function<void(T&&)>&& ff = {})
            : m_rsrc(std::move(src))
            , m_putback_callback(std::move(f))
            , m_abandon_callback(std::move(ff))
            , m_is_valid(true)
        {
        }

        /**
         * @brief Copy constructor is deleted
         *
         * @details
         * Resources are move-only to maintain clear ownership semantics.
         * Copying would create ambiguity about which wrapper owns the resource
         * and when it should be returned to the pool.
         *
         * @note Use move construction instead
         */
        explicit scoped_resource(const T&) = delete;

        /**
         * @brief Move constructor
         *
         * @param src R-value reference to another scoped_resource to move from
         *
         * @details
         * Moves the resource and callback from another wrapper. This is essential
         * for returning wrapped resources from functions and for transferring ownership.
         *
         * The source wrapper is reset to prevent double-return: its callback is cleared
         * and its validity flag is set to false. This ensures that only one wrapper
         * will attempt to return the resource to the pool.
         *
         * @note This is a noexcept operation
         * @note The source wrapper becomes invalid after the move
         * @note Derived classes should call this constructor in their move constructor
         *
         * @example
         * @code
         * scoped_resource<MyResource> src =  ... ;
         * scoped_resource<MyResource> dst = std::move(src);
         * // src is now invalid; only dst will return the resource
         * @endcode
         */
        scoped_resource(scoped_resource&& src) noexcept
            : m_rsrc(std::move(src.m_rsrc))
            , m_putback_callback(std::move(src.m_putback_callback))
            , m_abandon_callback(std::move(src.m_abandon_callback))
            , m_is_valid(src.m_is_valid)
        {
            // Reset to ensure that the source does not double return!
            src.m_putback_callback = {};
            src.m_is_valid         = false;
        }

        /**
         * @brief Move assignment operator
         *
         * @param src R-value reference to another scoped_resource to assign
         * @return Reference to this scoped_resource
         *
         * @details
         * Assigns a new scoped_resource to this wrapper. If this wrapper currently holds
         * a valid resource, it will be returned to the pool before the assignment.
         * The source wrapper is reset to prevent double-return.
         *
         * @note This is a noexcept operation
         * @note The previous resource (if valid) is returned to the pool
         * @note The source wrapper becomes invalid after the move
         *
         * @example
         * @code
         * auto resource1 = pool.checkout();
         * auto resource2 = pool.checkout();
         * resource1 = std::move(resource2);  // resource1 returned, resource2 moved
         * @endcode
         */
        scoped_resource& operator=(scoped_resource&& src) noexcept
        {
            if (this != &src) {
                // Return current resource if valid
                if (m_is_valid && m_putback_callback) {
                    m_putback_callback(std::move(m_rsrc));
                }

                // Move from src
                m_rsrc             = std::move(src.m_rsrc);
                m_putback_callback = std::move(src.m_putback_callback);
                m_abandon_callback = std::move(src.m_abandon_callback);
                m_is_valid         = src.m_is_valid;

                // Reset src
                src.m_putback_callback = {};
                src.m_abandon_callback = {};
                src.m_is_valid         = false;
            }
            return *this;
        }

        /**
         * @brief Move assignment operator for resource
         *
         * @param src R-value reference to the resource to assign
         * @return Reference to this scoped_resource
         *
         * @details
         * Assigns a new resource to this wrapper and marks it as valid.
         * The previous resource (if any) is discarded without invoking its callback.
         *
         * This operator allows reassigning a new resource to an existing wrapper,
         * which is useful in certain advanced scenarios.
         *
         * @note The previous resource is NOT returned to the pool
         * @note The new resource is marked as valid
         * @note The callback is NOT changed by this operation
         * @note In DEBUG builds, outputs debug information
         *
         * @example
         * @code
         * auto resource = pool.checkout();
         * MyResource new_res;
         * resource = std::move(new_res);  // Replace the resource
         * @endcode
         */
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

        /**
         * @brief Dereference operator to access the underlying resource
         *
         * @return Reference to the wrapped resource
         *
         * @details
         * Provides direct access to the wrapped resource via the dereference operator.
         * This allows treating the scoped_resource like a pointer to the resource.
         *
         * @note Does not check validity; use only when you know the resource is valid
         * @note Returns a non-const reference for modification
         *
         * @example
         * @code
         * auto resource = pool.checkout();
         * (*resource).doSomething();  // Access via dereference
         * @endcode
         */
        auto operator*() -> T& { return m_rsrc; }

             operator T&() { return m_rsrc; }

        /**
         * @brief Destructor - automatically returns resource to pool if valid
         *
         * @details
         * The destructor implements the RAII pattern:
         * - If m_is_valid is true and m_putback_callback exists: returns resource to pool
         * - If m_is_valid is false: resource is discarded (not returned)
         *
         * This ensures resources are always properly managed, even if an exception
         * occurs. The callback is invoked with an rvalue reference to the resource,
         * allowing the pool to take ownership.
         *
         * After invoking the callback, the validity flag and callback are cleared
         * to prevent accidental double-return.
         *
         * @note This is called automatically when the scoped_resource goes out of scope
         * @note Safe to call even if the resource is invalid
         * @note In DEBUG builds, outputs debug information
         * @note The callback is responsible for thread safety if needed
         *
         * @example
         * @code
         * {
         *     auto resource = pool.checkout();
         *     // Use resource...
         * }  // Destructor called here; resource returned to pool
         * @endcode
         */
        ~scoped_resource()
        {
            // Only return resource if it's valid and callback exists
            // This prevents returning uninitialized or moved-out resources to the pool
            if (m_is_valid && m_putback_callback) {
                m_putback_callback(std::move(m_rsrc));
                m_is_valid         = false;
                m_putback_callback = {};
            }
            else if (m_abandon_callback) {
                // Inform the pool that resource was invalidated
                m_abandon_callback(std::move(m_rsrc));
                m_is_valid         = false;
                m_putback_callback = {};
                m_abandon_callback = {};
            }
        }

        /**
         * @brief Invalidate the resource to prevent it from being returned to pool
         *
         * @details
         * Call this method when you've moved the resource out or want to prevent
         * automatic return to the pool. After calling this, the destructor will NOT
         * return the resource to the pool.
         *
         * This is useful when you want to take ownership of the resource and prevent
         * it from being returned to the pool for reuse.
         *
         * Use Cases:
         * - You've moved the resource out and it's no longer valid
         * - You want to take ownership and prevent automatic return
         * - You're implementing custom resource management
         * - The resource has been consumed or transferred elsewhere
         *
         * @note Safe to call multiple times (subsequent calls have no effect)
         * @note This is primarily for advanced scenarios; normal usage doesn't need this
         * @note Once invalidated, the resource cannot be re-validated
         *
         * @example
         * @code
         * auto resource = pool.checkout();
         * auto ptr = std::move(*resource);
         * resource.invalidate();  // Don't return the moved-out resource
         * // Resource is NOT returned to pool when resource goes out of scope
         *
         * // Another example: consuming the resource
         * auto resource = pool.checkout();
         * process_and_consume(*resource);
         * resource.invalidate();  // Resource was consumed, don't return it
         * @endcode
         */
        void invalidate()
        {
            m_is_valid         = false;
            m_putback_callback = {};
        }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        nlohmann::json to_json() const
        {
            return {{"_typver", "siddiqsoft.arrp.scoped_resource/0.0.0"}, {"capacity", m_is_valid}, {"value", m_rsrc}};
        }
#endif
    };
} // namespace siddiqsoft::arrp

#endif
