# Auto Returning Resource Pool

[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status%2FSiddiqSoft.arrp?branchName=master)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=33&branchName=master)

`arrp` is a header-only C++23 resource pool. A `resource_pool<T>` owns available
resources, while a move-only `resource_guard<T>` returns each borrowed resource
when the guard is destroyed.

## Requirements

- A C++23 compiler.
- A move-constructible, move-assignable, non-arithmetic resource type.
- `nlohmann/json.hpp`, included before `siddiqsoft/arrp.hpp`, only when JSON
  statistics are needed.

## Installation

### CMake

```cmake
include(FetchContent)

FetchContent_Declare(arrp
    GIT_REPOSITORY https://github.com/SiddiqSoft/arrp.git
    GIT_TAG master
)
FetchContent_MakeAvailable(arrp)

target_link_libraries(your_target PRIVATE arrp::arrp)
```

### NuGet

```bash
nuget install SiddiqSoft.aarp
```

### Manual integration

Add `include/` to your compiler's include path and include:

```cpp
#include <siddiqsoft/arrp.hpp>
```

## Basic use

Seed the pool before using `try_borrow()`:

```cpp
#include <siddiqsoft/arrp.hpp>
#include <string>

int main()
{
    siddiqsoft::arrp::resource_pool<std::string> pool {8};
    pool.seed("resource-1");

    {
        auto resource = pool.try_borrow();
        if (!resource) {
            return 1;
        }
        resource->append("-in-use");
    } // The resource returns to the pool.
}
```

Resources are borrowed and returned in FIFO order. `try_borrow()` returns an
invalid guard with `pool_error::NoMoreResources` when no resource is available.
Pass a positive `std::chrono::nanoseconds` timeout to wait for a returned
resource; expiry returns `pool_error::Timeout`.

## Creating resources on demand

Register a factory before concurrent borrowing and use `try_borrow_create()`:

```cpp
siddiqsoft::arrp::resource_pool<std::string> pool {8};
pool.set_factory_callback([] {
    return std::string {"created-on-demand"};
});

auto resource = pool.try_borrow_create();
if (resource) {
    // Use *resource.
}
```

The factory takes no arguments and must return `T` or the pool's scoped resource
type. It must not call methods on the same pool. `try_borrow()` never invokes the
factory.

## Discarding a resource

Call `invalidate()` when a resource is no longer reusable:

```cpp
auto resource = pool.try_borrow();
if (resource) {
    // Detect application-specific corruption here.
    resource.invalidate();
}
```

An invalid guard is discarded rather than returned. Moving the value out through
`static_cast<T>(std::move(guard))` also invalidates the guard.

## Threading and lifetime

Borrowing, seeding, clearing, sizing, and JSON statistics synchronize pool
storage. Individual `resource_guard` instances are not thread-safe. Do not let a
guard outlive the pool that created it.

`clear()` removes resources currently available in the pool. Borrowed resources
can still return when their guards are destroyed. An optional cleanup callback
runs under the pool lock for every resource removed by `clear()` or destruction,
so it must not call pool methods.

## Capacity and statistics

The constructor capacity is clamped to 1 through 255 and is reported in
statistics, but it is not a hard insertion limit: `seed()` and a factory can add
more resources than the configured value.

To enable JSON statistics, include nlohmann JSON first:

```cpp
#include <nlohmann/json.hpp>
#include <siddiqsoft/arrp.hpp>
#include <iostream>

auto stats = pool.to_json();
std::cout << stats.dump(2) << '\n';
```

The statistics include available `size`, configured `capacity`, `borrows`,
`returns`, `abandons`, `loans`, seeded resources, and factory-created resources.

## Documentation

- [Documentation site](https://siddiqsoft.github.io/arrp/)
- [API reference](https://siddiqsoft.github.io/arrp/api.html)
- [Usage guide](https://siddiqsoft.github.io/arrp/usage_guide.html)

## Building and testing

```bash
cmake --fresh --preset=Apple-Debug
cmake --build --preset=Apple-Debug
ctest --preset=Apple-Debug
```

## License

BSD 3-Clause License. See [LICENSE](LICENSE).
