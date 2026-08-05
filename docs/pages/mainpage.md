/**
@mainpage arrp - Auto Returning Resource Pool

@section overview Overview

**arrp** is a modern C++ library providing a thread-safe, auto-returning resource pool with RAII semantics.
Resources are automatically returned to the pool when scoped wrappers are destroyed, eliminating manual
resource management and preventing leaks.

@section features Key Features

- **Thread-Safe**: All operations protected by mutex (std::mutex or std::recursive_mutex)
- **RAII Pattern**: Resources automatically returned on destruction
- **Move-Only Semantics**: Prevents resource ownership ambiguity
- **Flexible Policies**: Support for fixed-size pools and factory callbacks
- **Callback-Based**: Factory and cleanup callbacks for custom resource management
- **Statistics**: Built-in counters for monitoring pool usage
- **JSON Serialization**: Export pool statistics to JSON (with nlohmann/json)

@section quick_start Quick Start

@subsection basic_usage Basic Usage

```cpp
#include "siddiqsoft/resource_pool.hpp"

siddiqsoft::arrp::resource_pool<MyResource> pool(10);

auto resource = pool.try_borrow();
if (resource) {
    resource->doSomething();
}
// Resource automatically returned when scoped_resource is destroyed
```

@subsection custom_factory Custom Factory

```cpp
siddiqsoft::arrp::resource_pool<MyResource> pool(10);
pool.set_factory_callback([] {
    return MyResource();
});

auto resource = pool.try_borrow_create();
```

@section architecture Architecture

The library consists of three main components:

1. **resource_pool<T>**: Thread-safe pool manager
   - Manages resource lifecycle
   - Handles borrowing and returning
   - Tracks statistics

2. **scoped_resource<T>**: RAII wrapper
   - Wraps individual resources
   - Automatically returns on destruction
   - Supports move semantics

3. **Common Types**: Enums and limits
   - pool_error: Error codes
   - resource_pool_limits: Capacity constraints

@section thread_safety Thread Safety

- **resource_pool**: Fully thread-safe (mutex-protected)
- **scoped_resource**: NOT thread-safe (single-threaded access)
- Each scoped_resource should be accessed by only one thread

@section error_handling Error Handling

Borrowed resources are represented by `scoped_resource<T>` wrappers. Check validity before use and inspect `error()` when invalid:

```cpp
auto resource = pool.try_borrow();
if (resource && resource.is_valid()) {
    // Use resource
} else {
    switch (resource.error()) {
        case siddiqsoft::arrp::pool_error::NoMoreResources:
            // Pool exhausted
            break;
        case siddiqsoft::arrp::pool_error::ShutdownInitiated:
            // Pool is shutting down
            break;
        default:
            break;
    }
}
```

@section statistics Statistics

Access pool statistics via JSON:

```cpp
auto stats = pool.to_json();
if (stats) {
    std::cout << stats.value().get().dump(2) << std::endl;
}
```

Statistics include:
- capacity: Maximum resources
- size: Available resources
- deficit: Resources needed to reach capacity
- peaksize: Peak pool size reached
- borrows: Total resources borrowed
- returns: Total resources returned
- loans: Currently borrowed resources
- abandons: Invalidated resources

@section concepts Concepts

@subsection non_numeric_move_constructible NonNumericMoveConstructible

A type T that satisfies:
- std::move_constructible<T>
- std::move_assignable<T>
- !std::is_arithmetic<T>

This prevents wrapping primitive types which would be inefficient.

@section examples Examples

@subsection example_file_pool File Handle Pool

```cpp
class FileHandle {
public:
    FileHandle(const std::string& path) : m_file(std::fopen(path.c_str(), "r")) {}
    ~FileHandle() { if (m_file) std::fclose(m_file); }
    FILE* get() { return m_file; }
private:
    FILE* m_file;
};

siddiqsoft::arrp::resource_pool<FileHandle> file_pool(5);
file_pool.set_factory_callback([]() {
    return FileHandle("data.txt");
});

auto file = file_pool.try_borrow_create();
if (file) {
    // Use file
}
// File automatically returned to pool
```

@section namespace Namespace

All types are in `siddiqsoft::arrp` namespace.

@see resource_pool
@see scoped_resource
@see pool_error
*/
