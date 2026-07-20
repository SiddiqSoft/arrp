@page usage_guide Usage Guide

@section overview Overview

This guide covers the resource_pool component and how to use it effectively for managing reusable resources.

@section resource_pool Resource Pool

The `resource_pool` is a thread-safe container for managing a pool of reusable resources with automatic lifecycle management using RAII principles.

@subsection rp_basic Basic Usage

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

    // Populate the pool with resources
    for (int i = 0; i < 10; ++i) {
        pool.checkin(std::make_shared<DatabaseConnection>());
    }

    // Borrow a resource
    {
        auto conn = pool.checkout();
        conn->execute("SELECT * FROM users");
        // Automatically returned when going out of scope
    }

    return 0;
}
```

@subsection rp_capacity Capacity Management

The resource pool enforces a maximum capacity limit:

```cpp
// Create a pool with default capacity (8 resources)
siddiqsoft::arrp::resource_pool<Resource> pool1;

// Create a pool with custom capacity (16 resources)
siddiqsoft::arrp::resource_pool<Resource, siddiqsoft::arrp::scoped_resource<Resource>, 16> pool2;

// Maximum capacity is 128
// siddiqsoft::arrp::resource_pool<Resource, siddiqsoft::arrp::scoped_resource<Resource>, 256> pool3;  // Error!
```

@subsection rp_factory Custom Resource Factory

Provide a custom factory callback to create resources on-demand:

```cpp
siddiqsoft::arrp::resource_pool<DatabaseConnection> pool{
    [](auto& p) -> siddiqsoft::arrp::scoped_resource<DatabaseConnection> {
        // Create a new resource
        auto conn = DatabaseConnection::create("localhost", 5432);
        
        // Wrap it with a callback to return it to the pool
        return siddiqsoft::arrp::scoped_resource<DatabaseConnection>(
            std::move(conn),
            [&p](DatabaseConnection&& conn) { 
                p.checkin(std::move(conn)); 
            }
        );
    }
};

// Resources are created on-demand up to capacity
for (int i = 0; i < 10; ++i) {
    auto conn = pool.checkout();
    conn.execute("SELECT * FROM users");
    // Automatically returned when going out of scope
}
```

@subsection rp_methods Resource Pool Methods

#### checkout()

Borrow a resource from the pool:

```cpp
auto resource = pool.checkout();  // Returns scoped_resource<T>
// Use the resource...
// Automatically returned when going out of scope
```

Throws `std::runtime_error` if the pool is empty and at capacity.

#### checkin()

Return a resource to the pool:

```cpp
auto resource = pool.checkout();
// Use the resource...
pool.checkin(std::move(*resource));  // Manual return (rarely needed)
```

This is typically called automatically by the scoped_resource destructor.

#### size()

Get the current number of available resources:

```cpp
auto available = pool.size();
std::cout << "Available resources: " << available << std::endl;
```

#### clear()

Remove all resources from the pool:

```cpp
pool.clear();  // All pooled resources are destroyed
```

#### to_json()

Get pool state as JSON (requires nlohmann/json):

```cpp
#include <nlohmann/json.hpp>

auto state = pool.to_json();
std::cout << state.dump(2) << std::endl;

// Output:
// {
//   "_typver": "siddiqsoft.arrp.resource_pool/0.0.0",
//   "capacity": 10,
//   "size": 5,
//   "load": 8,
//   "invalidated": 0,
//   "checkedout": 3,
//   "counters": {
//     "autoreturns": 3,
//     "newitems": 3,
//     "return": 5,
//     "borrow": 8
//   }
// }
```

@subsection rp_scoped_resource Scoped Resource Wrapper

The `scoped_resource<T>` wrapper implements RAII for automatic resource return:

```cpp
// Dereference operator
auto resource = pool.checkout();
(*resource).execute("SELECT * FROM users");


// Move semantics
auto resource1 = pool.checkout();
auto resource2 = std::move(resource1);  // resource1 is now invalid

// Invalidation
auto resource = pool.checkout();
auto ptr = std::move(*resource);
resource.invalidate();  // Don't return the moved-out resource
```

@subsection rp_multithreaded Multi-threaded Usage

The resource pool is thread-safe for concurrent access:

```cpp
#include <thread>
#include <vector>

siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;

// Populate pool
for (int i = 0; i < 10; ++i) {
    pool.checkin(std::make_shared<DatabaseConnection>());
}

// Use from multiple threads
std::vector<std::thread> threads;
for (int t = 0; t < 5; ++t) {
    threads.emplace_back([&pool]() {
        for (int i = 0; i < 20; ++i) {
            auto conn = pool.checkout();
            conn->execute("SELECT * FROM users");
            // Automatically returned
        }
    });
}

