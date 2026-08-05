/**
@page api_reference API reference

@section api_include Include and requirements

@code{.cpp}
#include <siddiqsoft/arrp.hpp>
@endcode

The library requires C++23. `resource_pool<T>` and `resource_guard<T>` require a
non-arithmetic `T` that is move-constructible and move-assignable
(`NonNumericMoveConstructible<T>`).

To expose JSON members, include nlohmann JSON first:

@code{.cpp}
#include <nlohmann/json.hpp>
#include <siddiqsoft/arrp.hpp>
@endcode

@section resource_pool_api resource_pool<T, SRT>

`resource_pool<T, SRT>` is a non-copyable, non-movable pool with synchronized
storage operations. `SRT` defaults to `resource_guard<T>` and must derive from it.

@subsection pool_constructors Constructors

| Declaration | Behavior |
|---|---|
| `resource_pool(uint8_t init_capacity = resource_pool_limits::DefaultCapacity, std::function<void(T&)>&& on_shutdown_callback = {})` | Creates a pool. The reported capacity is clamped to 1-255. |
| `resource_pool(std::function<void(T&)>&& on_shutdown_callback)` | Creates a pool with the default capacity of 8. |

The cleanup callback is called for each resource available when `clear()` or the
destructor removes it. It executes while the pool lock is held and must not call
pool methods. Exceptions derived from `std::exception` are caught and written to
standard error.

@subsection pool_methods Methods

| Declaration | Result |
|---|---|
| `SRT try_borrow(std::chrono::nanoseconds timeout = {})` | Borrows the next available resource. A zero timeout does not wait; a positive timeout waits for an available resource. |
| `SRT try_borrow_create(std::chrono::nanoseconds timeout = {})` | Borrows an available resource, or invokes the factory callback when no resource becomes available. |
| `template<class F> void set_factory_callback(F&& f)` | Registers a no-argument factory returning `T` or `SRT`. |
| `template<class... Args> pool_error seed(Args&&... args)` | Constructs `T` in place and adds it to the available pool. |
| `pool_error seed(T&& item)` | Moves an existing resource into the available pool. |
| `pool_error clear()` | Removes all resources currently available in the pool. |
| `auto size() const` | Returns the number of currently available resources (`size_t`). |
| `nlohmann::json to_json() const` | Returns pool statistics when JSON support is enabled. |

`seed()` returns `pool_error::Ok` unless destruction has begun, in which case it
returns `pool_error::ShutdownInitiated`. The configured capacity is statistic and
guidance only: neither `seed()` nor factory creation currently reject additions
above it.

The factory is used only by `try_borrow_create()`, never by `try_borrow()`. It
must not call pool methods. A factory that returns `T` is the normal form:

@code{.cpp}
pool.set_factory_callback([] {
    return connection::open();
});

auto connection = pool.try_borrow_create();
@endcode

@subsection pool_borrow_errors Borrow errors

Borrow methods return an invalid `SRT` with one of these errors:

| Error | Meaning |
|---|---|
| `NoMoreResources` | No resource was available and no applicable factory was used. |
| `Timeout` | `try_borrow()` waited for the specified positive timeout without a resource. |
| `ShutdownInitiated` | Pool destruction has begun. |
| `Unknown` | A factory or borrow implementation exception was caught. |

@subsection pool_statistics Statistics

`to_json()` returns an object with these keys:

| Key | Meaning |
|---|---|
| `_typver` | `"siddiqsoft.arrp.resource_pool/0.0.0"` |
| `capacity` | Constructor capacity after clamping. |
| `size` | Available resources. |
| `deficit` | `capacity - (size + checked-out resources)`. |
| `peaksize` | Highest available-pool size observed. |
| `seeds` | Calls that successfully added a resource with `seed()`. |
| `autoadds` | Resources created by the factory. |
| `borrows` | Successful borrows. |
| `returns` | Valid resources returned by guards. |
| `abandons` | Invalidated resources discarded by guards. |
| `loans` | `borrows - returns - abandons`. |
| `items` | Available items for `std::string` and `nlohmann::json` resources only. |

@section resource_guard_api resource_guard<T>

`resource_guard<T>` is the non-copyable, movable RAII handle returned by a pool.
Its destructor calls the pool's return callback. Do not let a guard outlive its
pool, and do not access a single guard concurrently.

| Declaration | Behavior |
|---|---|
| `explicit operator bool() const` / `has_value()` | True when the guard holds a valid resource. |
| `is_valid()` | Reports whether destruction will return the resource. |
| `pool_error error() const` | Returns the error associated with an invalid borrow result. |
| `T& operator*()` | Accesses the stored resource. Do not use after invalidation. |
| `T* operator->()` | Returns the resource address, or `nullptr` after invalidation. |
| `explicit operator T() &&` | Moves the resource out and invalidates the guard. |
| `explicit operator T&() &` | Provides a reference to the resource. |
| `resource_guard& operator=(T&&)` | Returns the current resource, then stores the replacement. |
| `resource_guard& operator=(resource_guard&&)` | Returns the current resource, then takes ownership from another guard. |
| `virtual void invalidate()` | Marks the resource abandoned; it will be discarded on destruction. |

When JSON support is enabled, `resource_guard<T>::to_json()` returns:

@code{.json}
{
  "_typver": "siddiqsoft.arrp.resource_guard/1.0.0",
  "valid": true,
  "value": "..."
}
@endcode

The value is serialized for `std::string` and types with `std::to_string`; other
resource types use `"-noserializer-"`.

@section common_types Common types

| Type | Values or purpose |
|---|---|
| `resource_pool_limits` | `MinimumCapacity` (1), `DefaultCapacity` (8), `MaxCapacity` (255). |
| `pool_error` | `Ok`, `NoMoreResources`, `ShutdownInitiated`, `Timeout`, `Unknown`. |
| `release_reason` | `Valid`, `Abandoned`, `Unknown`. |
*/
