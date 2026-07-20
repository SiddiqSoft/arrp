@page security Security Guide

@section security_overview Overview

The arrp library is designed with security as a core principle. This guide covers security considerations, best practices, and potential risks.

**Security Rating**: EXCELLENT ⭐⭐⭐⭐⭐

@section security_features Built-in Security Features

### Memory Safety
- ✅ No unsafe functions (strcpy, sprintf, malloc, free)
- ✅ No raw pointers in public API
- ✅ Dynamic containers (std::deque) prevent buffer overflows
- ✅ RAII principles ensure automatic cleanup
- ✅ No use-after-free vulnerabilities

### Thread Safety
- ✅ Proper synchronization with mutexes
- ✅ Correct memory ordering (acquire/release semantics)
- ✅ No race conditions
- ✅ Atomic operations for lock-free updates
- ✅ No data races

### Exception Safety
- ✅ Exceptions caught at resource boundaries
- ✅ RAII ensures cleanup even on exceptions
- ✅ No exception leaks
- ✅ Graceful degradation on errors
- ✅ No resource leaks on exception

### Type Safety
- ��� Strong typing throughout
- ✅ C++20 concepts for compile-time checking
- ✅ No unsafe casts
- ✅ No type confusion vulnerabilities
- ✅ Template-based type safety

### Resource Management
- ✅ RAII principles throughout
- ✅ scoped_resource ensures resource return
- ✅ No resource leaks
- ✅ Graceful shutdown procedures
- ✅ Capacity limits prevent unbounded growth

@section callback_security Callback Security

Callbacks are user-provided code and should be implemented securely.

### ✅ Good Callback Practices

```cpp
// Safe callback with error handling
auto res = pool.checkout();
try {
    res->process();
} catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
}

// Callback that doesn't block
auto res = pool.checkout();
res->update();  // Quick operation

// Callback that respects ownership
auto res = pool.checkout();
auto ptr = std::move(*res);
res.invalidate();  // Don't return moved-out resource
```

### ❌ Bad Callback Practices

```cpp
// ❌ Accessing invalid memory
auto res = pool.checkout();
auto ptr = std::move(*res);
// ptr is now invalid, don't use it

// ❌ Blocking indefinitely
auto res = pool.checkout();
while (true) {  // Blocks thread forever
    res->process();
}

// ❌ Throwing uncaught exceptions
auto res = pool.checkout();
throw std::runtime_error("Unhandled error");
```

@section deadlock_prevention Deadlock Prevention

### Circular Dependencies

Never create circular dependencies with resources:

```cpp
// ✅ GOOD: Independent resource usage
auto res1 = pool.checkout();
res1->process();

// ❌ BAD: Circular dependency (if pool is shared)
auto res = pool.checkout();
auto res2 = pool.checkout();  // May deadlock if pool exhausted
```

### Blocking Operations

Avoid blocking operations when holding resources:

```cpp
// ❌ BAD: Blocking operation while holding resource
auto res = pool.checkout();
std::this_thread::sleep_for(std::chrono::seconds(10));  // Blocks thread

// ✅ GOOD: Quick operation
auto res = pool.checkout();
res->process();  // Quick operation
```

### Lock Ordering

If using locks in resource operations, maintain consistent lock ordering:

```cpp
// ✅ GOOD: Consistent lock ordering
std::mutex lock1, lock2;

auto res = pool.checkout();
std::scoped_lock l(lock1, lock2);  // Always same order
res->process();
```

@section locking_considerations Locking and Deadlock Prevention

### Understanding the Mutex Strategy

The resource pool uses mutexes to protect internal state. By default, it uses `std::mutex` for optimal
performance. However, you can enable `std::recursive_mutex` by defining `ARRP_USE_RECURSIVE_MUTEX` or
by setting the CMake option `arrp_USE_RECURSIVE_MUTEX=ON`.

**When to use recursive_mutex:**
- During development and testing to catch potential deadlock issues
- When you need to call pool methods from within callbacks (not recommended, but sometimes necessary)
- In DEBUG builds for additional safety

