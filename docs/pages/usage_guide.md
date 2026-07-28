@page usage_guide Usage Guide

@section ug_overview Overview

This guide covers practical usage patterns for the ARRP resource pool library.

@section ug_basic_usage Basic Usage

@subsection ug_create_pool Creating a Pool

```cpp
#include "siddiqsoft/resource_pool.hpp"

// Default pool (NoGrow policy - returns error when empty)
siddiqsoft::arrp::resource_pool<std::string> pool(
    10,  // capacity
    siddiqsoft::arrp::auto_add_policy::NoGrow
);

// AutoGrow pool (creates resources on demand)
siddiqsoft::arrp::resource_pool<std::string> auto_pool(
    10,  // capacity
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);
```

@subsection ug_populate_pool Populating the Pool

```cpp
// Add single resource
auto result = pool.add_to_pool(std::make_shared<DatabaseConnection>());
if (!result) {
    std::cerr << "Failed to add resource" << std::endl;
}

// Add multiple resources
for (int i = 0; i < 10; ++i) {
    pool.add_to_pool(std::make_shared<DatabaseConnection>());
}
```

@subsection ug_borrow_resource Borrowing Resources

```cpp
auto resource = pool.borrow_from_pool();
if (resource) {
    // Use the resource
    resource->query("SELECT * FROM users");
    // Automatically returned when going out of scope
} else {
    std::cerr << "Failed to borrow resource" << std::endl;
}
```

@section ug_advanced_patterns Advanced Patterns

@subsection ug_custom_factory Custom Factory Callback

```cpp
siddiqsoft::arrp::resource_pool<DatabaseConnection> pool(
    10,  // capacity
    [](auto& p) -> std::expected<siddiqsoft::arrp::scoped_resource<DatabaseConnection>, siddiqsoft::arrp::pool_error> {
        // Create new resource
        auto conn = DatabaseConnection::create();
        
        // Return wrapped with auto-return callback
        return siddiqsoft::arrp::scoped_resource<DatabaseConnection>(
            [&p](DatabaseConnection&& res, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                if (isvalid) {
                    return p.add_to_pool(std::move(res));
                }
                return {};
            },
            std::move(conn)
        );
    }
);

// Resources are created on-demand up to capacity
for (int i = 0; i < 100; ++i) {
    auto conn = pool.borrow_from_pool();
    if (conn) {
        conn->query("SELECT * FROM users");
    }
}
```

@subsection ug_multithreaded Multi-threaded Usage

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

@subsection ug_monitoring Monitoring Pool State

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
    std::cout << "On-demand adds: " << json["autoadds"] << std::endl;
}
```

@section ug_error_handling Error Handling

@subsection ug_pool_exhaustion Handling Pool Exhaustion

```cpp
// Strategy 1: Retry with backoff
auto borrow_with_retry = [&pool](int max_retries = 3) -> std::expected<siddiqsoft::arrp::scoped_resource<Resource>, siddiqsoft::arrp::pool_error> {
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        auto result = pool.borrow_from_pool();
        if (result) {
            return result;
        }
        
        if (attempt < max_retries - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10 * (attempt + 1)));
        }
    }
    return std::unexpected(siddiqsoft::arrp::pool_error::NoMoreResources);
};

// Strategy 2: Use AutoGrow policy
siddiqsoft::arrp::resource_pool<Resource> auto_pool(
    10,
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);
```

@subsection ug_exception_safety Exception Safety

```cpp
// Resources are automatically returned even if exception occurs
try {
    auto resource = pool.borrow_from_pool();
    if (resource) {
        throw std::runtime_error("Something went wrong");
    }
} catch (const std::exception&) {
    // Resource is still returned to pool
}

// Verify resource was returned
auto size_result = pool.size();
if (size_result) {
    EXPECT_EQ(1u, size_result.value());
}
```

@section ug_resource_types Resource Types

@subsection ug_shared_ptr Using shared_ptr

```cpp
siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool(
    10,
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);

pool.add_to_pool(std::make_shared<DatabaseConnection>());

