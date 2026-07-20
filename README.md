<p align=right>

&#xFDFD;

</p>

# arrp: Auto Returning Resource Pool for Modern C++

## Objective

Design a resource pool with ease of use and safety guarantees using RAII principles to ensure:
- Stored object lifetime management
- Stored object restored to the pool
- Strict scoped checkout and checkin semantics

## Quick Start

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <memory>

class DatabaseConnection {
public:
    void execute(const std::string& query) { /* ... */ }
};

int main() {
    // Create a resource pool
    siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;

    // Populate the pool
    for (int i = 0; i < 5; ++i) {
        pool.checkin(std::make_shared<DatabaseConnection>());
    }

    // Borrow and use a resource
    {
        auto conn = pool.checkout();
        conn->execute("SELECT * FROM users");
        // Automatically returned to pool when going out of scope
    }

    return 0;
}
```

## Features

- **Thread-Safe**: All operations are protected by mutexes for concurrent access
- **RAII Pattern**: Automatic resource return via scoped_resource wrapper
- **Capacity Management**: Enforces maximum capacity limits to prevent unbounded growth
- **FIFO Ordering**: Resources retrieved from front, added to back
- **Customizable Factory**: Support for custom resource creation callbacks
- **Diagnostic Counters**: Track borrow, return, and auto-add operations
- **Modern C++20**: Uses only standard library features (no external dependencies)
- **Type-Safe**: Leverages C++20 concepts for compile-time type checking
- **Move Semantics**: Efficient resource transfer with perfect forwarding

## Requirements

- **Language**: C++20 minimum required for `std::jthread`, `std::mutex`, `std::recursive_mutex`, `std::scoped_lock`, `std::unique_lock`
- **Optional**: `nlohmann::json` for JSON serialization support
- **No External Dependencies**: Only uses standard library features
- **Platform Support**: MacOS, Linux, Windows
  - MacOS: Development platform
  - Linux (RedHat): CI pipeline tested
  - Windows: CI pipeline tested

### Compiler Support

- **GCC**: 10+ (Linux)
- **Clang**: 
  - AppleClang v21+
  - LLVM v20+
- **MSVC**: 
  - Visual Studio 2019
  - Visual Studio 2022
  - Visual Studio 2026

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

## Documentation

For detailed documentation, see:
- [Getting Started](docs/pages/getting_started.md) - Installation and setup guide
- [Usage Guide](docs/pages/usage_guide.md) - Detailed usage examples and best practices
- [API Reference](docs/pages/api.md) - Complete API documentation
- [Examples](docs/pages/examples.md) - Real-world code examples
- [Quick Reference](docs/pages/quick_reference.md) - Quick lookup guide
- [Security](docs/pages/security.md) - Security considerations

## Key Components

| Component | Description | Use Case |
|-----------|-------------|----------|
| `resource_pool<T>` | Thread-safe resource pool manager | Connection/resource management |
| `scoped_resource<T>` | RAII resource wrapper | Automatic resource return |

## Design Principles

- **Move Semantics**: All components use move semantics for efficient resource transfer
- **RAII**: Proper resource management through constructors and destructors
- **Thread Safety**: Internal synchronization using mutexes and scoped locks
- **Zero-Copy**: Minimal data copying through perfect forwarding
- **Type Safety**: C++20 concepts ensure compile-time type checking
- **Simplicity**: Clean API that hides complexity of resource management

## License

BSD 3-Clause License - See LICENSE file for details

## Copyright

Copyright (c) 2026, Abdulkareem Siddiq. All rights reserved.

## Links

- **GitHub**: https://github.com/SiddiqSoft/arrp
- **NuGet**: https://www.nuget.org/packages/SiddiqSoft.aarp/
- **Documentation**: https://siddiqsoft.github.io/arrp/
