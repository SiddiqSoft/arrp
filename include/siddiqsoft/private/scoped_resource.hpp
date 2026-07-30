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

#include <concepts>
#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <print>
#include <string>
#include <type_traits>

#include "common.hpp"

namespace siddiqsoft::arrp
{

    template <typename T>
    concept HasStdToStringImpl = requires(T t) {
        { std::to_string(t) } -> std::same_as<std::string>;
    };

    /// @brief Concept for types that are move-constructible but not arithmetic
    ///
    /// This concept ensures that a type T satisfies two requirements:
    /// 1. T is move-constructible (std::move_constructible<T>)
    /// 2. T is move-assignable (std::move_assignable<T>)
    /// 3. T is not an arithmetic type (integers, floats, etc.)
    ///
    /// This prevents wrapping primitive types which would be inefficient and
    /// defeats the purpose of resource pooling.
    ///
    /// @example
    /// @code
    /// // Valid types
    /// class MyResource { ... };
    /// std::unique_ptr<int> ptr;
    /// std::shared_ptr<MyResource> shared;
    ///
    /// // Invalid types
    /// int x;                    // Arithmetic type
    /// double d;                 // Arithmetic type
    /// @endcode
    template <typename T>
    concept NonNumericMoveConstructible =
            std::is_move_constructible_v<T> && std::is_move_assignable_v<T> && !std::is_arithmetic_v<T>;


    /// @brief RAII wrapper for managing resource lifecycle in a resource pool
    ///
    /// @details
    /// scoped_resource automatically returns resources to the pool when destroyed.
    /// It enforces move-only semantics to prevent resource ownership ambiguity.
    /// The resource is wrapped with a callback that is invoked during destruction
    /// to return the resource to the pool.
    ///
    /// @tparam T The resource type (must be move-constructible and non-arithmetic)
    ///
    /// @warning This class is NOT thread-safe. Each scoped_resource instance
    /// should be accessed by only one thread at a time. The resource_pool itself
    /// is thread-safe, but individual scoped_resource instances are not.
    ///
    /// @note Move-only semantics: Copy operations are deleted to prevent resource ownership ambiguity
    /// @note RAII pattern: Resource is automatically returned to pool on destruction
    /// @note Callback-based: Uses std::function callback to return resource to pool
    ///
    /// @example
    /// @code
    /// // Typically obtained from resource_pool::borrow_from_pool()
    /// auto resource = pool.borrow_from_pool();
    /// if (resource) {
    ///     // Use resource
    ///     resource->doSomething();
    /// }
    /// // Resource automatically returned to pool when scoped_resource is destroyed
    /// @endcode
    template <typename T>
        requires NonNumericMoveConstructible<T>
    class scoped_resource
    {
    public:
        // ========== Type Definitions ==========

        /// @brief Callback function type for returning resource to pool
        ///
        /// This callback allows the implementor that is asking for the scoped_resource the ability to
        /// recall it back or perform any additional tasks.
        /// The callback must not throw and must not invoke any other method in the pool that requires
        /// lock manipulation.
        ///
        /// @param resource The resource being returned (moved)
        /// @param is_valid Whether the resource is valid and should be reused
        using PutbackCallbackFunc = std::function<void(T&&, bool)>;

    private:
        // ========== Friend Declarations ==========

        // Allow resource_pool to access protected members
        template <typename U, typename SRT>
            requires NonNumericMoveConstructible<U> && std::derived_from<SRT, scoped_resource<U>>
        friend class resource_pool;

        // ========== Member Variables ==========

        /// @brief The actual resource being wrapped
        /// @details Stores the resource object that will be managed by this wrapper
        T m_rsrc {};

        /// @brief Callback function to return the resource to the pool
        /// @details Called by destructor when resource is valid. Typically returns the
        ///          resource to the resource_pool for reuse.
        PutbackCallbackFunc m_putback_callback {};

        /// @brief Tracks whether the resource is valid and should be returned to pool
        /// @details Prevents returning uninitialized or moved-out resources
        /// - true: resource will be returned to pool on destruction
        /// - false: resource will NOT be returned to pool on destruction
        bool m_is_valid {false};

    public:
        // ========== Deleted Constructors ==========

        /// @brief Default constructor is deleted
        /// @details scoped_resource must be constructed with a callback and resource
        scoped_resource() = delete;

        /// @brief Copy constructor is deleted
        /// @details scoped_resource is move-only to prevent resource ownership ambiguity
        /// and ensure proper RAII semantics. Only one scoped_resource can own a resource.
        explicit scoped_resource(const T&) = delete;

        /// @brief Copy assignment operator is deleted
        ///
        /// @details
        /// Copy assignment is not allowed to maintain move-only semantics
        /// and prevent resource ownership ambiguity.
        scoped_resource& operator=(const scoped_resource&) = delete;

        // ========== Constructors (Protected Access via Friend) ==========

