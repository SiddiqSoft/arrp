@page security Security Considerations

@section sec_overview Overview

This page covers security considerations and best practices when using the ARRP library.

@section sec_thread_safety Thread Safety

@subsection sec_ts_guarantees Thread Safety Guarantees

The resource_pool class provides the following thread safety guarantees:

- **All public methods are thread-safe**: Multiple threads can safely call checkout(), checkin(), size(), and clear() concurrently.
- **Internal synchronization**: The pool uses std::mutex to protect shared state.
- **No external synchronization required**: Users do not need to add additional locks.
- **Atomic counters**: Operation counters use std::atomic for lock-free updates.

@subsection sec_ts_best_practices Best Practices

1. **Do not hold resources across thread boundaries unnecessarily**
   ```cpp
   {
       auto res = pool.checkout();
       res->doWork();
   }
   ```

2. **Use RAII to ensure resources are returned**
   ```cpp
   {
       auto res = pool.checkout();
   }
   ```

@section sec_factory_callbacks Factory Callback Security

@subsection sec_fc_deadlock Deadlock Prevention

**CRITICAL**: Factory callbacks MUST NOT call any pool methods.

```cpp
// WRONG - Will cause deadlock
auto pool = resource_pool<Resource>(
    [](auto& p) -> scoped_resource<Resource> {
        auto res = p.checkout();
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

@section sec_resource_types Resource Type Security

@subsection sec_rt_constraints Type Constraints

The pool enforces type constraints at compile-time:

- **Move-constructible**: Resources must be move-constructible
- **Non-arithmetic**: Primitive types (int, float, etc.) are not allowed
- **Type-safe**: C++20 concepts prevent invalid types

```cpp
resource_pool<std::string> pool1;
resource_pool<std::unique_ptr<Connection>> pool2;
resource_pool<std::shared_ptr<Resource>> pool3;
```

@section sec_exception_safety Exception Safety

The resource_pool provides strong exception safety guarantees:

- **checkout()**: If factory callback throws, the checkout count is properly decremented
- **checkin()**: No exceptions thrown (noexcept)
- **clear()**: No exceptions thrown (noexcept)
- **size()**: No exceptions thrown (noexcept)

```cpp
try {
    auto res = pool.checkout();
} catch (const std::exception& e) {
    // Pool state is valid; can retry
}
```

@section sec_best_practices General Best Practices

1. **Always use RAII**: Let scoped_resource handle resource return
2. **Avoid manual checkin()**: Only use in advanced scenarios
3. **Monitor pool health**: Use to_json() to track utilization
4. **Handle exceptions**: Catch std::runtime_error from checkout()
5. **Keep factories simple**: Factory callbacks should only create resources
6. **Use appropriate types**: Prefer shared_ptr or unique_ptr over raw pointers
7. **Test concurrency**: Verify thread safety with your specific use case

@section sec_see_also See Also

- @ref usage_guide - Detailed usage guide
- @ref api - Complete API reference
- @ref examples - Real-world examples
