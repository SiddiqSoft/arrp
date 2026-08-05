/**
@page usage_guide Usage Guide

@section getting_started Getting Started

@subsection include_header Include Header

```cpp
#include "siddiqsoft/resource_pool.hpp"
```

@subsection basic_example Basic Example

```cpp
#include "siddiqsoft/resource_pool.hpp"

class MyResource {
public:
    void doWork() { /* ... */ }
};

int main() {
    // Create pool with 10 resources, auto-grow enabled
    siddiqsoft::arrp::resource_pool<MyResource> pool(
        10,
        siddiqsoft::arrp::auto_add_policy::AutoGrow
    );

    // Borrow resource
    auto resource = pool.borrow_from_pool();
    if (resource) {
        resource->doWork();
    }
    // Resource automatically returned when scoped_resource is destroyed

    return 0;
}
```

---

@section pool_creation Pool Creation

@subsection create_simple Simple Pool

```cpp
// Default: 8 resources, no auto-grow
siddiqsoft::arrp::resource_pool<MyResource> pool;

// Custom capacity, no auto-grow
siddiqsoft::arrp::resource_pool<MyResource> pool(20);

// Custom capacity with auto-grow
siddiqsoft::arrp::resource_pool<MyResource> pool(
    20,
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);
```

@subsection create_factory Pool with Factory

```cpp
siddiqsoft::arrp::resource_pool<MyResource> pool(
    10,
    [](auto& pool) -> std::expected<siddiqsoft::arrp::scoped_resource<MyResource>, 
                                     siddiqsoft::arrp::pool_error> {
        return pool.create_resource("arg1", "arg2");
    }
);
```

@subsection create_cleanup Pool with Cleanup

```cpp
siddiqsoft::arrp::resource_pool<MyResource> pool(
    10,
    [](auto& pool) { return pool.create_resource(); },
    [](MyResource&& res) {
        std::cout << "Cleaning up resource\n";
        res.cleanup();
    }
);
```

---

@section resource_borrowing Borrowing Resources

@subsection borrow_basic Basic Borrowing

```cpp
auto resource = pool.borrow_from_pool();
if (resource) {
    // Use resource
    resource->doWork();
    // Automatically returned when scope ends
}
```

@subsection borrow_error Error Handling

```cpp
auto resource = pool.borrow_from_pool();
if (!resource) {
    switch (resource.error()) {
        case siddiqsoft::arrp::pool_error::NoMoreResources:
            std::cerr << "Pool exhausted\n";
            break;
        case siddiqsoft::arrp::pool_error::ShutdownInitiated:
            std::cerr << "Pool shutting down\n";
            break;
        default:
            std::cerr << "Unknown error\n";
    }
}
```

@subsection borrow_move Move Resource

```cpp
auto resource = pool.borrow_from_pool();
if (resource) {
    // Extract resource
    auto extracted = std::move(*resource);
    
    // Mark as invalid so pool discards it
    resource.invalidate();
    
    // Use extracted resource elsewhere
    use_elsewhere(std::move(extracted));
}
```

---

@section resource_seeding Seeding Resources

@subsection seed_move Seed via Move

```cpp
MyResource res("config");
auto result = pool.seed_to_pool(std::move(res));
if (result) {
    std::cout << "Resource added\n";
}
```

@subsection seed_inplace Seed via In-Place Construction

```cpp
auto result = pool.seed_to_pool("arg1", "arg2", "arg3");
if (result) {
    std::cout << "Resource created and added\n";
}
```

---

@section resource_invalidation Resource Invalidation

@subsection invalidate_example Invalidate Resource

```cpp
auto resource = pool.borrow_from_pool();
if (resource) {
    if (resource->isCorrupted()) {
        resource.invalidate();  // Don't return to pool
    }
    // Resource destroyed without returning to pool
}
```

@subsection check_validity Check Validity

```cpp
auto resource = pool.borrow_from_pool();
if (resource && resource.is_valid()) {
    // Resource is valid
}
```

---

@section statistics Monitoring Statistics

@subsection get_statistics Get Statistics

```cpp
auto stats = pool.to_json();
if (stats) {
    auto& json = stats.value().get();
    std::cout << json.dump(2) << std::endl;
}
```

@subsection statistics_fields Statistics Fields

```json
{
  "_typver": "siddiqsoft.arrp.resource_pool/0.0.0",
  "capacity": 10,
  "size": 5,
  "deficit": 0,
  "peaksize": 10,
  "abandons": 2,
  "seeds": 10,
  "autoadds": 0,
  "returns": 8,
  "borrows": 10,
  "loans": 2
}
```

---

@section threading Threading

@subsection thread_safe Thread-Safe Operations

All pool operations are thread-safe:

```cpp
std::thread t1([&pool]() {
    auto res = pool.borrow_from_pool();
    // Use resource
});

