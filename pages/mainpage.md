@mainpage ARRP - Auto Returning Resource Pool for Modern C++

@section intro Introduction

The **arrp** (Auto Returning Resource Pool) library provides a thread-safe, modern C++20 resource pool implementation with automatic lifecycle management using RAII principles. It leverages standard library features like `std::deque`, `std::mutex`, and `std::concepts` to provide a clean, efficient abstraction for managing pools of reusable resources.

This header-only library eliminates boilerplate resource management code and provides a simple, type-safe API for resource pooling scenarios.

@section features Key Features

- **Resource Pool**: Thread-safe management of reusable resources with automatic return
- **RAII Pattern**: Automatic resource return via scoped_resource wrapper
- **Capacity Management**: Enforces maximum capacity limits to prevent unbounded growth
- **FIFO Ordering**: Resources retrieved from front, added to back
- **Customizable Factory**: Support for custom resource creation callbacks
- **Diagnostic Counters**: Track borrow, return, and auto-add operations
- **Modern C++20**: Uses only standard library features (no external dependencies)
- **Type-Safe**: Leverages C++20 concepts for compile-time type checking
- **Thread-Safe**: All operations protected by mutexes for concurrent access
- **Move Semantics**: Efficient resource transfer with perfect forwarding

@section requirements Requirements

- **C++20 Support**: Requires `std::deque`, `std::mutex`, and `std::concepts`
- **Compiler Support**:
  - GCC 10+
  - MSVC 16.11+ (Visual Studio 2019 or later)
  - Clang 10+ (with `-fexperimental-library` flag)
- **Platform Support**: Windows, Linux, macOS
- **Optional**: nlohmann/json for JSON serialization support

@section components Main Components

| Component | Description | Use Case |
|-----------|-------------|----------|
| @ref siddiqsoft::arrp::resource_pool | Resource pool manager | Connection/resource management |
| @ref siddiqsoft::arrp::scoped_resource | RAII resource wrapper | Automatic resource return |

@section design Design Principles

- **Move Semantics**: All components use move semantics for efficient resource transfer
- **RAII**: Proper resource management through constructors and destructors
- **Thread Safety**: Internal synchronization using mutexes and scoped locks
- **Zero-Copy**: Minimal data copying through perfect forwarding
- **Type Safety**: C++20 concepts ensure compile-time type checking
- **Simplicity**: Clean API that hides complexity of resource management

@section documentation Documentation

- @ref getting_started - Installation and setup guide
- @ref usage_guide - Detailed usage examples and best practices
- @ref examples - Real-world code examples
- @ref quick_reference - Quick lookup guide for common tasks
- @ref api - Complete API reference

@section quickstart Quick Start

@subsection resource_pool_basic Basic Resource Pool Example

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <iostream>
#include <memory>

class DatabaseConnection {
public:
    void query(const std::string& sql) { 
        std::cout << "Executing: " << sql << std::endl;
    }
};

int main() {
    // Create a resource pool
    siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;

    // Populate the pool
    for (int i = 0; i < 5; ++i) {
        pool.return_to_pool(std::make_shared<DatabaseConnection>());
    }

    // Borrow and use a resource
    {
        auto conn = pool.borrow_from_pool();
        conn->query("SELECT * FROM users");
        // Automatically returned to pool when going out of scope
    }

    return 0;
}
```

@subsection resource_pool_custom Custom Factory Example

```cpp
#include "siddiqsoft/resource_pool.hpp"

int main() {
    siddiqsoft::arrp::resource_pool<DatabaseConnection> pool{
        [](auto& p) -> siddiqsoft::arrp::scoped_resource<DatabaseConnection> {
            return siddiqsoft::arrp::scoped_resource<DatabaseConnection>(
                DatabaseConnection::create(),
                [&p](DatabaseConnection&& conn) { 
                    p.return_to_pool(std::move(conn)); 
                }
            );
        }
    };

    // Resources are created on-demand up to capacity
    for (int i = 0; i < 10; ++i) {
        auto conn = pool.borrow_from_pool();
        conn.query("SELECT * FROM users");
        // Automatically returned when going out of scope
    }

    return 0;
}
```

@subsection resource_pool_multithreaded Multi-threaded Usage Example

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <thread>
#include <vector>

int main() {
    siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;

    // Populate pool
    for (int i = 0; i < 10; ++i) {
        pool.return_to_pool(std::make_shared<DatabaseConnection>());
    }

    // Use from multiple threads
    std::vector<std::thread> threads;
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&pool]() {
            for (int i = 0; i < 20; ++i) {
                auto conn = pool.borrow_from_pool();
                conn->query("SELECT * FROM users");
                // Automatically returned
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

@section installation Installation

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

@section license License

BSD 3-Clause License - See LICENSE file for details

@section copyright Copyright

Copyright (c) 2026, Abdulkareem Siddiq. All rights reserved.

@section links Links

- **GitHub**: https://github.com/SiddiqSoft/arrp
- **NuGet**: https://www.nuget.org/packages/SiddiqSoft.aarp/
- **Documentation**: https://siddiqsoft.github.io/arrp/

@section see_also See Also

- @ref getting_started
- @ref usage_guide
- @ref examples
- @ref quick_reference
- @ref api
