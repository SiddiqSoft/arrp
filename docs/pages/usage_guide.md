@page usage_guide Usage Guide

@section ug_overview Overview

This guide covers practical usage patterns for the ARRP resource pool library.

@section ug_basic_usage Basic Usage

@subsection ug_create_pool Creating a Pool

```cpp
#include "siddiqsoft/resource_pool.hpp"

// Default pool (NoGrow policy - throws when empty)
siddiqsoft::arrp::resource_pool<std::string> pool;

// AutoGrow pool (creates resources on demand)
siddiqsoft::arrp::resource_pool<std::string> auto_pool(
    siddiqsoft::arrp::resource_pool<std::string>::auto_add_policy::AutoGrow
);
```

@subsection ug_populate_pool Populating the Pool

```cpp
// Add single resource
pool.checkin(std::make_shared<DatabaseConnection>());

// Add multiple resources
for (int i = 0; i < 10; ++i) {
    pool.checkin(std::make_shared<DatabaseConnection>());
}
```

@subsection ug_borrow_resource Borrowing Resources

```cpp
try {
    auto resource = pool.checkout();
    // Use the resource
    resource->query("SELECT * FROM users");
    // Automatically returned when going out of scope
} catch (const std::runtime_error& e) {
    std::cerr << "Failed to borrow resource: " << e.what() << std::endl;
}
```

@section ug_advanced_patterns Advanced Patterns

@subsection ug_custom_factory Custom Factory Callback

```cpp
siddiqsoft::arrp::resource_pool<DatabaseConnection> pool(
    [](auto& p) -> siddiqsoft::arrp::scoped_resource<DatabaseConnection> {
        // Create new resource
        auto conn = DatabaseConnection::create();
        
        // Return wrapped with auto-return callback
        return siddiqsoft::arrp::scoped_resource<DatabaseConnection>(
            std::move(conn),
            [&p](DatabaseConnection&& res) { 
                p.checkin(std::move(res)); 
            }
        );
    }
);

// Resources are created on-demand up to capacity
for (int i = 0; i < 100; ++i) {
    auto conn = pool.checkout();
    conn->query("SELECT * FROM users");
}
```

@subsection ug_multithreaded Multi-threaded Usage

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
                // Automatically returned
            } catch (const std::runtime_error&) {
                // Pool exhausted - handle gracefully
            }
        }
    });
}
// jthread joins automatically
```

@subsection ug_monitoring Monitoring Pool State

```cpp
// Get pool statistics
auto state = pool.to_json();

std::cout << "Capacity: " << state["capacity"] << std::endl;
std::cout << "Available: " << state["size"] << std::endl;
std::cout << "Checked out: " << state["loan"] << std::endl;
std::cout << "Total load: " << state["load"] << std::endl;

// Access operation counters
auto counters = state["counters"];
std::cout << "Total borrows: " << counters["checkout"] << std::endl;
std::cout << "Total returns: " << counters["checkin"] << std::endl;

```

@section ug_error_handling Error Handling

@subsection ug_pool_exhaustion Handling Pool Exhaustion

```cpp
// Strategy 1: Retry with backoff
auto borrow_with_retry = [&pool](int max_retries = 3) {
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        try {
            return pool.checkout();
        } catch (const std::runtime_error&) {
            if (attempt < max_retries - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10 * (attempt + 1)));
            } else {
                throw;
            }
        }
    }
};

// Strategy 2: Use AutoGrow policy
siddiqsoft::arrp::resource_pool<Resource> auto_pool(
    siddiqsoft::arrp::resource_pool<Resource>::auto_add_policy::AutoGrow
);
```

@subsection ug_exception_safety Exception Safety

```cpp
// Resources are automatically returned even if exception occurs
try {
    auto resource = pool.checkout();
    throw std::runtime_error("Something went wrong");
} catch (const std::exception&) {
    // Resource is still returned to pool
}

// Verify resource was returned
EXPECT_EQ(1u, pool.size());
```

@section ug_resource_types Resource Types

@subsection ug_shared_ptr Using shared_ptr

```cpp
siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;

pool.checkin(std::make_shared<DatabaseConnection>());

{
    auto conn = pool.checkout();
    conn->query("SELECT * FROM users");
}
```

@subsection ug_unique_ptr Using unique_ptr

```cpp
siddiqsoft::arrp::resource_pool<std::unique_ptr<DatabaseConnection>> pool;

pool.checkin(std::make_unique<DatabaseConnection>());

{
    auto conn = pool.checkout();
    conn->query("SELECT * FROM users");
}
```

@subsection ug_custom_types Using Custom Types

```cpp
class MyResource {
public:
    void doWork() { /* ... */ }
};

siddiqsoft::arrp::resource_pool<MyResource> pool;

pool.checkin(MyResource());

{
    auto res = pool.checkout();
    res->doWork();
}
```

@section ug_invalidation Resource Invalidation

@subsection ug_invalidate_moved Invalidating Moved Resources

```cpp
{
    auto resource = pool.checkout();
    
    // Move resource out
    auto extracted = std::move(*resource);
    
    // Mark as invalid so it's not returned to pool
    resource.invalidate();
    
    // Use extracted resource
    extracted.doWork();
}
// Resource is NOT returned to pool
```

@subsection ug_invalidate_consumed Invalidating Consumed Resources

```cpp
{
    auto resource = pool.checkout();
    
    // Consume the resource
    process_and_consume(*resource);
    
    // Mark as invalid
    resource.invalidate();
}
// Resource is NOT returned to pool
```

@section ug_troubleshooting Troubleshooting

@subsection ug_deadlock Avoiding Deadlocks

**Problem**: Application hangs when calling pool methods.

**Cause**: Factory callback calls pool methods.

**Solution**:
```cpp
// WRONG - Will deadlock
auto pool = resource_pool<Resource>(
    [](auto& p) -> scoped_resource<Resource> {
        auto res = p.checkout();  // DEADLOCK!
        return scoped_resource<Resource>(res, ...);
    }
);

// CORRECT - Only create and return
auto pool = resource_pool<Resource>(
    [](auto& p) -> scoped_resource<Resource> {
        return scoped_resource<Resource>(
            Resource::create(),
            [&p](Resource&& res) { p.checkin(std::move(res)); }
        );
    }
);
```

@subsection ug_pool_empty Pool is Empty

**Problem**: Getting "No items in the pool" exception.

**Solutions**:
1. Pre-populate pool before use
2. Use AutoGrow policy
3. Implement retry logic with backoff

@subsection ug_performance Performance Issues

**Problem**: Pool operations are slow.

**Solutions**:
1. Increase pool size to reduce contention
2. Ensure factory callbacks are fast
3. Monitor with to_json() to check utilization
4. Use appropriate resource types (shared_ptr vs unique_ptr)

@section ug_best_practices Best Practices

1. **Always use RAII**: Let scoped_resource handle resource return
2. **Pre-populate pools**: Add resources before concurrent access
3. **Handle exceptions**: Catch std::runtime_error from checkout()
4. **Keep factories simple**: Factory callbacks should only create resources
5. **Monitor utilization**: Use to_json() to track pool health
6. **Use appropriate types**: Prefer shared_ptr or unique_ptr
7. **Test concurrency**: Verify thread safety with your specific use case
8. **Avoid manual checkin()**: Only use in advanced scenarios
9. **Document assumptions**: Clearly document resource lifecycle expectations
10. **Profile under load**: Test with realistic concurrent access patterns
