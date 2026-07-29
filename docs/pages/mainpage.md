@mainpage ARRP - Auto Returning Resource Pool for Modern C++

@section intro Introduction

The **arrp** (Auto Returning Resource Pool) library provides a thread-safe, modern C++20 resource pool implementation with automatic lifecycle management using RAII principles.

This header-only library eliminates boilerplate resource management code and provides a simple, type-safe API for resource pooling scenarios. Perfect for managing expensive-to-create resources like database connections, thread pools, or network sockets.

The library uses `std::expected<T, pool_error>` for error handling, providing a modern, exception-free approach to resource management.

@section features Key Features

- **Thread-Safe**: All operations protected by mutexes for concurrent access
- **RAII Pattern**: Automatic resource return via scoped_resource wrapper
- **Capacity Management**: Enforces maximum capacity limits
- **FIFO Ordering**: Predictable resource ordering
- **Customizable Factory**: Support for custom resource creation callbacks
- **Error Handling**: Uses `std::expected` for safe error handling without exceptions
- **Modern C++20**: Uses only standard library features
- **Type-Safe**: Leverages C++20 concepts for compile-time type checking
- **Move Semantics**: Efficient resource transfer with perfect forwarding
- **JSON Diagnostics**: Optional JSON serialization for pool state monitoring

@section documentation Documentation

- @ref api - Complete API reference
- @ref usage_guide - Detailed usage examples and best practices

@section quick_example Quick Example

```cpp
#include "siddiqsoft/resource_pool.hpp"

// Create a pool with capacity and auto-grow policy
siddiqsoft::arrp::resource_pool<MyResource> pool(
    10,  // capacity
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);

// Add resources
pool.seed_to_pool(MyResource());

// Borrow and use
{
    auto resource = pool.borrow_from_pool();
    if (resource) {
        resource->doSomething();
        // Automatically returned to pool when going out of scope
    }
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

@section key_concepts Key Concepts

### Resource Borrowing

Resources are borrowed from the pool using `borrow_from_pool()`, which returns a `std::expected<scoped_resource<T>, pool_error>`. The scoped_resource automatically returns the resource to the pool when destroyed.

### Capacity Management

The pool has a fixed capacity (max 255 resources). When the pool is under capacity and a resource is requested, the factory callback can create new resources on-demand (if AutoGrow policy is enabled).

### Error Handling

Instead of throwing exceptions, the pool uses `std::expected<T, pool_error>` to return errors. This allows for explicit error handling without exception overhead.

### Thread Safety

All public methods are thread-safe. Multiple threads can safely borrow and return resources concurrently without external synchronization.

@section license License

BSD 3-Clause License - See LICENSE file for details

@section copyright Copyright

Copyright (c) 2026, Abdulkareem Siddiq. All rights reserved.

@section links Links

- **GitHub**: https://github.com/SiddiqSoft/arrp
- **NuGet**: https://www.nuget.org/packages/SiddiqSoft.aarp/
- **Documentation**: https://siddiqsoft.github.io/arrp/

@section see_also See Also

- @ref api - Complete API reference
- @ref usage_guide - Usage guide and examples
