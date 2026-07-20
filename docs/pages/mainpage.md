@mainpage ARRP - Auto Returning Resource Pool for Modern C++

@section intro Introduction

The **arrp** (Auto Returning Resource Pool) library provides a thread-safe, modern C++20 resource pool implementation with automatic lifecycle management using RAII principles.

This header-only library eliminates boilerplate resource management code and provides a simple, type-safe API for resource pooling scenarios. Perfect for managing expensive-to-create resources like database connections, thread pools, or network sockets.

@section features Key Features

- **Resource Pool**: Thread-safe management of reusable resources with automatic return
- **RAII Pattern**: Automatic resource return via scoped_resource wrapper
- **Capacity Management**: Enforces maximum capacity limits to prevent unbounded growth
- **FIFO Ordering**: Resources retrieved from front, added to back
- **Customizable Factory**: Support for custom resource creation callbacks
- **Diagnostic Counters**: Track borrow, return, and auto-add operations
- **Exception Safe**: Strong exception guarantees with automatic cleanup
- **Modern C++20**: Uses only standard library features
- **Type-Safe**: Leverages C++20 concepts for compile-time type checking
- **Thread-Safe**: All operations protected by mutexes for concurrent access
- **Move Semantics**: Efficient resource transfer with perfect forwarding
- **JSON Diagnostics**: Optional JSON serialization for pool state monitoring

@section requirements Requirements

- **C++20 Support**: Requires std::deque, std::mutex, and std::concepts
- **Compiler Support**: GCC 10+, MSVC 16.11+, Clang 10+
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
- **Performance**: O(1) checkout/checkin operations with minimal contention

@section documentation Documentation

- @ref getting_started - Installation and setup guide
- @ref usage_guide - Detailed usage examples and best practices
- @ref examples - Real-world code examples
- @ref quick_reference - Quick lookup guide for common tasks
- @ref api - Complete API reference
- @ref security - Security considerations and best practices

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
- @ref security
