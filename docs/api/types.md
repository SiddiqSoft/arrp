# Types & Enumerations

Defined in header `<siddiqsoft/arrp.hpp>` / `<siddiqsoft/private/common.hpp>`.

---

## Enumerations

### `resource_pool_limits`

Defines capacity bounds and defaults for resource pools.

```cpp
enum resource_pool_limits : uint8_t {
    MinimumCapacity = 1,
    DefaultCapacity = 8,
    MaxCapacity     = 255
};
```

| Constant | Value | Description |
|---|---|---|
| `MinimumCapacity` | `1` | Smallest allowed capacity configuration. |
| `DefaultCapacity` | `8` | Default capacity when unassigned in constructor. |
| `MaxCapacity` | `255` | Upper clamp limit for constructor capacity (`UCHAR_MAX`). |

---

### `pool_error`

Error codes returned by pool borrowing operations or stored inside invalid `resource_guard` instances.

```cpp
enum class pool_error : uint8_t {
    Ok = 0,
    NoMoreResources,
    ShutdownInitiated,
    Timeout,
    Unknown
};
```

| Error Code | Value | Description |
|---|---|---|
| `Ok` | `0` | Operation succeeded. |
| `NoMoreResources` | `1` | Pool is empty and no factory callback was available. |
| `ShutdownInitiated` | `2` | Operation failed because the pool is shutting down. |
| `Timeout` | `3` | Timed out waiting for an available resource. |
| `Unknown` | `4` | Unspecified or exception-backed failure. |

---

### `release_reason`

Indicates why a resource guard is releasing its underlying resource.

```cpp
enum class release_reason : uint8_t {
    Valid,
    Abandoned,
    Unknown
};
```

| Value | Description |
|---|---|
| `Valid` | Guard held a healthy resource; return it to pool for reuse. |
| `Abandoned` | Guard was invalidated or moved out; discard resource. |
| `Unknown` | Unspecified release reason. |

---

## `std::formatter` Specializations

`arrp` provides `std::formatter` specializations for formatting enums with `std::format` / `std::print`:

- `std::formatter<siddiqsoft::arrp::resource_pool_limits>`
- `std::formatter<siddiqsoft::arrp::pool_error>`
- `std::formatter<siddiqsoft::arrp::release_reason>`

```cpp
#include <siddiqsoft/arrp.hpp>
#include <format>
#include <iostream>

int main() {
    auto err = siddiqsoft::arrp::pool_error::Timeout;
    std::cout << std::format("Error status: {}\n", err); // Prints: Error status: Timeout
}
```
