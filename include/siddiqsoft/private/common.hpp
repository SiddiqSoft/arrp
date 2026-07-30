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


namespace std
{
    /// @brief Specialization of std::formatter for release_reason
    /// @details Provides formatted output for release_reason enum values
    template <typename CharT>
    struct formatter<siddiqsoft::arrp::release_reason, CharT> : formatter<std::basic_string_view<CharT>, CharT>
    {
        auto format(const siddiqsoft::arrp::release_reason& rr, std::basic_format_context<typename std::basic_string<CharT>::iterator, CharT>& ctx) const -> decltype(ctx.out())
        {
            std::basic_string_view<CharT> val{};
            switch (rr) {
                case siddiqsoft::arrp::release_reason::Valid: val = std::basic_string_view<CharT>{"Valid"}; break;
                case siddiqsoft::arrp::release_reason::Abandoned: val = std::basic_string_view<CharT>{"Abandoned"}; break;
                default: val = std::basic_string_view<CharT>{"Unknown"}; break;
            }
            return formatter<std::basic_string_view<CharT>, CharT>::format(val, ctx);
        }
    };

    /// @brief Specialization of std::formatter for auto_add_policy
    /// @details Provides formatted output for auto_add_policy enum values
    template <typename CharT>
    struct formatter<siddiqsoft::arrp::auto_add_policy, CharT> : formatter<std::basic_string_view<CharT>, CharT>
    {
        auto format(const siddiqsoft::arrp::auto_add_policy& aap, std::basic_format_context<typename std::basic_string<CharT>::iterator, CharT>& ctx) const -> decltype(ctx.out())
        {
            std::basic_string_view<CharT> val{};
            switch (aap) {
                case siddiqsoft::arrp::auto_add_policy::NoGrow: val = std::basic_string_view<CharT>{"NoGrow"}; break;
                case siddiqsoft::arrp::auto_add_policy::AutoGrow: val = std::basic_string_view<CharT>{"AutoGrow"}; break;
                default: val = std::basic_string_view<CharT>{"Unknown"}; break;
            }
            return formatter<std::basic_string_view<CharT>, CharT>::format(val, ctx);
        }
    };

    /// @brief Specialization of std::formatter for pool_error
    /// @details Provides formatted output for pool_error enum values
    template <typename CharT>
    struct formatter<siddiqsoft::arrp::pool_error, CharT> : formatter<std::basic_string_view<CharT>, CharT>
    {
        auto format(const siddiqsoft::arrp::pool_error& pe, std::basic_format_context<typename std::basic_string<CharT>::iterator, CharT>& ctx) const -> decltype(ctx.out())
        {
            std::basic_string_view<CharT> val{};
            switch (pe) {
                case siddiqsoft::arrp::pool_error::NoMoreResources: val = std::basic_string_view<CharT>{"NoMoreResources"}; break;
                case siddiqsoft::arrp::pool_error::UnderCapacityNoAutoGrow: val = std::basic_string_view<CharT>{"UnderCapacityNoAutoGrow"}; break;
                case siddiqsoft::arrp::pool_error::ShutdownInitiated: val = std::basic_string_view<CharT>{"ShutdownInitiated"}; break;
                default: val = std::basic_string_view<CharT>{"Unknown"}; break;
            }
            return formatter<std::basic_string_view<CharT>, CharT>::format(val, ctx);
        }
    };

    /// @brief Specialization of std::formatter for resource_pool_limits
    /// @details Provides formatted output for resource_pool_limits enum values
    template <typename CharT>
    struct formatter<siddiqsoft::arrp::resource_pool_limits, CharT> : formatter<std::basic_string_view<CharT>, CharT>
    {
        auto format(const siddiqsoft::arrp::resource_pool_limits& rpl, std::basic_format_context<typename std::basic_string<CharT>::iterator, CharT>& ctx) const -> decltype(ctx.out())
        {
            std::basic_string_view<CharT> val{};
            switch (rpl) {
                case siddiqsoft::arrp::resource_pool_limits::MinimumCapacity: val = std::basic_string_view<CharT>{"MinimumCapacity"}; break;
                case siddiqsoft::arrp::resource_pool_limits::DefaultCapacity: val = std::basic_string_view<CharT>{"DefaultCapacity"}; break;
                case siddiqsoft::arrp::resource_pool_limits::MaxCapacity: val = std::basic_string_view<CharT>{"MaxCapacity"}; break;
                default: val = std::basic_string_view<CharT>{"Unknown"}; break;
            }
            return formatter<std::basic_string_view<CharT>, CharT>::format(val, ctx);
        }
    };
}

#endif // !ARRP_COMMON_HPP
