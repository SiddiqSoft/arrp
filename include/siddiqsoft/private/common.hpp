#pragma once
#ifndef ARRP_COMMON_HPP
#define ARRP_COMMON_HPP

#include <cstdint>
#include <exception>
#include <format>
#include <limits>
#include <string_view>


namespace siddiqsoft::arrp
{
    /// @brief Resource pool capacity limits and defaults
    ///
    /// @details
    /// Defines the minimum, default, and maximum capacity values for resource pools.
    /// These values control how many resources can be managed by a pool.
    /// Do not use large values as this defeats the purpose of a resource_pool
    /// and shared across multiple threads.
    ///
    /// @note Values selected here have no special meaning and are only guides.
    /// @note MinimumCapacity: Smallest allowed pool size (1 resource)
    /// @note DefaultCapacity: Default pool size when not specified (8 resources)
    /// @note MaxCapacity: Largest allowed pool size (255 resources)
    enum resource_pool_limits : uint8_t
    {
        MinimumCapacity = 1,
        DefaultCapacity = 8,
        MaxCapacity     = UCHAR_MAX
    };

    /// @brief Controls auto-grow behavior for resource pools
    ///
    /// @details
    /// Determines whether the resource pool automatically creates new resources
    /// when the pool is starving (empty but under capacity).
    ///
    /// @note NoGrow: Pool does not create new resources; returns error when exhausted
    /// @note AutoGrow: Pool creates new resources on-demand up to capacity limit
    enum class auto_add_policy : uint8_t
    {
        NoGrow,  ///< Do not automatically add resources when pool is starving
        AutoGrow ///< Automatically add resources when pool is starving and under capacity
    };


    /// @brief Reason for releasing a resource back to the pool
    ///
    /// @details
    /// Indicates why a resource is being returned to the pool.
    /// Valid resources are reused; abandoned resources are discarded.
    enum class release_reason : uint8_t
    {
        Valid,     ///< Resource is valid and should be reused
        Abandoned, ///< Resource is invalid/abandoned and should be discarded
        Unknown,   ///< Default/unknown reason
    };

    /// @brief Error codes for resource pool operations
    ///
    /// @details
    /// Indicates various error conditions that can occur during pool operations.
    enum class pool_error : uint8_t
    {
        NoMoreResources,   ///< Pool is exhausted and no factory callback available
        ShutdownInitiated, ///< Pool is shutting down
        Timeout,           ///< Resource was not available within the specified timeout
        Unknown            ///< Unknown error
    };
} // namespace siddiqsoft::arrp

/// @brief Formatter for resource_pool_limits
template <>
struct std::formatter<siddiqsoft::arrp::resource_pool_limits> : std::formatter<std::string>
{
    auto format(siddiqsoft::arrp::resource_pool_limits& pl, auto& ctx) const noexcept
    {
        return std::format_to(ctx.out(), "{}", static_cast<uint8_t>(pl));
    }
};

/// @brief Formatter for auto_add_policy
template <>
struct std::formatter<siddiqsoft::arrp::auto_add_policy> : std::formatter<std::string>
{
    auto format(siddiqsoft::arrp::auto_add_policy& rr, auto& ctx) const noexcept
    {
        switch (rr) {
            case siddiqsoft::arrp::auto_add_policy::NoGrow: return std::format_to(ctx.out(), "NoGrow");
            case siddiqsoft::arrp::auto_add_policy::AutoGrow: return std::format_to(ctx.out(), "AutoGrow");
            default: return std::format_to(ctx.out(), "Unknown");
        }
    }
};

/// @brief Formatter for pool_error
template <>
struct std::formatter<siddiqsoft::arrp::pool_error> : std::formatter<std::string>
{
    auto format(siddiqsoft::arrp::pool_error& pe, auto& ctx) const noexcept
    {
        switch (pe) {
            case siddiqsoft::arrp::pool_error::NoMoreResources: return std::format_to(ctx.out(), "NoMoreResources");
            case siddiqsoft::arrp::pool_error::ShutdownInitiated: return std::format_to(ctx.out(), "ShutdownInitiated");
            default: return std::format_to(ctx.out(), "Unknown");
        }
    }
};

/// @brief Formatter for release_reason
template <>
struct std::formatter<siddiqsoft::arrp::release_reason> : std::formatter<std::string>
{
    auto format(siddiqsoft::arrp::release_reason& rr, auto& ctx) const noexcept
    {
        switch (rr) {
            case siddiqsoft::arrp::release_reason::Valid: return std::format_to(ctx.out(), "Valid");
            case siddiqsoft::arrp::release_reason::Abandoned: return std::format_to(ctx.out(), "Abandoned");
            default: return std::format_to(ctx.out(), "Unknown");
        }
    }
};

#endif // !ARRP_COMMON_HPP