        /// @brief Constructs a scoped_resource with a callback and resource
        ///
        /// @param f The callback function to invoke on destruction
        /// @param src The resource to manage (moved)
        ///
        /// @note The callback is stored and invoked during destruction
        /// @note The resource is moved into the wrapper
        /// @note The resource is marked as valid
        /// @note Protected: Only accessible via resource_pool friend class
        explicit scoped_resource(PutbackCallbackFunc&& f, T&& src)
            : m_rsrc(std::move(src))
            , m_putback_callback(std::move(f))
            , m_is_valid(true)
        {
        }

        /// @brief Constructs a scoped_resource with a callback and in-place constructed resource
        ///
        /// @tparam Args Types of arguments to forward to T's constructor
        /// @param f The callback function to invoke on destruction
        /// @param args Arguments to forward to T's constructor
        ///
        /// @note The resource is constructed in-place
        /// @note The resource is marked as valid
        /// @note Protected: Only accessible via resource_pool friend class
        template <typename... Args>
        scoped_resource(PutbackCallbackFunc&& f, Args&&... args)
            : m_rsrc(std::forward<Args>(args)...)
            , m_putback_callback(std::move(f))
            , m_is_valid(true)
        {
        }

        // ========== Move Operations ==========

        /// @brief Move constructor
        ///
        /// Transfers ownership from another scoped_resource to this one.
        /// The source is invalidated to prevent double-return.
        ///
        /// @param src The source scoped_resource to move from
        ///
        /// @note The source's callback is cleared to prevent double-return
        /// @note The source is marked as invalid
        scoped_resource(scoped_resource&& src) noexcept
            : m_rsrc(std::move(src.m_rsrc))
            , m_putback_callback(std::move(src.m_putback_callback))
            , m_is_valid(src.m_is_valid)
        {
            // Reset to ensure that the source does not double return!
            src.m_putback_callback = {};
            src.m_is_valid         = false;
        }

        /// @brief Move assignment operator
        ///
        /// Transfers ownership from another scoped_resource to this one.
        /// Before taking ownership, the currently-held resource (if valid) is returned
        /// to the pool via the putback callback. The source is then invalidated to
        /// prevent double-return.
        ///
        /// @param src The source scoped_resource to move from
        /// @return Reference to this scoped_resource
        ///
        /// @note Self-assignment is checked via pointer comparison
        /// @note The currently-held resource is returned to the pool before overwrite
        /// @note The source's callback is cleared to prevent double-return
        /// @note The source is marked as invalid after the move
        /// @note NOT noexcept: T's move-assignment may throw; declaring noexcept here
        ///       would call std::terminate if T::operator=(T&&) throws after the
        ///       putback callback has already fired (state would be inconsistent).
        scoped_resource& operator=(scoped_resource&& src)
        {
            if (this != &src) {
                // Return current resource to pool before overwriting it.
                // The callback is invoked with the current validity flag so the pool
                // can decide whether to reuse (valid) or discard (invalid) the resource.
                if (m_putback_callback) {
                    try {
                        m_putback_callback(std::move(m_rsrc), m_is_valid);
                    }
                    catch (...) {
                        std::print(std::cerr,
                                   "scoped_resource move-assignment: exception while returning current resource to pool!\n");
                    }
                    // Clear our own callback and validity now that the resource has been
                    // handed back; this prevents the destructor from double-returning.
                    m_putback_callback = {};
                    m_is_valid         = false;
                }

                // Take ownership of src's resource and callback.
                m_rsrc             = std::move(src.m_rsrc);
                m_putback_callback = std::move(src.m_putback_callback);
                m_is_valid         = src.m_is_valid;

                // Disarm src so its destructor does nothing.
                src.m_putback_callback = {};
                src.m_is_valid         = false;
            }
            return *this;
        }

        // ========== Destructor ==========

        /// @brief Destructor - invokes callback to handle resource return or abandonment
        ///
        /// Invokes the putback callback if one exists, passing the resource and its validity status.
        /// The callback is responsible for deciding whether to return the resource to the pool (if valid)
        /// or discard it (if invalid). Exceptions from the callback are caught and logged to stderr.
        ///
        /// @note Noexcept: Exceptions are caught and logged, not propagated
        /// @note The callback is always invoked if set, regardless of validity
        /// @note The callback receives the validity flag to make the appropriate decision
        /// @note The callback is cleared after invocation
        /// @note The resource is marked as invalid after callback invocation
        ~scoped_resource() noexcept
        {
            // Invoke callback if it exists, passing the resource and validity status
            // The callback (typically resource_pool::return_to_pool) decides whether to
            // reuse the resource (if valid) or discard it (if invalid)
            if (m_putback_callback) {
                try {
                    m_putback_callback(std::move(m_rsrc), m_is_valid);
                }
                catch (...) {
                    std::print(std::cerr, "scoped_resource destructor: exception while invoking putback callback!\n");
                }
                m_is_valid         = false;
                m_putback_callback = {};
            }
        }

        // ========== Resource Access Operators ==========

        /// @brief Dereference operator to access the wrapped resource
        /// @return Reference to the wrapped resource
        /// @warning Behavior is undefined if resource has been invalidated
        auto operator*() -> T& { return m_rsrc; }

        /// @brief Pointer-like access to the wrapped resource
        /// @return Pointer to the wrapped resource, or nullptr if invalid
        /// @note Returns nullptr if resource is invalid
        auto operator->() -> T* { return m_is_valid ? &m_rsrc : nullptr; }