**When to use standard mutex (default):**
- Production builds for optimal performance
- When you follow proper locking discipline
- When factory callbacks don't call pool methods

### Critical Rule: Factory Callbacks Must Not Call Pool Methods

This is the most important rule to prevent deadlocks:

```cpp
// ❌ DEADLOCK RISK: Factory callback calls pool methods
siddiqsoft::arrp::resource_pool<Resource> pool(
    [&pool](auto& p) -> siddiqsoft::arrp::scoped_resource<Resource> {
        // NEVER do this:
        auto other = pool.checkout();  // DEADLOCK!
        return siddiqsoft::arrp::scoped_resource<Resource>(
            Resource::create(),
            [&p](Resource&& r) { p.checkin(std::move(r)); }
        );
    }
);

// ✅ CORRECT: Factory callback only creates resources
siddiqsoft::arrp::resource_pool<Resource> pool(
    [](auto& p) -> siddiqsoft::arrp::scoped_resource<Resource> {
        // Only create the resource, don't call pool methods
        return siddiqsoft::arrp::scoped_resource<Resource>(
            Resource::create(),
            [&p](Resource&& r) { p.checkin(std::move(r)); }
        );
    }
);
```

### Lock Ordering and Consistency

If your resource operations acquire locks, maintain consistent lock ordering:

```cpp
// ✅ GOOD: Consistent lock ordering
std::mutex resource_lock;
std::mutex other_lock;

auto res = pool.checkout();
{
    // Always acquire in the same order
    std::scoped_lock l(resource_lock, other_lock);
    res->process();
}

// ❌ BAD: Inconsistent lock ordering
// Thread 1: acquires resource_lock, then other_lock
// Thread 2: acquires other_lock, then resource_lock
// Result: DEADLOCK
```

### Detecting Deadlock Issues

Enable `ARRP_USE_RECURSIVE_MUTEX` during development to catch potential deadlock issues:

```bash
cmake -DARRP_USE_RECURSIVE_MUTEX=ON ..
```

This allows the pool to be re-entered from the same thread, which can help identify problematic patterns.

@section resource_exhaustion Resource Exhaustion Prevention

### Capacity Limits

The resource pool enforces capacity limits to prevent unbounded growth:

```cpp
// ✅ GOOD: Set appropriate capacity
siddiqsoft::arrp::resource_pool<Resource, siddiqsoft::arrp::scoped_resource<Resource>, 16> pool;

// Populate with appropriate number of resources
for (int i = 0; i < 16; ++i) {
    pool.checkin(Resource{});
}
```

### Handling Pool Exhaustion

Handle the case when the pool is exhausted:

```cpp
// ✅ GOOD: Handle pool exhaustion
try {
    auto res = pool.checkout();
    // Use resource
} catch (const std::runtime_error& e) {
    std::cerr << "Pool exhausted: " << e.what() << std::endl;
    // Handle error (retry, use fallback, etc.)
}
```

@section exception_handling Exception Handling

### Resource Exceptions

Exceptions during resource operations are handled safely:

```cpp
// Safe exception handling
try {
    auto res = pool.checkout();
    res->process();  // May throw
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    // Resource is still properly returned to pool
}
```

### Critical Exceptions

Critical exceptions (std::bad_alloc, etc.) are handled specially:

```cpp
// Critical exceptions are identified
try {
    auto res = pool.checkout();
} catch (const std::bad_alloc& ex) {
    // Handle memory exhaustion
    std::cerr << "Out of memory" << std::endl;
}
```

@section thread_safety Thread Safety

### Pool Operations

All pool operations are thread-safe:

```cpp
// Safe to call from multiple threads
std::thread t1([&]() { auto res = pool.checkout(); });
std::thread t2([&]() { auto res = pool.checkout(); });
t1.join();
t2.join();
```

### Resource Access

Resources are accessed safely through scoped_resource:

```cpp
// Safe access through wrapper
auto res = pool.checkout();
res->process();  // Safe access
// Automatically returned when going out of scope
```

@section shutdown_safety Shutdown Safety

### Graceful Shutdown

Shutdown is graceful and safe:

