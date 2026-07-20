@page quick_reference Quick Reference

@section qr_common_tasks Common Tasks

@subsection qr_create_pool Create a Resource Pool

```cpp
// With NoGrow policy (default)
siddiqsoft::arrp::resource_pool<std::string> pool;

// With AutoGrow policy
siddiqsoft::arrp::resource_pool<std::string> pool(
    siddiqsoft::arrp::resource_pool<std::string>::auto_add_policy::AutoGrow
);

// With custom factory
siddiqsoft::arrp::resource_pool<MyResource> pool(
    [](auto& p) -> siddiqsoft::arrp::scoped_resource<MyResource> {
        return siddiqsoft::arrp::scoped_resource<MyResource>(
            MyResource::create(),
            [&p](MyResource&& res) { p.checkin(std::move(res)); }
        );
    }
);
```

@subsection qr_add_resources Add Resources to Pool

```cpp
pool.checkin(std::make_shared<MyResource>());

for (int i = 0; i < 10; ++i) {
    pool.checkin(std::make_shared<MyResource>());
}
```

@subsection qr_borrow_resource Borrow a Resource

```cpp
try {
    auto resource = pool.checkout();
    resource->doSomething();
} catch (const std::runtime_error& e) {
    std::cerr << "Failed to borrow resource: " << e.what() << std::endl;
}
```

@subsection qr_check_size Check Pool Size

```cpp
size_t available = pool.size();
std::cout << "Available resources: " << available << std::endl;
```

@subsection qr_clear_pool Clear the Pool

```cpp
pool.clear();
```

@subsection qr_monitor_pool Monitor Pool State

```cpp
auto state = pool.to_json();
std::cout << state.dump(2) << std::endl;

std::cout << "Capacity: " << state["capacity"] << std::endl;
std::cout << "Available: " << state["size"] << std::endl;
std::cout << "Checked out: " << state["checkedout"] << std::endl;
std::cout << "Total borrows: " << state["counters"]["borrow"] << std::endl;
```

@section qr_patterns Common Patterns

@subsection qr_pattern_raii RAII Pattern

```cpp
{
    auto resource = pool.checkout();
    // Use resource
}
```

@subsection qr_pattern_exception Exception Handling

```cpp
try {
    auto resource = pool.checkout();
} catch (const std::runtime_error& e) {
    // Handle pool exhaustion
}
```

@subsection qr_pattern_multithreaded Multi-threaded Usage

```cpp
std::vector<std::thread> threads;
for (int t = 0; t < 4; ++t) {
    threads.emplace_back([&pool]() {
        for (int i = 0; i < 100; ++i) {
            auto resource = pool.checkout();
        }
    });
}
for (auto& t : threads) {
    t.join();
}
```

@section qr_troubleshooting Troubleshooting

@subsection qr_pool_empty Pool is Empty

**Problem**: Getting "No items in the pool" exception.

**Solution**: 
- Add resources with checkin() before calling checkout()
- Or use AutoGrow policy to create resources on demand

@subsection qr_deadlock Deadlock Issues

**Problem**: Application hangs when calling pool methods.

**Solution**:
- Ensure factory callbacks do NOT call pool methods
- Factory should only create and return resources

@section qr_api_summary API Summary

| Method | Purpose | Returns |
|--------|---------|---------|
| checkout() | Borrow a resource | scoped_resource<T> |
| checkin(T&&) | Return a resource | void |
| size() | Get available count | size_t |
| clear() | Remove all resources | void |
| to_json() | Get pool state | nlohmann::json |

@section qr_see_also See Also

- @ref usage_guide - Detailed usage guide
- @ref examples - Real-world examples
- @ref api - Complete API reference
