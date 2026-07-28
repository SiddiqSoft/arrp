# Auto Returning Resource Pool

<img src="https://gravatar.com/avatar/b22603b65d11dcab44885c65e44f7dc9" align=right>

![](https://img.shields.io/github/v/tag/SiddiqSoft/arrp)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status%2FSiddiqSoft.arrp?branchName=master)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=33&branchName=master)
![](https://img.shields.io/nuget/v/SiddiqSoft.arrp)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/33)

A thread-safe, header-only C++20 resource pool library with automatic lifecycle management using RAII principles.

## Features

- **Thread-Safe**: All operations protected by mutexes for concurrent access
- **RAII Pattern**: Automatic resource return via scoped_resource wrapper
- **Capacity Management**: Enforces maximum capacity limits to prevent unbounded growth
- **FIFO Ordering**: Predictable resource ordering (first-in, first-out)
- **Customizable Factory**: Support for custom resource creation callbacks
- **Error Handling**: Uses `std::expected` for safe error handling without exceptions
- **Modern C++20**: Uses only standard library features (no external dependencies).
- **Optional**: JSON serialization for pool state monitoring via nlohmann json
- **Type-Safe**: Leverages C++20 concepts for compile-time type checking
- **Move Semantics**: Efficient resource transfer with perfect forwarding
- **JSON Diagnostics**: Optional JSON serialization for pool state monitoring

## Quick Start

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <memory>

// Create a resource pool with capacity and auto-grow policy
siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool(
    10,  // capacity
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);

// Populate the pool
for (int i = 0; i < 10; ++i) {
    pool.add_to_pool(std::make_shared<DatabaseConnection>());
}

// Borrow and use a resource
{
    auto conn = pool.borrow_from_pool();
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

- **C++20 Support**: Requires `std::deque`, `std::mutex`, `std::concepts`, and `std::expected`
- **Compiler Support**:
  - GCC 10+
  - MSVC 16.11+ (Visual Studio 2019 or later)
  - Clang 10+
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

int main() {
    // Create pool with default capacity and NoGrow policy
    siddiqsoft::arrp::resource_pool<std::string> pool(
        10,  // capacity
        siddiqsoft::arrp::auto_add_policy::NoGrow
    );
    
    // Add resources
    pool.add_to_pool(std::string("resource-1"));
    pool.add_to_pool(std::string("resource-2"));
    
    // Borrow and use
    {
        auto res = pool.borrow_from_pool();
        if (res) {
            std::cout << *res << std::endl;
        }
    }
    
    return 0;
}
```

### AutoGrow Policy

```cpp
// Pool automatically creates resources on demand
siddiqsoft::arrp::resource_pool<std::string> pool(
    10,  // capacity
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);

// Resources are created automatically up to capacity
for (int i = 0; i < 100; ++i) {
    auto res = pool.borrow_from_pool();
    if (res) {
        // Use resource
    }
}
```

### Custom Factory

```cpp
siddiqsoft::arrp::resource_pool<DatabaseConnection> pool(
    10,  // capacity
    [](auto& my_pool) -> std::expected<siddiqsoft::arrp::scoped_resource<DatabaseConnection>, siddiqsoft::arrp::pool_error> {
        // Create new resource
        auto conn = DatabaseConnection::create();
        
        // Return wrapped with auto-return callback
        return siddiqsoft::arrp::scoped_resource<DatabaseConnection>{
            [&my_pool](DatabaseConnection&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                if (isvalid) {
                    return my_pool.add_to_pool(std::move(res));
                }
                return {};
            },
            std::move(conn)
        };
    }
);
```

### Multi-threaded Usage

```cpp
#include <thread>
#include <vector>

siddiqsoft::arrp::resource_pool<DatabaseConnection> pool(
    10,  // capacity
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);

// Pre-populate pool
for (int i = 0; i < 10; ++i) {
    pool.add_to_pool(std::make_shared<DatabaseConnection>());
}

// Use from multiple threads
std::vector<std::jthread> threads;
for (int t = 0; t < 4; ++t) {
    threads.emplace_back([&pool]() {
        for (int i = 0; i < 100; ++i) {
            auto conn = pool.borrow_from_pool();
            if (conn) {
                conn->query("SELECT * FROM users");
                // Automatically returned
            }
        }
    });
}
// jthread joins automatically
```

### Monitoring Pool State

```cpp
// Get pool statistics
auto state = pool.to_json();

if (state) {
    auto& json = state.value().get();
    std::cout << "Capacity: " << json["capacity"] << std::endl;
    std::cout << "Available: " << json["size"] << std::endl;
    std::cout << "Deficit: " << json["deficit"] << std::endl;
    std::cout << "Total borrows: " << json["borrows"] << std::endl;
    std::cout << "Total returns: " << json["returns"] << std::endl;
}
```

## API Overview

### [resource_pool](https://siddiqsoft.github.io/arrp/api.html#api_resource_pool) Methods

| Method | Returns | Description 
|--------|---------|----------------------
| `borrow_from_pool()` | `std::expected<scoped_resource<T>, pool_error>` | Borrow a resource from the pool.<br/>Returns error if pool is exhausted and no factory callback available.<br/>If pool is under capacity, factory callback is invoked to create new resource.
| `add_to_pool(T&&)` | `std::expected<void, pool_error>` | Add a resource to the pool.<br/>The move-semantics is required as the resource must be returned exclusively to the pool.<br/>If the `scoped_resource` is marked invalid then the resource will not be claimed back.
| `size()` | `std::expected<size_t, pool_error>` | Get number of available resources in pool
| `clear()` | `std::expected<void, pool_error>` | Remove all resources from pool
| `to_json()` | `std::expected<std::reference_wrapper<nlohmann::json>, pool_error>` | Get pool state as JSON (requires nlohmann/json)

### [scoped_resource](https://siddiqsoft.github.io/arrp/api.html#api_scoped_resource) Methods

| Method | Description |
|--------|-------------|
| `operator*()` | Dereference wrapped resource |
| `operator->()` | Pointer-like access to wrapped resource |
| `invalidate()` | Mark resource as invalid (prevent auto-return) |
| `is_valid()` | Check if resource is valid |

For complete API documentation, see the [API Reference](https://siddiqsoft.github.io/arrp/api.html).

## Thread Safety

All public methods of `resource_pool` are thread-safe:
- Multiple threads can safely call `borrow_from_pool()` and `add_to_pool()` concurrently
- The pool uses internal mutexes to protect shared state
- No external synchronization is required
- Atomic counters for lock-free statistics

## Error Handling

The resource_pool uses `std::expected<T, pool_error>` for error handling:
- **borrow_from_pool()**: Returns error if pool is exhausted and no factory callback available
- **add_to_pool()**: Returns error if pool is shutting down
- **clear()**: Returns error if pool is shutting down
- **size()**: Returns error if pool is shutting down
- **to_json()**: Returns error if pool is shutting down

Error types are defined in `pool_error` enum:
- `NoMoreResources`: Pool is exhausted
- `UnderCapacityNoAutoGrow`: Pool is under capacity but no auto-grow policy
- `ShutdownInitiated`: Pool is shutting down
- `Unknown`: Unknown error

## Best Practices

1. **Always use RAII**: Let `scoped_resource` handle resource return. You can use derived classes that--for example--specialize the handling of `CURL*`.
2. **Pre-populate pools**: Add resources before concurrent access
3. **Handle errors**: Check `std::expected` return values from `borrow_from_pool()`
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
