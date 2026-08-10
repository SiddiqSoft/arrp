/*
    resource_guard

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

#ifndef RESOURCE_GUARD_HPP
#define RESOURCE_GUARD_HPP

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
    /// resource_guard invokes its return callback when destroyed.
    /// It enforces move-only semantics to prevent resource ownership ambiguity.
    /// The resource is wrapped with a callback that is invoked during destruction
    /// to return the resource to the pool.
    ///
    /// @tparam T The resource type (must be move-constructible and non-arithmetic)
    ///
    /// @warning This class is NOT thread-safe. Each resource_guard instance
    /// should be accessed by only one thread at a time. The resource_pool itself
    /// has synchronized storage operations, but individual resource_guard instances
    /// are not.
    ///
    /// @note Move-only semantics: Copy operations are deleted to prevent resource ownership ambiguity
    /// @note RAII pattern: Resource is automatically returned to pool on destruction
    /// @note Callback-based: Uses std::function callback to return resource to pool
    /// @note Pool-created guards carry the callback that returns their resource.
    /// @note Validity tracking: Tracks whether resource is valid and should be returned to pool
    ///
    /// @example
    /// @code
    /// // Typically obtained from resource_pool::try_borrow()
    /// auto resource = pool.try_borrow();
    /// if (resource) {
    ///     // Use resource
    ///     resource->doSomething();
    /// }
    /// // Resource automatically returned to pool when resource_guard is destroyed
    /// @endcode
    template <typename T>
        requires NonNumericMoveConstructible<T>
    class resource_guard final
    {
    public:
        /// @brief Callback function type for returning resource to pool
        ///
        /// The callback receives the resource and whether it remains reusable.
        /// It is normally installed by resource_pool.
        ///
        /// @param resource The resource being returned (moved)
        /// @param is_valid Whether the resource is valid and should be reused
        using PutbackCallbackFunc = std::function<void(T&&, bool)>;

    protected:
        /// @brief The actual resource being wrapped
        /// @details Stores the resource object that will be managed by this wrapper
        T m_rsrc;

    private:
        // Allow resource_pool to access protected members
        template <typename U>
            requires NonNumericMoveConstructible<U>
        friend class resource_pool;

        /// @brief Callback function to return the resource to the pool
        /// @details Called by the destructor for both valid and invalid resources.
        PutbackCallbackFunc m_putback_callback {};

        /// @brief Tracks whether the resource is valid and should be returned to pool
        /// @details Prevents returning uninitialized or moved-out resources
        /// - true: resource will be returned to pool on destruction
        /// - false: resource will NOT be returned to pool on destruction
        bool       m_is_valid {false};

        pool_error m_error_code {pool_error::Ok};

    protected:
        /// @brief Default constructor
        /// @note resource_guard is `final`, so no derived class can call this
        ///       constructor. It is retained as `protected` rather than removed;
        ///       it creates a valid guard without a return callback.
        resource_guard()
            : m_is_valid(true)
        {
        }

    private:
        /// @brief Constructs a resource_guard with a callback and resource
        ///
        /// @note Only resource_pool may create resource_guard instances.
        explicit resource_guard(PutbackCallbackFunc&& f, T&& src)
            : m_rsrc(std::move(src))
            , m_putback_callback(std::move(f))
        {
            m_is_valid = true;
        }

        /// @brief Constructs a resource_guard with a callback and in-place constructed resource
        ///
        /// @note Only resource_pool may create resource_guard instances.
        template <typename... Args>
        resource_guard(PutbackCallbackFunc&& f, Args&&... args)
            : m_rsrc(std::forward<Args>(args)...)
            , m_putback_callback(std::move(f))
            , m_error_code(pool_error::Ok)
        {
            m_is_valid = true;
        }

    public:
        /// @brief Copy constructor is deleted
        /// @details resource_guard is move-only to prevent resource ownership ambiguity
        /// and ensure proper RAII semantics. Only one resource_guard can own a resource.
        resource_guard(const resource_guard&) = delete;

        /// @brief Copy assignment operator is deleted
        ///
        /// @details
        /// Copy assignment is not allowed to maintain move-only semantics
        /// and prevent resource ownership ambiguity.
        resource_guard& operator=(const resource_guard&) = delete;

        /// @brief Constructs an invalid guard carrying a borrow error
        /// @param err Error reported by error()
        resource_guard(const pool_error& err)
            : m_is_valid(false)
            , m_error_code(err)
        {
        }

    public:
        /// @brief Move constructor
        ///
        /// Transfers ownership from another resource_guard to this one.
        /// The source is invalidated to prevent double-return.
        ///
        /// @param src The source resource_guard to move from
        ///
        /// @note The source's callback is cleared to prevent double-return
        /// @note The source is marked as invalid
        /// @note This constructor is using new syntax for noexcept specification based
        /// on the move-constructibility of T and the callback function.
        resource_guard(resource_guard&& src) noexcept(false)
        try
            : m_rsrc(std::move(src.m_rsrc))
            , m_putback_callback(std::move(src.m_putback_callback))
            , m_is_valid(src.m_is_valid)
            , m_error_code(src.m_error_code) {
            // This code is in the try block to ensure that if T's move constructor throws,
            // we can still safely invalidate the source and prevent double-return.
            // Its syntax is a bit unusual, but it is valid C++ and ensures exception safety.
            // Reset to ensure that the source does not double return or preserve stale error state.
            src.m_putback_callback = {};
            src.m_is_valid         = false;
            src.m_error_code       = pool_error::Ok;
        }
        catch (...) {
            // Undo everything..
            src.m_putback_callback = {};
            src.m_is_valid         = false;
            src.m_error_code       = pool_error::Ok;
            // propogate back..
            throw;
        }

        /// @brief Move assignment operator
        ///
        /// Transfers ownership from another resource_guard to this one.
        /// Before taking ownership, the currently-held resource (if valid) is returned
        /// to the pool via the putback callback. The source is then invalidated to
        /// prevent double-return.
        ///
        /// @param src The source resource_guard to move from
        /// @return Reference to this resource_guard
        ///
        /// @note Self-assignment is checked via pointer comparison
        /// @note The currently-held resource is returned to the pool before overwrite
        /// @note The source's callback is cleared to prevent double-return
        /// @note The source is marked as invalid after the move
        /// @note NOT noexcept: T's move-assignment may throw; declaring noexcept here
        ///       would call std::terminate if T::operator=(T&&) throws after the
        ///       putback callback has already fired (state would be inconsistent).
        resource_guard& operator=(resource_guard&& src)
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
                                   "resource_guard move-assignment: exception while returning current resource to pool!\n");
                    }
                    // Clear the old callback and validity now that the resource has been handed back.
                    m_putback_callback = {};
                    m_is_valid         = false;
                }

                // Disarm src prior to moving m_rsrc to ensure exception safety:
                // if T's move-assignment throws, src will not double-return its resource.
                auto src_cb            = std::move(src.m_putback_callback);
                bool src_valid         = src.m_is_valid;
                auto src_err           = src.m_error_code;

                src.m_putback_callback = {};
                src.m_is_valid         = false;
                src.m_error_code       = pool_error::Ok;

                try {
                    m_rsrc             = std::move(src.m_rsrc);
                    m_putback_callback = std::move(src_cb);
                    m_is_valid         = src_valid;
                    m_error_code       = src_err;
                }
                catch (...) {
                    m_putback_callback = {};
                    m_is_valid         = false;
                    m_error_code       = pool_error::Unknown;
                    throw;
                }
            }
            return *this;
        }

    public:
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
        ~resource_guard() noexcept
        {
            // Invoke callback if it exists, passing the resource and validity status
            // The callback (typically resource_pool::return_to_pool) decides whether to
            // reuse the resource (if valid) or discard it (if invalid)
            if (m_putback_callback) {
                try {
                    m_putback_callback(std::move(m_rsrc), m_is_valid);
                }
                catch (...) {
                    std::print(std::cerr, "resource_guard destructor: exception while invoking putback callback!\n");
                }
                m_is_valid         = false;
                m_putback_callback = {};
            }
        }

    public:
        /// @brief Dereference operator to access the wrapped resource
        /// @return Reference to the wrapped resource
        /// @warning Does not check validity; do not use after invalidation or move-out.
        auto operator*() -> T& { return m_rsrc; }

        /// @brief Pointer-like access to the wrapped resource
        /// @return Pointer to the wrapped resource, or nullptr if invalid
        /// @note Returns nullptr if resource is invalid
        auto operator->() -> T* { return m_is_valid ? &m_rsrc : nullptr; }

        /// @brief Provides const pointer-like access to the wrapped resource.
        /// @return The resource address, or nullptr if the guard is invalid.
        auto operator->() const -> const T* { return m_is_valid ? &m_rsrc : nullptr; }

        /// @brief Explicit conversion to resource reference
        /// @return Reference to the wrapped resource
        /// @warning Does not check validity; do not use after invalidation or move-out.
        explicit operator T&() & { return m_rsrc; }

        /// @brief Provides a const reference to the wrapped resource.
        /// @warning Does not check validity; do not use after invalidation or move-out.
        explicit operator const T&() const& { return m_rsrc; }

        /// @brief Tests whether the guard holds a resource eligible for return.
        /// @return true when the guard is valid
        explicit operator bool() const noexcept { return m_is_valid; }

        /// @brief Converts through a conversion supplied by the stored resource type.
        /// @tparam InnerType Requested conversion target.
        /// @return The result of converting the stored resource to InnerType.
        template <typename InnerType>
            requires std::convertible_to<const T&, InnerType>
        operator InnerType() const
        {
            return static_cast<InnerType>(m_rsrc);
        }

        /// @brief Assignment operator for resource value
        ///
        /// Replaces the held resource value in place. The old resource is returned to
        /// the pool, and the guard retains ownership of the new resource, which will be
        /// returned to the pool when destroyed.
        ///
        /// @param src The new resource value (moved)
        /// @return Reference to this resource_guard
        ///
        /// @note Returns existing resource to pool before taking ownership of new resource.
        resource_guard& operator=(T&& src)
        {
            if (m_putback_callback) {
                try {
                    m_putback_callback(std::move(m_rsrc), m_is_valid);
                }
                catch (...) {
                    std::print(std::cerr, "resource_guard operator=(T&&): exception while returning old resource to pool!\n");
                }
            }
            m_rsrc     = std::move(src);
            m_is_valid = true;
            return *this;
        }

    public:
        /// @brief Marks the resource as invalid (abandoned)
        ///
        /// Sets the validity flag to false. When the resource is destroyed, the callback
        /// will be invoked with isvalid=false, allowing the pool to discard the resource
        /// rather than returning it for reuse. This is appropriate when the resource has
        /// been moved out, corrupted, or otherwise rendered unusable.
        ///
        /// @note Virtual for interface consistency, but resource_guard is `final`,
        ///       so there is currently no derived class to override this.
        /// @note The callback is still invoked; only the validity flag changes
        /// @note Typically called when the resource is corrupted, moved out, or consumed
        ///
        /// @example
        /// @code
        /// auto resource = pool.try_borrow();
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
        /// @note Virtual for interface consistency, but resource_guard is `final`,
        ///       so there is currently no derived class to override this.
        /// @note Const: Does not modify the resource
        virtual bool is_valid() const { return m_is_valid; }

        /// @brief Sets the error reported by error().
        /// @param err Error code to store.
        /// @return This guard.
        auto& set_error(pool_error err)
        {
            m_error_code = err;
            m_is_valid   = (err == pool_error::Ok); // Mark invalid if error is not Ok
            return *this;
        }
        /// @brief Gets the error associated with this guard.
        /// @return The stored error code; valid guards normally report pool_error::Ok.
        pool_error error() const { return m_error_code; }

        /// @brief Tests whether the guard holds a valid resource.
        /// @return true when is_valid() would return true.
        virtual bool has_value() const { return m_is_valid; }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
    public:
        /// @brief Serializes the resource_guard to JSON
        ///
        /// Returns a JSON object containing the resource state and validity.
        /// Only available if nlohmann/json.hpp is included before this header.
        ///
        /// @return JSON object with:
        ///   - _typver: Type and version string ("siddiqsoft.arrp.resource_guard/1.0.0")
        ///   - valid: Whether the resource is valid (boolean)
        ///   - value: The resource value (if serializable, otherwise "-noserializer-")
        ///
        /// @note Available only when nlohmann/json.hpp was included before this header.
        /// @note If T is not serializable, value is set to "-noserializer-"
        ///
        /// @par JSON Schema:
        /// @code{.json}
        /// {
        ///   "_typver": "siddiqsoft.arrp.resource_guard/1.0.0",
        ///   "valid": true,
        ///   "value": <resource_value>
        /// }
        /// @endcode
        nlohmann::json to_json() const
        {
            if constexpr (std::is_same_v<T, std::string> || std::is_arithmetic_v<T>)
                return {{"_typver", "siddiqsoft.arrp.resource_guard/1.0.0"}, {"valid", m_is_valid}, {"value", m_rsrc}};
            else if constexpr (HasStdToStringImpl<T>)
                return {{"_typver", "siddiqsoft.arrp.resource_guard/1.0.0"},
                        {"valid", m_is_valid},
                        {"value", std::to_string(m_rsrc)}};
            else
                return {{"_typver", "siddiqsoft.arrp.resource_guard/1.0.0"}, {"valid", m_is_valid}, {"value", "-noserializer-"}};
        }
#endif
    };
} // namespace siddiqsoft::arrp


/// @brief Specialization of std::formatter for resource_guard
/// @details Provides formatted output for resource_guard instances using std::format.
/// Uses a consistent format across all translation units to avoid ODR violations.
/// @tparam T The resource type
/// @note This formatter always uses the same format regardless of whether nlohmann/json is available,
///       ensuring ODR safety. For JSON output, use the to_json() method directly.
template <typename T>
struct std::formatter<siddiqsoft::arrp::resource_guard<T>> : std::formatter<std::string>
{
    /// @brief Format the resource_guard
    /// @param sr The resource_guard to format
    /// @param ctx Format context
    /// @return Iterator to end of formatted output
    /// @note Always uses the same format to ensure ODR safety across translation units
    auto format(const siddiqsoft::arrp::resource_guard<T>& sr, auto& ctx) const
    {
        // Use consistent format across all TUs to avoid ODR violations
        // Format: resource_guard<T>{valid: <bool>}
        return std::format_to(ctx.out(), "resource_guard<T>{{valid: {}}}", sr.is_valid());
    }
};

#endif
