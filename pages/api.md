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
- **SRT**: @ref scoped_resource "Scoped resource wrapper type". Defaults to @ref scoped_resource<T>.
- **InitCapacity**: Initial capacity (max 255).

@subsection api_rp_constructors Constructors

#### Default Constructor
```cpp
resource_pool(auto_add_policy add_policy = auto_add_policy::NoGrow);
```
Creates a pool with specified growth policy.
- **NoGrow**: Throws when empty (default)
- **AutoGrow**: Creates resources on demand

See @ref auto_add_policy for available policies.

#### Custom Factory Constructor
```cpp
resource_pool(std::function<SRT(resource_pool&)>&& new_resource_callback);
```
Creates a pool with custom resource factory callback.

Returns a @ref scoped_resource "scoped_resource" wrapping the newly created resource.

**Warning**: Factory callbacks MUST NOT call any pool methods (would cause deadlock).

@subsection api_rp_methods Methods

#### checkout()
```cpp
[[nodiscard]] auto checkout() -> SRT;
```
Borrows a resource from the pool.

**Returns**: @ref scoped_resource "scoped_resource wrapper" that automatically returns resource on destruction.

**Throws**: std::runtime_error if pool is at capacity and empty.

**Thread Safety**: Thread-safe. Resource creation happens outside lock.

**Performance**: O(1) amortized.

**Exception Safety**: Strong - if factory throws, checkout count is properly decremented.

**See Also**: @ref scoped_resource for automatic resource management.

#### checkin()
```cpp
void checkin(T&& raw_resource);
```
Returns a resource to the pool.

**Note**: Typically called automatically by @ref scoped_resource destructor.

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

RAII wrapper for automatic resource return to pool. Implements the Resource Acquisition Is Initialization (RAII) pattern.

@subsection api_sr_template Template Parameters

- **T**: Resource type. Must be move-constructible and non-arithmetic.

@subsection api_sr_constructors Constructors

#### Explicit Constructor
```cpp
explicit scoped_resource(T&& src, std::function<void(T&&)>&& f = {});
```
Wraps resource with optional return callback.

**Parameters**:
- `src`: R-value reference to the resource to wrap
- `f`: Optional callback function invoked on destruction to return resource to pool

**Note**: Typically created by @ref resource_pool::checkout() "resource_pool::checkout()".

@subsection api_sr_methods Methods

#### operator*()
```cpp
auto operator*() -> T&;
```
Dereferences wrapped resource.

**Returns**: Reference to the wrapped resource.

**Example**:
```cpp
auto res = pool.checkout();
(*res).doSomething();  // Access via dereference
```

#### operator T&()
```cpp
operator T&();
```
Implicit conversion to resource reference.

**Example**:
```cpp
auto res = pool.checkout();
T& ref = res;  // Implicit conversion
```

#### invalidate()
```cpp
void invalidate();
```
Marks resource as invalid to prevent automatic return to pool.

**Use Cases**:
- Resource has been moved out
- Resource has been consumed
- Custom resource management needed

**Example**:
```cpp
auto res = pool.checkout();
auto extracted = std::move(*res);
res.invalidate();  // Don't return moved-out resource
```

#### Move Constructor
```cpp
scoped_resource(scoped_resource&& src) noexcept;
```
Transfers ownership. Source becomes invalid.

**Note**: Ensures only one wrapper returns the resource to the pool.

#### Move Assignment
```cpp
scoped_resource& operator=(scoped_resource&& src) noexcept;
```
Transfers ownership. Previous resource (if valid) is returned to pool.

**Note**: If this wrapper held a valid resource, it is returned to the pool before assignment.

@section api_enums Enumerations

#### auto_add_policy
```cpp
enum class auto_add_policy {
    NoGrow,    // Throw when pool is empty
    AutoGrow   // Create resources on demand
};
```

Controls whether @ref resource_pool "resource_pool" automatically creates resources when empty.

**Values**:
- **NoGrow**: Throws std::runtime_error when @ref resource_pool::checkout() "checkout()" is called on empty pool
- **AutoGrow**: Automatically creates new resources on demand up to capacity limit

**Usage**:
```cpp
// NoGrow policy (default)
resource_pool<Resource> pool1;

// AutoGrow policy
resource_pool<Resource> pool2(auto_add_policy::AutoGrow);
```

@section api_exceptions Exceptions

All exceptions thrown by ARRP are of type std::runtime_error:

- **"No items in the pool; add something first."** - @ref resource_pool::checkout() "checkout()" on empty pool with @ref auto_add_policy::NoGrow "NoGrow policy"
- **"Pool Size:X checkedout:Y capacity:Z"** - @ref resource_pool::checkout() "checkout()" when at capacity and empty

@section api_thread_safety Thread Safety

**Guarantees**:
- All public methods of @ref resource_pool "resource_pool" are thread-safe
- Multiple threads can safely call @ref resource_pool::checkout() "checkout()" and @ref resource_pool::checkin() "checkin()" concurrently
- Internal mutexes protect shared state
- Atomic counters for lock-free statistics
- No external synchronization required

**Performance**:
- Resource creation happens outside lock to minimize contention
- Atomic operations for counters avoid lock overhead

**Note**: @ref scoped_resource "scoped_resource" is not thread-safe by itself; thread safety is provided by @ref resource_pool "resource_pool".

@section api_performance Performance Characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| @ref resource_pool::checkout() "checkout()" | O(1) amortized | Resource creation outside lock |
| @ref resource_pool::checkin() "checkin()" | O(1) amortized | Simple push_back operation |
| @ref resource_pool::size() "size()" | O(1) | With lock acquisition |
| @ref resource_pool::clear() "clear()" | O(n) | n = pool size |
| @ref resource_pool::to_json() "to_json()" | O(1) | With lock acquisition |

@section api_memory Memory Characteristics

- Pool overhead: ~200 bytes (mutex, deque, counters, callback)
- Per-resource overhead: ~40 bytes (deque node)
- No dynamic allocations after initialization

@section api_constraints Constraints

- Capacity limited to 255 resources (uint8_t)
- Factory callbacks must not call @ref resource_pool "resource_pool" methods
- Resources must be move-constructible and non-arithmetic types
- Counters wrap around after ~18 quintillion operations (uint64_t)

@section api_exception_safety Exception Safety

- **@ref resource_pool::checkout() "checkout()"**: Strong - if factory throws, state remains consistent
- **@ref resource_pool::checkin() "checkin()"**: noexcept
- **@ref resource_pool::clear() "clear()"**: noexcept
- **@ref resource_pool::size() "size()"**: noexcept
- **@ref resource_pool::to_json() "to_json()"**: Strong

@section api_best_practices Best Practices

1. **Always use RAII**: Let @ref scoped_resource "scoped_resource" handle resource return
2. **Factory callbacks**: Only create and return resources, never call @ref resource_pool "resource_pool" methods
3. **Exception handling**: Catch std::runtime_error from @ref resource_pool::checkout() "checkout()"
4. **Resource types**: Prefer shared_ptr or unique_ptr over raw pointers
5. **Monitoring**: Use @ref resource_pool::to_json() "to_json()" to track pool utilization
6. **Thread safety**: No external synchronization needed
7. **Invalidation**: Use @ref scoped_resource::invalidate() "invalidate()" only when resource is moved out or consumed

@section api_quick_links Quick Links

- @ref resource_pool "resource_pool class" - Main pool implementation
- @ref scoped_resource "scoped_resource class" - RAII wrapper
- @ref auto_add_policy "auto_add_policy enum" - Growth policy control
- @ref usage_guide "Usage Guide" - Practical examples and patterns
