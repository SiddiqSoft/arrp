# Auto Returning Resource Pool

<img src="https://gravatar.com/avatar/b22603b65d11dcab44885c65e44f7dc9" align=right>

![](https://img.shields.io/github/v/tag/SiddiqSoft/arrp)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status%2FSiddiqSoft.arrp?branchName=master)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=33&branchName=master)
![](https://img.shields.io/nuget/v/SiddiqSoft.arrp)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/33)

A thread-safe, header-only C++23 resource pool library with automatic lifecycle management using RAII principles.

## Features

- **Thread-Safe**: All operations protected by mutexes for concurrent access
- **RAII Pattern**: Automatic resource return via `scoped_resource`
- **Capacity Management**: Enforces maximum capacity limits to prevent unbounded growth
- **FIFO Ordering**: Predictable resource ordering (first-in, first-out)
- **Factory Callback**: Support for on-demand resource creation
- **Cleanup Callback**: Optional cleanup behavior for pool destruction
- **Modern C++23**: Uses only standard library features (no external dependencies)
- **Optional**: JSON serialization for pool state monitoring via nlohmann/json
- **Type-Safe**: Leverages C++20 concepts for compile-time type checking
- **Move Semantics**: Efficient resource transfer with perfect forwarding

## Quick Start

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <memory>

// Create a resource pool with capacity
siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool(10);

// Populate the pool
for (int i = 0; i < 10; ++i) {
    pool.seed(std::make_shared<DatabaseConnection>());
}

// Borrow and use a resource
{
    auto conn = pool.try_borrow();
    if (conn) {
        conn->query("SELECT * FROM users");
        // Automatically returned to pool when going out of scope
    }
}
```

## Documentation

Complete documentation is available at: **[https://siddiqsoft.github.io/arrp/](https://siddiqsoft.github.io/arrp/)**

### Key Documentation Pages

- **[API Reference](https://siddiqsoft.github.io/arrp/api.html)** - Complete API documentation with all methods and classes
- **[Usage Guide](https://siddiqsoft.github.io/arrp/usage_guide.html)** - Detailed usage examples and best practices
- **[Main Page](https://siddiqsoft.github.io/arrp/index.html)** - Overview and introduction

## Requirements

- **C++23 Support**: Requires `std::deque`, `std::mutex`, `std::concepts`, `std::print`, and other C++23 features
- **Compiler Support**:
  - **GCC 12+**
  - **MSVC 19.33+**
  - **Clang 16+**
- **Platform Support**: Windows, Linux, macOS
- **Optional**: nlohmann/json for JSON serialization support

## Installation

### Using CMake (Recommended)

```cmake
include(FetchContent)
FetchContent_Declare(arrp
    GIT_REPOSITORY https://github.com/SiddiqSoft/arrp.git
    GIT_TAG main
)
FetchContent_MakeAvailable(arrp)

target_link_libraries(your_target PRIVATE arrp::arrp)
```

### Using NuGet (Windows)

```bash
nuget install SiddiqSoft.aarp
```

### Manual Integration

Simply include the header files from `include/siddiqsoft/` in your project.

## Usage Examples

### Basic Pool Usage

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <iostream>

int main() {
    siddiqsoft::arrp::resource_pool<std::string> pool(10);

    pool.seed(std::string("resource-1"));
    pool.seed(std::string("resource-2"));

    {
        auto res = pool.try_borrow();
        if (res) {
            std::cout << *res << std::endl;
        }
    }

    return 0;
}
```

### Factory Callback

```cpp
#include "siddiqsoft/resource_pool.hpp"

siddiqsoft::arrp::resource_pool<DatabaseConnection> pool(10);

pool.set_factory_callback([]() {
    return DatabaseConnection::create();
});

auto conn = pool.try_borrow_create();
if (conn) {
    conn->query("SELECT * FROM users");
}
```

### Multi-threaded Usage

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <thread>
#include <vector>

siddiqsoft::arrp::resource_pool<DatabaseConnection> pool(10);
for (int i = 0; i < 10; ++i) {
    pool.seed(std::make_shared<DatabaseConnection>());
}

std::vector<std::jthread> threads;
for (int t = 0; t < 4; ++t) {
    threads.emplace_back([&pool]() {
        for (int i = 0; i < 100; ++i) {
            auto conn = pool.try_borrow();
            if (conn) {
                conn->query("SELECT * FROM users");
            }
        }
    });
}
```

### Monitoring Pool State

```cpp
#include "nlohmann/json.hpp"

auto stats = pool.to_json();
std::cout << stats.dump(2) << std::endl;
```

## API Overview

### `resource_pool` Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `try_borrow()` | `scoped_resource<T>` | Borrow a resource from the pool. Returns an invalid scoped resource with `error()` set when unavailable.
| `try_borrow_create()` | `scoped_resource<T>` | Borrow a resource or create one on demand using the registered factory callback.
| `seed(Args&&...)` | `pool_error` | Add a resource to the pool by constructing it in place.
| `seed(T&&)` | `pool_error` | Add a moved resource to the pool.
| `size()` | `size_t` | Get the number of available resources in the pool.
| `clear()` | `pool_error` | Remove all resources from the pool.
| `set_factory_callback(F&&)` | `void` | Register a callback to create resources when the pool is empty.
| `to_json()` | `nlohmann::json` | Export pool statistics to JSON when nlohmann/json is available.

### `scoped_resource` Methods

| Method | Description |
|--------|-------------|
| `operator*()` | Dereference the wrapped resource |
| `operator->()` | Pointer-like access to the wrapped resource |
| `invalidate()` | Mark resource as invalid so it is discarded on destruction |
| `is_valid()` | Check whether the resource is valid |
| `error()` | Retrieve the failure code when the wrapper is invalid |

## Thread Safety

All public methods of `resource_pool` are thread-safe:
- Multiple threads can safely call `try_borrow()` and `seed()` concurrently
- The pool uses internal mutexes to protect shared state
- No external synchronization is required
- Scoped resource wrappers themselves are not thread-safe

## Error Handling

The library uses `pool_error` for error reporting:
- `NoMoreResources`: Pool exhausted
- `ShutdownInitiated`: Pool is shutting down
- `Timeout`: Wait timed out while borrowing
- `Unknown`: Unknown error

Usage notes:
- `try_borrow()` and `try_borrow_create()` return a `scoped_resource<T>` that may be invalid
- `seed()` and `clear()` return `pool_error`
- `size()` returns `size_t`
- `to_json()` returns `nlohmann::json` when JSON support is enabled

- `ShutdownInitiated`: Pool is shutting down
- `Unknown`: Unknown error

## Best Practices

1. **Always use RAII**: Let `scoped_resource` handle resource return. You can use derived classes that--for example--specialize the handling of `CURL*`.
2. **Pre-populate pools**: Add resources before concurrent access
3. **Handle errors**: Check `std::expected` return values from `try_borrow()`
4. **Keep factories simple**: Factory callbacks should only create resources
5. **Monitor utilization**: Use `to_json()` to track pool health
6. **Use appropriate types**: Prefer `shared_ptr` or `unique_ptr` over raw pointers
7. **Test concurrency**: Verify thread safety with your specific use case
8. **Invalidate when needed**: Use `invalidate()` when resource is corrupted or moved out

## Constraints

- Capacity limited to 255 resources (uint8_t). If you'd like more or customizable, please open an issue. Generally, resources are meant to be scarce and therefore the pool must be small.
- Factory callbacks must not call pool methods (would cause deadlock). The sole purpose of the factory callback would be to perform special initialization for your resource (and chose to not perform such tasks via derived classes.)
- Resources must be move-constructible and non-arithmetic types
- Counters wrap around after ~18 quintillion operations (uint64_t)

## Building and Testing

```bash
cmake --fresh --preset=Apple-Debug
cmake --build --preset=Apple-Debug
ctest --preset=Apple-Debug
```

```bash
cmake --fresh --preset=Windows-arm64-Release
cmake --build --preset=Windows-arm64-Release
ctest --preset=Windows-arm64-Release
```

## License

BSD 3-Clause License - See [LICENSE](LICENSE) file for details

## Copyright

Copyright (c) 2026, Abdulkareem Siddiq. All rights reserved.

## Links

- **GitHub Repository**: https://github.com/SiddiqSoft/arrp
- **NuGet Package**: https://www.nuget.org/packages/SiddiqSoft.aarp/
- **Documentation**: https://siddiqsoft.github.io/arrp/
- **API Reference**: https://siddiqsoft.github.io/arrp/api.html
- **Usage Guide**: https://siddiqsoft.github.io/arrp/usage_guide.html

## Contributing

Contributions are welcome! Please ensure:
- Code follows the existing style
- All tests pass
- New features include tests
- Documentation is updated

## Support

For issues, questions, or suggestions, please open an issue on GitHub.