for (auto& t : threads) {
    t.join();
}
```

@section best_practices Best Practices

@subsection bp_lifetime Lifetime Management

- Keep the pool alive as long as you're borrowing resources
- Destruction waits for all checked-out resources to be returned
- Use RAII to ensure proper cleanup

```cpp
{
    siddiqsoft::arrp::resource_pool<Resource> pool;
    
    // Populate and use pool
    auto res = pool.checkout();
    // Use resource...
}  // Pool destroyed here; all resources cleaned up
```

@subsection bp_resource_types Resource Types

- Use `std::shared_ptr<T>` for shared ownership
- Use `std::unique_ptr<T>` for exclusive ownership
- Use custom classes for complex resources
- Avoid arithmetic types (int, float, double, bool)

```cpp
// Good: Shared pointer
siddiqsoft::arrp::resource_pool<std::shared_ptr<Connection>> pool1;

// Good: Unique pointer
siddiqsoft::arrp::resource_pool<std::unique_ptr<Connection>> pool2;

// Good: Custom class
siddiqsoft::arrp::resource_pool<DatabaseConnection> pool3;

// Bad: Arithmetic type (compilation error)
// siddiqsoft::arrp::resource_pool<int> pool4;  // Error!
```

@subsection bp_capacity Capacity Planning

- Set capacity to match your thread pool size
- Consider peak concurrent usage
- Balance memory usage with availability

```cpp
// For 4 concurrent threads, use capacity of 4-8
siddiqsoft::arrp::resource_pool<Resource, siddiqsoft::arrp::scoped_resource<Resource>, 8> pool;

// Populate with initial resources
for (int i = 0; i < 8; ++i) {
    pool.checkin(Resource{});
}
```

@subsection bp_error_handling Error Handling

- Handle `std::runtime_error` from `checkout()`
- Implement proper exception handling in callbacks
- Monitor pool state for bottlenecks

```cpp
try {
    auto resource = pool.checkout();
    // Use resource...
} catch (const std::runtime_error& e) {
    std::cerr << "Failed to borrow resource: " << e.what() << std::endl;
    // Handle error (retry, use fallback, etc.)
}
```

@subsection bp_monitoring Monitoring

- Use `size()` to check available resources
- Use `to_json()` to get detailed diagnostics
- Monitor for pool exhaustion

```cpp
auto available = pool.size();
if (available < THRESHOLD) {
    std::cerr << "Warning: Pool running low on resources" << std::endl;
}

// Get detailed state
auto state = pool.to_json();
std::cout << "Pool state: " << state.dump(2) << std::endl;
```

@section advanced Advanced Topics

@subsection adv_custom_types Custom Resource Types

Your resource type must be move-constructible and non-arithmetic:

```cpp
class MyResource {
public:
    // Must be move-constructible
    MyResource(MyResource&&) = default;
    MyResource& operator=(MyResource&&) = default;

    // Copy is optional
    MyResource(const MyResource&) = delete;
    MyResource& operator=(const MyResource&) = delete;

    void doSomething() { /* ... */ }
};

siddiqsoft::arrp::resource_pool<MyResource> pool;
```

@subsection adv_json_serialization JSON Serialization

Get pool diagnostics as JSON (requires nlohmann/json):

```cpp
#include <nlohmann/json.hpp>

siddiqsoft::arrp::resource_pool<Resource> pool;

// Get JSON representation
auto json = pool.to_json();
std::cout << json.dump(2) << std::endl;

// JSON fields:
// - _typver: Version identifier
// - capacity: Maximum capacity
// - size: Current pool size
// - load: Total resources (in pool + checked out)
// - invalidated: Number of invalidated resources
// - checkedout: Number of checked-out resources
// - counters: Operation counters (borrow, return, autoreturns, newitems)
```

@subsection adv_resource_invalidation Resource Invalidation

Prevent a resource from being returned to the pool:

```cpp
auto resource = pool.checkout();

// Move the resource out
auto ptr = std::move(*resource);

// Invalidate to prevent automatic return
resource.invalidate();

// Resource is NOT returned to pool when going out of scope
```

@section performance Performance Considerations

### resource_pool

- Ideal capacity should match `std::thread::hardware_concurrency()`
- Each borrow/return operation acquires a lock
- Resources are stored in a deque for efficient FIFO access
- Minimal overhead from the `scoped_resource` wrapper
- Use for expensive resources (connections, file handles, buffers)

### Optimization Tips

- Pre-populate the pool with resources
- Use appropriate capacity for your workload
- Monitor pool state with `to_json()`
- Avoid frequent pool exhaustion
- Use `std::shared_ptr` for shared resources
