@page api API Reference

@section api_overview Overview

This page provides a comprehensive API reference for the ARRP library.

@section api_resource_pool resource_pool Class

@subsection api_rp_template Template Parameters

```cpp
template <typename T, typename SRT = scoped_resource<T>, uint8_t InitCapacity = DefaultCapacity>
class resource_pool
```

- **T**: The resource type to be pooled. Must be move-constructible and non-arithmetic.
- **SRT**: The scoped resource wrapper type. Defaults to scoped_resource<T>.
- **InitCapacity**: The initial capacity of the pool (max 255).

@subsection api_rp_methods Methods

#### checkout()
```cpp
[[nodiscard]] auto checkout() -> SRT;
```
Borrows a resource from the pool. Returns a scoped_resource wrapper that automatically returns the resource when destroyed.

**Throws**: std::runtime_error if pool is at capacity and empty.

**Thread Safety**: Thread-safe. Resource creation happens outside the lock.

**Performance**: O(1) amortized.

#### checkin()
```cpp
void checkin(T&& raw_resource);
```
Returns a resource to the pool. Typically called automatically by scoped_resource destructor.

**Thread Safety**: Thread-safe.

**Performance**: O(1) amortized.

#### size()
```cpp
[[nodiscard]] size_t size() const noexcept;
```
Returns the number of available resources in the pool.

**Thread Safety**: Thread-safe.

**Performance**: O(1) with lock acquisition.

#### clear()
```cpp
void clear() noexcept;
```
Removes and destroys all resources in the pool.

**Thread Safety**: Thread-safe.

**Performance**: O(n) where n is pool size.

#### to_json()
```cpp
nlohmann::json to_json() const;
```
Serializes pool state to JSON (requires nlohmann/json).

**Returns**: JSON object with pool statistics.

**Thread Safety**: Thread-safe.

@section api_scoped_resource scoped_resource Class

@subsection api_sr_methods Methods

#### operator*()
```cpp
auto operator*() -> T&;
```
Dereferences the wrapped resource.

#### invalidate()
```cpp
void invalidate();
```
Marks the resource as invalid to prevent automatic return to pool.

@section api_exceptions Exceptions

All exceptions thrown by ARRP are of type std::runtime_error:

- "No items in the pool; add something first." - Thrown when checkout() is called on an empty pool with NoGrow policy.
- "Pool Size:X checkedout:Y capacity:Z" - Thrown when pool is at capacity and empty.

@section api_thread_safety Thread Safety

All public methods of resource_pool are thread-safe:
- Multiple threads can safely call checkout() and checkin() concurrently
- The pool uses internal mutexes to protect shared state
- No external synchronization is required

@section api_performance Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| checkout() | O(1) amortized | Resource creation outside lock |
| checkin() | O(1) amortized | Simple push_back operation |
| size() | O(1) | With lock acquisition |
| clear() | O(n) | n = pool size |
| to_json() | O(1) | With lock acquisition |

@section api_constraints Constraints

- Capacity limited to 255 resources (uint8_t)
- Factory callbacks must not call pool methods (would cause deadlock)
- Resources must be move-constructible
- Counters wrap around after ~18 quintillion operations (uint64_t)

@section api_see_also See Also

- @ref usage_guide - Detailed usage examples
- @ref examples - Real-world code examples
- @ref quick_reference - Quick lookup guide
