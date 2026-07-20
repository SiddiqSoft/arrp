@page api API Reference

@section api_overview Overview

Complete API reference for the ARRP (Auto Returning Resource Pool) library.

@section api_resource_pool resource_pool Class

Thread-safe resource pool with automatic lifecycle management.

@subsection api_rp_template Template Parameters

```cpp
template <typename T, typename SRT = scoped_resource<T>, uint8_t InitCapacity = DefaultCapacity>
class resource_pool
```

- **T**: Resource type. Must be move-constructible and non-arithmetic.
- **SRT**: Scoped resource wrapper type. Defaults to scoped_resource<T>.
- **InitCapacity**: Initial capacity (max 255).

@subsection api_rp_constructors Constructors

#### Default Constructor
```cpp
resource_pool(auto_add_policy add_policy = auto_add_policy::NoGrow);
```
Creates a pool with specified growth policy.
- **NoGrow**: Throws when empty (default)
- **AutoGrow**: Creates resources on demand

#### Custom Factory Constructor
```cpp
resource_pool(std::function<SRT(resource_pool&)>&& new_resource_callback);
```
Creates a pool with custom resource factory callback.

**Warning**: Factory callbacks MUST NOT call any pool methods (would cause deadlock).

@subsection api_rp_methods Methods

#### checkout()
```cpp
[[nodiscard]] auto checkout() -> SRT;
```
Borrows a resource from the pool.

**Returns**: scoped_resource wrapper that automatically returns resource on destruction.

**Throws**: std::runtime_error if pool is at capacity and empty.

**Thread Safety**: Thread-safe. Resource creation happens outside lock.

**Performance**: O(1) amortized.

**Exception Safety**: Strong - if factory throws, checkout count is properly decremented.

#### checkin()
```cpp
void checkin(T&& raw_resource);
```
Returns a resource to the pool.

**Note**: Typically called automatically by scoped_resource destructor.

**Thread Safety**: Thread-safe.

**Performance**: O(1) amortized.

**Exception Safety**: noexcept.

#### size()
```cpp
[[nodiscard]] size_t size() const noexcept;
```
Returns number of available resources in pool (excludes checked-out resources).

**Thread Safety**: Thread-safe.

**Performance**: O(1) with lock acquisition.

**Exception Safety**: noexcept.

#### clear()
```cpp
void clear() noexcept;
```
Removes and destroys all resources in pool.

**Note**: Does not affect checked-out resources.

**Thread Safety**: Thread-safe.

**Performance**: O(n) where n is pool size.

**Exception Safety**: noexcept.

#### to_json()
```cpp
nlohmann::json to_json() const;
```
Serializes pool state to JSON (requires nlohmann/json).

**Returns**: JSON object with:
- `capacity`: Maximum resources
- `size`: Available resources
- `load`: Total resources (in pool + checked out)
- `checkedout`: Currently checked out
- `counters`: Operation statistics

**Thread Safety**: Thread-safe.

**Performance**: O(1) with lock acquisition.

@section api_scoped_resource scoped_resource Class

RAII wrapper for automatic resource return to pool.

@subsection api_sr_constructors Constructors

#### Explicit Constructor
```cpp
explicit scoped_resource(T&& src, std::function<void(T&&)>&& f = {});
```
Wraps resource with optional return callback.

@subsection api_sr_methods Methods

#### operator*()
```cpp
auto operator*() -> T&;
```
Dereferences wrapped resource.

#### operator T&()
```cpp
operator T&();
```
Implicit conversion to resource reference.

#### invalidate()
```cpp
void invalidate();
```
Marks resource as invalid to prevent automatic return to pool.

**Use Cases**:
- Resource has been moved out
- Resource has been consumed
- Custom resource management needed

#### Move Constructor
```cpp
scoped_resource(scoped_resource&& src) noexcept;
```
Transfers ownership. Source becomes invalid.

#### Move Assignment
```cpp
scoped_resource& operator=(scoped_resource&& src) noexcept;
```
Transfers ownership. Previous resource (if valid) is returned to pool.

@section api_enums Enumerations

#### auto_add_policy
```cpp
enum class auto_add_policy {
    NoGrow,    // Throw when pool is empty
    AutoGrow   // Create resources on demand
};
```

@section api_exceptions Exceptions

All exceptions are std::runtime_error:

- **"No items in the pool; add something first."** - checkout() on empty pool with NoGrow policy
- **"Pool Size:X checkedout:Y capacity:Z"** - checkout() when at capacity and empty

@section api_thread_safety Thread Safety

**Guarantees**:
- All public methods are thread-safe
- Multiple threads can safely call checkout() and checkin() concurrently
- Internal mutexes protect shared state
- Atomic counters for lock-free statistics
- No external synchronization required

**Performance**:
- Resource creation happens outside lock to minimize contention
- Atomic operations for counters avoid lock overhead

@section api_performance Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| checkout() | O(1) amortized | Resource creation outside lock |
| checkin() | O(1) amortized | Simple push_back operation |
| size() | O(1) | With lock acquisition |
| clear() | O(n) | n = pool size |
| to_json() | O(1) | With lock acquisition |

@section api_memory Memory Characteristics

- Pool overhead: ~200 bytes (mutex, deque, counters, callback)
- Per-resource overhead: ~40 bytes (deque node)
- No dynamic allocations after initialization

@section api_constraints Constraints

- Capacity limited to 255 resources (uint8_t)
- Factory callbacks must not call pool methods
- Resources must be move-constructible
- Counters wrap around after ~18 quintillion operations (uint64_t)

@section api_exception_safety Exception Safety

- **checkout()**: Strong - if factory throws, state remains consistent
- **checkin()**: noexcept
- **clear()**: noexcept
- **size()**: noexcept
- **to_json()**: Strong

@section api_best_practices Best Practices

1. **Always use RAII**: Let scoped_resource handle resource return
2. **Factory callbacks**: Only create and return resources, never call pool methods
3. **Exception handling**: Catch std::runtime_error from checkout()
4. **Resource types**: Prefer shared_ptr or unique_ptr over raw pointers
5. **Monitoring**: Use to_json() to track pool utilization
6. **Thread safety**: No external synchronization needed
7. **Invalidation**: Use invalidate() only when resource is moved out or consumed