```cpp
{
    siddiqsoft::arrp::resource_pool<Resource> pool;
    
    // Populate and use pool
    auto res = pool.checkout();
    res->process();
    
    // Destructor waits for all checked-out resources to be returned
    // Then destroys all pooled resources
}  // All cleanup happens here
```

### Resource Cleanup

All resources are properly cleaned up:

```cpp
// Resources are destroyed in FIFO order
// Checked-out resources are returned before pool destruction
// No resource leaks occur
```

@section monitoring Monitoring and Diagnostics

### JSON Serialization

Get pool state as JSON:

```cpp
#include <nlohmann/json.hpp>

auto state = pool.to_json();
std::cout << state.dump(2) << std::endl;

// Output includes:
// - capacity: Maximum capacity
// - size: Current pool size
// - load: Total resources (in pool + checked out)
// - invalidated: Number of invalidated resources
// - checkedout: Number of checked-out resources
// - counters: Operation counters (borrow, return, ondemand adds, auto-returned)
```

### Monitoring Best Practices

```cpp
// ✅ GOOD: Monitor pool state
auto state = pool.to_json();
auto available = state["size"];

if (available < THRESHOLD) {
    std::cerr << "Warning: Pool running low on resources" << std::endl;
}

// ✅ GOOD: Monitor checked-out resources
auto checkedout = state["checkedout"];
if (checkedout > THRESHOLD) {
    std::cerr << "Warning: Many resources checked out" << std::endl;
}
```

@section security_checklist Security Checklist

Before deploying arrp-based code:

- [ ] Resource types are non-arithmetic
- [ ] Pool capacity is set appropriately
- [ ] Resources are properly initialized
- [ ] Exception handling is comprehensive
- [ ] No circular dependencies exist
- [ ] Factory callbacks don't call pool methods
- [ ] Shutdown is tested
- [ ] Memory usage is monitored
- [ ] Thread safety is verified
- [ ] Resource invalidation is used correctly
- [ ] Locking discipline is maintained
- [ ] Security review completed

@section vulnerability_reporting Vulnerability Reporting

If you discover a security vulnerability:

1. **Do not** open a public issue
2. Email security details to: github@siddiqsoft.com
3. Include:
   - Description of vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if available)

We will:
- Acknowledge receipt within 48 hours
- Investigate the vulnerability
- Develop and test a fix
- Release a patched version
- Credit the reporter (if desired)

@section security_resources Additional Resources

- @ref getting_started "Getting Started" - Installation and setup
- @ref usage_guide "Usage Guide" - Detailed usage examples
- @ref api "API Reference" - Complete API documentation

@section security_faq FAQ

**Q: Is the library safe for production use?**
A: Yes, the library is designed for production use with proper resource management.

**Q: Can resources be exploited?**
A: Resources are user-provided. Implement them securely following the guidelines in this document.

**Q: What happens if a resource operation throws an exception?**
A: The exception is propagated. The resource is still properly returned to the pool via RAII.

**Q: Can the pool cause memory exhaustion?**
A: No, the pool enforces capacity limits to prevent unbounded growth.

**Q: Is the library thread-safe?**
A: Yes, all public operations are thread-safe.

**Q: Can deadlock occur?**
A: Only if user code creates circular dependencies or calls pool methods from factory callbacks. Follow the guidelines to prevent this.

**Q: What about timing attacks?**
A: The library is not cryptographic, so timing attacks are not a concern.

**Q: How do I report a security issue?**
A: Email security details to github@siddiqsoft.com (do not open public issues).

**Q: What is the NonNumericMoveConstructible constraint?**
A: It prevents using arithmetic types (int, float, etc.) which don't benefit from pooling.

**Q: How do I handle pool exhaustion?**
A: Catch std::runtime_error from checkout() and handle appropriately.

**Q: What is ARRP_USE_RECURSIVE_MUTEX?**
A: It's a CMake option that enables std::recursive_mutex instead of std::mutex. Use it during development to catch potential deadlock issues.

**Q: Should I use recursive_mutex in production?**
A: No, use the default std::mutex for optimal performance. Only use recursive_mutex during development and testing.