std::thread t2([&pool]() {
    auto res = pool.borrow_from_pool();
    // Use resource
});

t1.join();
t2.join();
```

@subsection thread_unsafe NOT Thread-Safe

Individual `scoped_resource` instances are NOT thread-safe:

```cpp
auto resource = pool.borrow_from_pool();

// WRONG: Don't share across threads
std::thread t([&resource]() {
    resource->doWork();  // NOT SAFE
});
```

@subsection thread_correct Correct Threading

```cpp
// Each thread gets its own resource
std::vector<std::thread> threads;
for (int i = 0; i < 4; ++i) {
    threads.emplace_back([&pool]() {
        auto resource = pool.borrow_from_pool();
        if (resource) {
            resource->doWork();
        }
    });
}

for (auto& t : threads) {
    t.join();
}
```

---

@section advanced Advanced Usage

@subsection custom_scoped_resource Custom Scoped Resource

```cpp
class MyCustomScoped : public siddiqsoft::arrp::scoped_resource<MyResource> {
public:
    using scoped_resource::scoped_resource;
    
    void invalidate() override {
        std::cout << "Custom invalidate\n";
        scoped_resource::invalidate();
    }
};

siddiqsoft::arrp::resource_pool<MyResource, MyCustomScoped> pool(10);
```

@subsection pool_clear Clear Pool

```cpp
auto result = pool.clear();
if (result) {
    std::cout << "Pool cleared\n";
}
```

@subsection pool_size Get Pool Size

```cpp
auto size = pool.size();
if (size) {
    std::cout << "Available resources: " << size.value() << "\n";
}
```

---

@section best_practices Best Practices

@subsection bp_capacity Choose Appropriate Capacity

```cpp
// Good: Reasonable capacity for typical workload
siddiqsoft::arrp::resource_pool<Resource> pool(10);

// Bad: Too large, defeats purpose of pooling
siddiqsoft::arrp::resource_pool<Resource> pool(255);
```

@subsection bp_callbacks Keep Callbacks Fast

```cpp
// Good: Quick factory
[](auto& pool) {
    return pool.create_resource();
}

// Bad: Slow factory
[](auto& pool) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return pool.create_resource();
}
```

@subsection bp_no_deadlock Don't Call Pool Methods in Callbacks

```cpp
// WRONG: Deadlock risk
[](auto& pool) {
    auto size = pool.size();  // DEADLOCK!
    return pool.create_resource();
}

// Correct: Only create resource
[](auto& pool) {
    return pool.create_resource();
}
```

@subsection bp_error_handling Always Check Results

```cpp
// Good
auto resource = pool.borrow_from_pool();
if (resource) {
    // Use resource
}

// Bad: Ignoring errors
auto resource = pool.borrow_from_pool();
resource->doWork();  // Undefined behavior if error
```

@subsection bp_invalidate Invalidate Corrupted Resources

```cpp
auto resource = pool.borrow_from_pool();
if (resource) {
    if (!resource->isHealthy()) {
        resource.invalidate();  // Don't return corrupted resource
    }
}
```

---

@section troubleshooting Troubleshooting

@subsection ts_pool_exhausted Pool Exhausted

**Problem**: `borrow_from_pool()` returns `NoMoreResources`

**Solutions**:
1. Increase capacity: `resource_pool<T>(20)` instead of `resource_pool<T>(10)`
2. Enable auto-grow: `auto_add_policy::AutoGrow`
3. Provide factory callback for on-demand creation
4. Ensure resources are being returned (check `loans` in statistics)

@subsection ts_deadlock Deadlock

**Problem**: Application hangs

**Causes**:
1. Calling pool methods from callbacks
2. Holding scoped_resource across thread boundaries
3. Recursive mutex not enabled

**Solutions**:
1. Don't call pool methods in callbacks
2. Each thread gets its own scoped_resource
3. Enable recursive mutex if needed: `#define ARRP_USE_RECURSIVE_MUTEX`

@subsection ts_memory_leak Memory Leak

**Problem**: Resources not being cleaned up

**Causes**:
1. Resources not returned to pool (check `loans` in statistics)
2. Cleanup callback not implemented

**Solutions**:
1. Ensure scoped_resource goes out of scope
2. Implement cleanup callback if needed
3. Check statistics: `pool.to_json()`

*/