{
    auto conn = pool.borrow_from_pool();
    if (conn) {
        conn->query("SELECT * FROM users");
    }
}
```

@subsection ug_unique_ptr Using unique_ptr

```cpp
siddiqsoft::arrp::resource_pool<std::unique_ptr<DatabaseConnection>> pool(
    10,
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);

pool.add_to_pool(std::make_unique<DatabaseConnection>());

{
    auto conn = pool.borrow_from_pool();
    if (conn) {
        conn->query("SELECT * FROM users");
    }
}
```

@subsection ug_custom_types Using Custom Types

```cpp
class MyResource {
public:
    void doWork() { /* ... */ }
};

siddiqsoft::arrp::resource_pool<MyResource> pool(
    10,
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);

pool.add_to_pool(MyResource());

{
    auto res = pool.borrow_from_pool();
    if (res) {
        res->doWork();
    }
}
```

@section ug_invalidation Resource Invalidation

@subsection ug_invalidate_moved Invalidating Moved Resources

```cpp
{
    auto resource = pool.borrow_from_pool();
    if (resource) {
        // Move resource out
        auto extracted = std::move(*resource);
        
        // Mark as invalid so it's not returned to pool
        resource.invalidate();
        
        // Use extracted resource
        extracted.doWork();
    }
}
// Resource is NOT returned to pool
```

@subsection ug_invalidate_consumed Invalidating Consumed Resources

```cpp
{
    auto resource = pool.borrow_from_pool();
    if (resource) {
        // Consume the resource
        process_and_consume(*resource);
        
        // Mark as invalid
        resource.invalidate();
    }
}
// Resource is NOT returned to pool
```

@subsection ug_check_validity Checking Resource Validity

```cpp
{
    auto resource = pool.borrow_from_pool();
    if (resource && resource.is_valid()) {
        // Resource is valid and will be returned
        resource->doWork();
    }
}
```

@section ug_troubleshooting Troubleshooting

@subsection ug_deadlock Avoiding Deadlocks

**Problem**: Application hangs when calling pool methods.

**Cause**: Factory callback calls pool methods.

**Solution**:
```cpp
// WRONG - Will deadlock
auto pool = resource_pool<Resource>(
    10,
    [](auto& p) -> std::expected<scoped_resource<Resource>, pool_error> {
        auto res = p.borrow_from_pool();  // DEADLOCK!
        return res;
    }
);

// CORRECT - Only create and return
auto pool = resource_pool<Resource>(
    10,
    [](auto& p) -> std::expected<scoped_resource<Resource>, pool_error> {
        return scoped_resource<Resource>(
            [&p](Resource&& res, bool isvalid) -> std::expected<void, pool_error> {
                if (isvalid) {
                    return p.add_to_pool(std::move(res));
                }
                return {};
            },
            Resource::create()
        );
    }
);
```

@subsection ug_pool_empty Pool is Empty

**Problem**: Getting `NoMoreResources` error.

**Solutions**:
1. Pre-populate pool before use
2. Use AutoGrow policy
3. Implement retry logic with backoff
4. Increase pool capacity

@subsection ug_performance Performance Issues

**Problem**: Pool operations are slow.

**Solutions**:
1. Increase pool capacity to reduce contention
2. Ensure factory callbacks are fast
3. Monitor with `to_json()` to check utilization
4. Use appropriate resource types (shared_ptr vs unique_ptr)
5. Pre-populate pool to avoid on-demand creation overhead

@section ug_best_practices Best Practices

1. **Always use RAII**: Let scoped_resource handle resource return
2. **Pre-populate pools**: Add resources before concurrent access
3. **Handle errors**: Check `std::expected` return values from `borrow_from_pool()`
4. **Keep factories simple**: Factory callbacks should only create resources
5. **Monitor utilization**: Use `to_json()` to track pool health
6. **Use appropriate types**: Prefer shared_ptr or unique_ptr
7. **Test concurrency**: Verify thread safety with your specific use case
8. **Avoid manual add_to_pool()**: Only use in advanced scenarios
9. **Document assumptions**: Clearly document resource lifecycle expectations
10. **Profile under load**: Test with realistic concurrent access patterns
11. **Use AutoGrow wisely**: AutoGrow can mask resource leaks; monitor carefully
12. **Invalidate strategically**: Only invalidate when resource is truly unusable
