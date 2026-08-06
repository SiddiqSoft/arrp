/**
@mainpage arrp

@section overview Overview

**arrp** is a header-only C++23 library for reusing scarce, moveable resources.
`resource_pool<T>` owns available resources, and a move-only `resource_guard<T>`
returns a borrowed resource when the guard is destroyed.

All public types live in the `siddiqsoft::arrp` namespace. Include the complete
public API with:

@code{.cpp}
#include <siddiqsoft/arrp.hpp>
@endcode

@section quick_start Quick start

Seed a pool before borrowing from it:

@code{.cpp}
#include <siddiqsoft/arrp.hpp>
#include <string>

int main()
{
    siddiqsoft::arrp::resource_pool<std::string> pool {4};
    pool.seed("connection-1");

    if (auto resource = pool.try_borrow(); resource) {
        resource->append("-in-use");
    } // The guard returns the string here.
}
@endcode

Resources are stored and borrowed in FIFO order. A pool does not create resources
by itself; either seed resources explicitly or register a factory and use
`try_borrow_create()`.

@section concepts Core types

- `resource_pool<T, SRT>` owns available resources and synchronizes pool access.
- `resource_guard<T>` is the move-only RAII handle returned by borrowing.
- `pool_error` describes an unsuccessful borrow or seed during shutdown.
- `release_reason` distinguishes valid and abandoned releases.
- `resource_pool_limits` provides the capacity defaults and bounds.

`T` must satisfy `NonNumericMoveConstructible`: it must be move-constructible,
move-assignable, and not an arithmetic type. Smart pointers, containers, handles,
and user-defined resource classes are suitable resource types.

@section behavior Pool behavior

Borrowing, seeding, clearing, sizing, and JSON statistics synchronize access to
the pool. Configure a factory before concurrent borrowing; a `resource_guard` is
not thread-safe and must have one owning thread at a time.

The constructor capacity is clamped to the range 1 through 255 and is exposed in
statistics. It is not currently enforced as a hard limit on `seed()` or factory
creation; callers are responsible for keeping the number of resources appropriate
for their application.

Borrowing returns an invalid guard rather than throwing for normal pool conditions:

@code{.cpp}
auto resource = pool.try_borrow();
if (!resource) {
    if (resource.error() == siddiqsoft::arrp::pool_error::NoMoreResources) {
        // No available resource.
    }
}
@endcode

@section lifecycle Lifetime rules

Destroy every `resource_guard` before its originating `resource_pool`. A guard
keeps the callback used to return its resource, so allowing it to outlive the pool
would invoke a callback on a destroyed pool.

Call `invalidate()` when a borrowed resource is corrupted or has been moved out.
An invalid guard is discarded instead of returned:

@code{.cpp}
auto resource = pool.try_borrow();
if (resource) {
    auto extracted = static_cast<std::string>(std::move(resource));
    // extracted is no longer owned by the pool.
}
@endcode

`clear()` removes only resources that are currently available. Resources already
borrowed can still return when their guards are destroyed.

@section json JSON statistics

JSON support is available only when `nlohmann/json.hpp` is included before
`siddiqsoft/arrp.hpp`:

@code{.cpp}
#include <nlohmann/json.hpp>
#include <siddiqsoft/arrp.hpp>
#include <iostream>

auto stats = pool.to_json();
std::cout << stats.dump(2) << '\n';
@endcode

See @ref api_reference for every public operation and @ref usage_guide for
complete usage patterns.
*/