        /// @brief Explicit conversion to resource reference
        /// @return Reference to the wrapped resource
        /// @warning Behavior is undefined if resource has been invalidated
        explicit operator T&() { return m_rsrc; }

        /// @brief Assignment operator for resource value
        ///
        /// Assigns a new resource value to this wrapper.
        /// Marks the resource as valid.
        ///
        /// @param src The new resource value (moved)
        /// @return Reference to this scoped_resource
        ///
        /// @note The resource is moved into the wrapper
        /// @note The resource is marked as valid
        scoped_resource& operator=(T&& src)
        {
            m_rsrc     = std::move(src);
            m_is_valid = true;
            return *this;
        }

        // ========== State Management ==========

        /// @brief Marks the resource as invalid (abandoned)
        ///
        /// Sets the validity flag to false. When the resource is destroyed, the callback
        /// will be invoked with isvalid=false, allowing the pool to discard the resource
        /// rather than returning it for reuse. This is appropriate when the resource has
        /// been moved out, corrupted, or otherwise rendered unusable.
        ///
        /// @note Virtual: Can be overridden in derived classes
        /// @note The callback is still invoked; only the validity flag changes
        /// @note The pool's callback should check the validity flag and handle accordingly
        /// @note Typically called when the resource is corrupted, moved out, or consumed
        ///
        /// @example
        /// @code
        /// auto resource = pool.borrow_from_pool();
        /// if (resource) {
        ///     auto extracted = std::move(*resource);
        ///     resource.invalidate();  // Mark as invalid so pool discards it
        ///     // Use extracted resource elsewhere
        /// }
        /// @endcode
        virtual void invalidate() { m_is_valid = false; }

        /// @brief Checks if the resource is valid
        ///
        /// @return true if the resource is valid and will be returned to pool, false otherwise
        ///
        /// @note Virtual: Can be overridden in derived classes
        /// @note Const: Does not modify the resource
        virtual bool is_valid() const { return m_is_valid; }

        // ========== Serialization ==========

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        /// @brief Serializes the scoped_resource to JSON
        ///
        /// Returns a JSON object containing the resource state and validity.
        /// Only available if nlohmann/json.hpp is included before this header.
        ///
        /// @return JSON object with:
        ///   - _typver: Type and version string ("siddiqsoft.arrp.scoped_resource/1.0.0")
        ///   - valid: Whether the resource is valid (boolean)
        ///   - value: The resource value (if serializable, otherwise "-noserializer-")
        ///
        /// @note Requires NLOHMANN_JSON_VERSION_MAJOR to be defined
        /// @note If T is not serializable, value is set to "-noserializer-"
        ///
        /// @warning BREAKING CHANGE (v1.0.0): The JSON schema key changed from "capacity" to "valid".
        ///          Previous versions incorrectly used "capacity" to represent the validity flag.
        ///          Code parsing this JSON must be updated to use the "valid" key.
        ///
        /// @par JSON Schema:
        /// @code{.json}
        /// {
        ///   "_typver": "siddiqsoft.arrp.scoped_resource/1.0.0",
        ///   "valid": true,
        ///   "value": <resource_value>
        /// }
        /// @endcode
        nlohmann::json to_json() const
        {
            if constexpr (std::is_same_v<T, std::string> || std::is_arithmetic_v<T>)
                return {{"_typver", "siddiqsoft.arrp.scoped_resource/1.0.0"}, {"valid", m_is_valid}, {"value", m_rsrc}};
            else if constexpr (HasStdToStringImpl<T>)
                return {{"_typver", "siddiqsoft.arrp.scoped_resource/1.0.0"},
                        {"valid", m_is_valid},
                        {"value", std::to_string(m_rsrc)}};
            else
                return {{"_typver", "siddiqsoft.arrp.scoped_resource/1.0.0"}, {"valid", m_is_valid}, {"value", "-noserializer-"}};
        }
#endif
    };
} // namespace siddiqsoft::arrp


/// @brief Specialization of std::formatter for scoped_resource
/// @details Provides formatted output for scoped_resource instances using std::format.
/// Uses a consistent format across all translation units to avoid ODR violations.
/// @tparam T The resource type
/// @note This formatter always uses the same format regardless of whether nlohmann/json is available,
///       ensuring ODR safety. For JSON output, use the to_json() method directly.
template <typename T>
struct std::formatter<siddiqsoft::arrp::scoped_resource<T>> : std::formatter<std::string>
{
    /// @brief Format the scoped_resource
    /// @param sr The scoped_resource to format
    /// @param ctx Format context
    /// @return Iterator to end of formatted output
    /// @note Always uses the same format to ensure ODR safety across translation units
    auto format(const siddiqsoft::arrp::scoped_resource<T>& sr, auto& ctx) const
    {
        // Use consistent format across all TUs to avoid ODR violations
        // Format: scoped_resource<T>{valid: <bool>}
        return std::format_to(ctx.out(), "scoped_resource<T>{{valid: {}}}", sr.is_valid());
    }
};


#endif
