# ARRP - Auto Returning Resource Pool for Modern C++

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/SiddiqSoft/arrp)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-BSD%203--Clause-green)](LICENSE)

A thread-safe, header-only C++20 resource pool library with automatic lifecycle management using RAII principles.

## Features

- **Thread-Safe**: All operations protected by mutexes for concurrent access
- **RAII Pattern**: Automatic resource return via scoped_resource wrapper
- **Capacity Management**: Enforces maximum capacity limits to prevent unbounded growth
- **FIFO Ordering**: Predictable resource ordering (first-in, first-out)
- **Customizable Factory**: Support for custom resource creation callbacks
- **Exception Safe**: Strong exception guarantees with automatic cleanup
- **Modern C++20**: Uses only standard library features (no external dependencies)
- **Type-Safe**: Leverages C++20 concepts for compile-time type checking
- **Move Semantics**: Efficient resource transfer with perfect forwarding
- **JSON Diagnostics**: Optional JSON serialization for pool state monitoring

## Quick Start

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <memory>

// Create a resource pool
siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;

// Populate the pool
for (int i = 0; i < 10; ++i) {
    pool.checkin(std::make_shared<DatabaseConnection>());
}

// Borrow and use a resource
{
    auto conn = pool.checkout();
    conn->query("SELECT * FROM users");
    // Automatically returned to pool when going out of scope
}
```

## Documentation

Complete documentation is available at: **[https://siddiqsoft.github.io/arrp/](https://siddiqsoft.github.io/arrp/)**

### Key Documentation Pages

- **[API Reference](https://siddiqsoft.github.io/arrp/api.html)** - Complete API documentation with all methods and classes
- **[Usage Guide](https://siddiqsoft.github.io/arrp/usage_guide.html)** - Detailed usage examples and best practices
- **[Main Page](https://siddiqsoft.github.io/arrp/index.html)** - Overview and introduction

## Requirements

- **C++20 Support**: Requires `std::deque`, `std::mutex`, and `std::concepts`
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
    // Create pool with NoGrow policy (throws when empty)
    siddiqsoft::arrp::resource_pool<std::string> pool;
    
    // Add resources
    pool.checkin(std::string("resource-1"));
    pool.checkin(std::string("resource-2"));
    
    // Borrow and use
    {
        auto res = pool.checkout();
        std::cout << *res << std::endl;
    }
    
    return 0;
}
```

### AutoGrow Policy

```cpp
// Pool automatically creates resources on demand
siddiqsoft::arrp::resource_pool<std::string> pool(
    siddiqsoft::arrp::resource_pool<std::string>::auto_add_policy::AutoGrow
);

// Resources are created automatically up to capacity
for (int i = 0; i < 100; ++i) {
    auto res = pool.checkout();
    // Use resource
}
```

### Custom Factory

```cpp
siddiqsoft::arrp::resource_pool<DatabaseConnection> pool(
    [](auto& p) -> siddiqsoft::arrp::scoped_resource<DatabaseConnection> {
        return siddiqsoft::arrp::scoped_resource<DatabaseConnection>(
            DatabaseConnection::create(),
            [&p](DatabaseConnection&& conn) { 
                p.checkin(std::move(conn)); 
            }
        );
    }
);
```

### Multi-threaded Usage

```cpp
#include <thread>
#include <vector>

siddiqsoft::arrp::resource_pool<DatabaseConnection> pool;

// Pre-populate pool
for (int i = 0; i < 10; ++i) {
    pool.checkin(std::make_shared<DatabaseConnection>());
}

// Use from multiple threads
std::vector<std::jthread> threads;
for (int t = 0; t < 4; ++t) {
    threads.emplace_back([&pool]() {
        for (int i = 0; i < 100; ++i) {
            try {
                auto conn = pool.checkout();
                conn->query("SELECT * FROM users");
            } catch (const std::runtime_error&) {
                // Pool exhausted
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

std::cout << "Capacity: " << state["capacity"] << std::endl;
std::cout << "Available: " << state["size"] << std::endl;
std::cout << "Checked out: " << state["checkedout"] << std::endl;
std::cout << "Total borrows: " << state["counters"]["borrow"] << std::endl;
```

## API Overview

### resource_pool Methods

| Method | Description | Returns |
|--------|-------------|---------|
| `checkout()` | Borrow a resource from the pool | `scoped_resource<T>` |
| `checkin(T&&)` | Return a resource to the pool | `void` |
| `size()` | Get number of available resources | `size_t` |
| `clear()` | Remove all resources from pool | `void` |
| `to_json()` | Get pool state as JSON | `nlohmann::json` |

### scoped_resource Methods

| Method | Description |
|--------|-------------|
| `operator*()` | Dereference wrapped resource |
| `invalidate()` | Mark resource as invalid (prevent auto-return) |

For complete API documentation, see the [API Reference](https://siddiqsoft.github.io/arrp/api.html).

## Thread Safety

All public methods of `resource_pool` are thread-safe:
- Multiple threads can safely call `checkout()` and `checkin()` concurrently
- The pool uses internal mutexes to protect shared state
- No external synchronization is required
- Atomic counters for lock-free statistics

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `checkout()` | O(1) amortized | Resource creation outside lock |
| `checkin()` | O(1) amortized | Simple push_back operation |
| `size()` | O(1) | With lock acquisition |
| `clear()` | O(n) | n = pool size |
| `to_json()` | O(1) | With lock acquisition |

## Exception Safety

The resource_pool provides strong exception safety guarantees:
- **checkout()**: If factory callback throws, the checkout count is properly decremented
- **checkin()**: No exceptions thrown (noexcept)
- **clear()**: No exceptions thrown (noexcept)
- **size()**: No exceptions thrown (noexcept)

## Best Practices

1. **Always use RAII**: Let `scoped_resource` handle resource return
2. **Pre-populate pools**: Add resources before concurrent access
3. **Handle exceptions**: Catch `std::runtime_error` from `checkout()`
4. **Keep factories simple**: Factory callbacks should only create resources
5. **Monitor utilization**: Use `to_json()` to track pool health
6. **Use appropriate types**: Prefer `shared_ptr` or `unique_ptr` over raw pointers
7. **Test concurrency**: Verify thread safety with your specific use case

## Constraints

- Capacity limited to 255 resources (uint8_t)
- Factory callbacks must not call pool methods (would cause deadlock)
- Resources must be move-constructible and non-arithmetic types
- Counters wrap around after ~18 quintillion operations (uint64_t)

## Building and Testing

### Build with CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Run Tests

```bash
ctest --output-on-failure
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
