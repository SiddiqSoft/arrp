#pragma once
#ifndef ARRP_COMMON_HPP
#define ARRP_COMMON_HPP

#include <cstdint>
#include <limits>
#include <expected>
#include <exception>


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
        MaxCapacity     = std::numeric_limits<uint8_t>::max()
    };

    /// @brief Controls auto-grow behavior for resource pools
    ///
    /// @details
    /// Determines whether the resource pool automatically creates new resources
    /// when the pool is starving (empty but under capacity).
    ///
    /// @note NoGrow: Pool does not create new resources; returns error when exhausted
    /// @note AutoGrow: Pool creates new resources on-demand up to capacity limit
    enum class auto_add_policy
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
    enum class pool_error
    {
        NoMoreResources,         ///< Pool is exhausted and no factory callback available
        UnderCapacityNoAutoGrow, ///< Pool is under capacity but auto-grow is disabled
        ShutdownInitiated,       ///< Pool is shutting down
        Unknown                  ///< Unknown error
    };
} // namespace siddiqsoft::arrp


/// @brief Specialization of std::formatter for release_reason
/// @details Provides formatted output for release_reason enum values
template <class ct>
struct std::formatter<siddiqsoft::arrp::release_reason, ct> : std::formatter<ct>
{
    /// @brief Format the release_reason
    /// @param rr The release_reason to format
    /// @param ctx Format context
    /// @return Iterator to end of formatted output
    template <typename FormatContext>
    auto format(const siddiqsoft::arrp::release_reason& rr, FormatContext& ctx) const
    {
        std::string_view val {};
        switch (rr) {
            case siddiqsoft::arrp::release_reason::Valid: val = "Valid"; break;
            case siddiqsoft::arrp::release_reason::Abandoned: val = "abandons"; break;
            default: val = "Unknown"; break;
        }

        return std::format_to(ctx.out(), "{}", val);
    }
};

/// @brief Specialization of std::formatter for auto_add_policy
/// @details Provides formatted output for auto_add_policy enum values
template <class ct>
struct std::formatter<siddiqsoft::arrp::auto_add_policy, ct> : std::formatter<ct>
{
    /// @brief Format the auto_add_policy
    /// @param aap The auto_add_policy to format
    /// @param ctx Format context
    /// @return Iterator to end of formatted output
    template <typename FormatContext>
    auto format(const siddiqsoft::arrp::auto_add_policy& aap, FormatContext& ctx) const
    {
        std::string_view val {};
        switch (aap) {
            case siddiqsoft::arrp::auto_add_policy::NoGrow: val = "NoGrow"; break;
            case siddiqsoft::arrp::auto_add_policy::AutoGrow: val = "AutoGrow"; break;
            default: val = "Unknown"; break;
        }

        return std::format_to(ctx.out(), "{}", val);
    }
};

#endif // !ARRP_COMMON_HPP
