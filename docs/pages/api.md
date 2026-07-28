@page api API Reference

@section api_overview Overview

Complete API reference for the ARRP (Auto Returning Resource Pool) library.

@section api_resource_pool resource_pool Class

Thread-safe resource pool with automatic lifecycle management.

@subsection api_rp_template Template Parameters

```cpp
template <typename T, typename SRT = scoped_resource<T>>
class resource_pool
```

- **T**: Resource type. Must be move-constructible and non-arithmetic.
- **SRT**: @ref scoped_resource "Scoped resource wrapper type". Defaults to @ref scoped_resource<T>.

@subsection api_rp_constructors Constructors

#### Constructor with Capacity and Auto-Add Policy
```cpp
resource_pool(uint8_t init_capacity = resource_pool_limits::DefaultCapacity,
              auto_add_policy add_policy = auto_add_policy::NoGrow);
```
Creates a pool with specified capacity and growth policy.
- **init_capacity**: Maximum number of resources (clamped to valid range)
- **add_policy**: Controls resource creation behavior
  - **NoGrow**: Returns error when empty (default)
  - **AutoGrow**: Creates resources on demand

#### Constructor with Factory Callback
```cpp
resource_pool(uint8_t init_capacity,
              std::function<std::expected<SRT, pool_error>(resource_pool&)>&& new_resource_callback,
              std::function<void(T&&)>&& on_shutdown_callback = {});
```
Creates a pool with custom resource factory callback.

**Parameters**:
- **init_capacity**: Maximum number of resources
- **new_resource_callback**: Factory function to create new resources
- **on_shutdown_callback**: Optional cleanup callback invoked during destruction

**Warning**: Factory callbacks MUST NOT call any pool methods (would cause deadlock).

#### Constructor with Cleanup Callback
```cpp
resource_pool(std::function<void(T&&)>&& on_shutdown_callback);
```
Creates a pool with only cleanup callback.

@subsection api_rp_methods Methods

#### borrow_from_pool()
```cpp
[[nodiscard]] auto borrow_from_pool() -> std::expected<SRT, pool_error>;
```
Borrows a resource from the pool.

**Returns**: `std::expected<SRT, pool_error>` containing:
- Success: @ref scoped_resource "scoped_resource wrapper" that automatically returns resource on destruction
- Error: @ref pool_error indicating why borrow failed

**Possible Errors**:
- `NoMoreResources`: Pool is exhausted and no factory callback available
- `UnderCapacityNoAutoGrow`: Pool is under capacity but no auto-grow policy
- `ShutdownInitiated`: Pool is shutting down

**Thread Safety**: Thread-safe. Resource creation happens outside lock.

**Performance**: O(1) amortized.

**Exception Safety**: Strong - if factory throws, state remains consistent.

**See Also**: @ref scoped_resource for automatic resource management.

#### add_to_pool()
```cpp
auto add_to_pool(T&& item) -> std::expected<void, pool_error>;

template <typename... Args>
auto add_to_pool(Args&&... args) -> std::expected<void, pool_error>;
```
Adds a resource to the pool.

**Parameters**:
- `item`: Resource to add (moved)
- `args`: Arguments to forward to T's constructor (in-place construction)

**Returns**: `std::expected<void, pool_error>` indicating success or error.

**Possible Errors**:
- `ShutdownInitiated`: Pool is shutting down

**Note**: Typically called automatically by @ref scoped_resource destructor.

**Thread Safety**: Thread-safe.

**Performance**: O(1) amortized.

**Exception Safety**: noexcept.

#### size()
```cpp
[[nodiscard]] auto size() const -> std::expected<size_t, pool_error>;
```
Returns number of available resources in pool (excludes checked-out resources).

**Returns**: `std::expected<size_t, pool_error>` containing pool size or error.

**Thread Safety**: Thread-safe.

**Performance**: O(1) with lock acquisition.

**Exception Safety**: noexcept.

#### clear()
```cpp
auto clear() -> std::expected<void, pool_error>;
```
Removes and destroys all resources in pool.

**Returns**: `std::expected<void, pool_error>` indicating success or error.

**Note**: Does not affect checked-out resources.

**Thread Safety**: Thread-safe.

**Performance**: O(n) where n is pool size.

**Exception Safety**: noexcept.

#### to_json()
```cpp
auto to_json() -> std::expected<std::reference_wrapper<nlohmann::json>, pool_error>;
```
Serializes pool state to JSON (requires nlohmann/json).

**Returns**: `std::expected<std::reference_wrapper<nlohmann::json>, pool_error>` containing:
- Success: Reference to JSON object with:
  - `capacity`: Maximum resources
  - `size`: Available resources
  - `deficit`: Resources needed to reach capacity
  - `capsize`: Peak capacity reached
  - `abandoned`: Invalidated resources
  - `adds`: Total resources added
  - `autoadds`: Resources created on-demand
  - `returns`: Total resources returned
  - `borrows`: Total resources borrowed
- Error: @ref pool_error if pool is shutting down

**Thread Safety**: Thread-safe.

**Performance**: O(1) with lock acquisition.

@section api_scoped_resource scoped_resource Class

RAII wrapper for automatic resource return to pool. Implements the Resource Acquisition Is Initialization (RAII) pattern.

@subsection api_sr_template Template Parameters

- **T**: Resource type. Must be move-constructible and non-arithmetic.

@subsection api_sr_constructors Constructors

#### Explicit Constructor
```cpp
explicit scoped_resource(PutbackCallbackFunc&& f, T&& src);
```
Wraps resource with return callback.

**Parameters**:
- `f`: Callback function invoked on destruction to return resource to pool
- `src`: R-value reference to the resource to wrap

**Note**: Typically created by @ref resource_pool::borrow_from_pool() "resource_pool::borrow_from_pool()".

#### In-Place Constructor
```cpp
template <typename... Args>
scoped_resource(PutbackCallbackFunc&& f, Args&&... args);
```
Constructs resource in-place with return callback.

**Parameters**:
- `f`: Callback function invoked on destruction
- `args`: Arguments to forward to T's constructor

@subsection api_sr_methods Methods

#### operator*()
```cpp
auto operator*() -> T&;
```
Dereferences wrapped resource.

**Returns**: Reference to the wrapped resource.

**Example**:
```cpp
auto res = pool.borrow_from_pool();
if (res) {
    (*res).doSomething();  // Access via dereference
}
```

#### operator->()
```cpp
auto operator->() -> T*;
```
Pointer-like access to wrapped resource.

**Returns**: Pointer to the wrapped resource, or nullptr if invalid.

**Example**:
```cpp
auto res = pool.borrow_from_pool();
if (res) {
    res->doSomething();  // Pointer-like access
}
```

#### operator T&()
```cpp
explicit operator T&();
```
Explicit conversion to resource reference.

**Example**:
```cpp
auto res = pool.borrow_from_pool();
if (res) {
    T& ref = static_cast<T&>(res);  // Explicit conversion
}
```

#### invalidate()
```cpp
virtual void invalidate();
```
Marks resource as invalid to prevent automatic return to pool.

**Use Cases**:
- Resource has been moved out
- Resource has been consumed
- Resource is corrupted or unusable

**Example**:
```cpp
auto res = pool.borrow_from_pool();
if (res) {
    auto extracted = std::move(*res);
    res.invalidate();  // Don't return moved-out resource
}
```

#### is_valid()
```cpp
virtual bool is_valid() const;
```
Checks if resource is valid and will be returned to pool.

**Returns**: true if valid, false otherwise.

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

#### Resource Assignment
```cpp
scoped_resource& operator=(T&& src);
```
Assigns a new resource value to this wrapper.

**Note**: The resource is marked as valid.

@section api_enums Enumerations

#### auto_add_policy
```cpp
enum class auto_add_policy {
    NoGrow,    // Return error when pool is empty
    AutoGrow   // Create resources on demand
};
```

Controls whether @ref resource_pool "resource_pool" automatically creates resources when empty.

**Values**:
- **NoGrow**: Returns error when @ref resource_pool::borrow_from_pool() "borrow_from_pool()" is called on empty pool
- **AutoGrow**: Automatically creates new resources on demand up to capacity limit

**Usage**:
```cpp
// NoGrow policy (default)
resource_pool<Resource> pool1(10, auto_add_policy::NoGrow);

// AutoGrow policy
resource_pool<Resource> pool2(10, auto_add_policy::AutoGrow);
```

#### pool_error
```cpp
enum class pool_error {
    NoMoreResources,           // Pool exhausted
    UnderCapacityNoAutoGrow,   // Under capacity but no auto-grow
    ShutdownInitiated,         // Pool is shutting down
    Unknown                    // Unknown error
};
```

Error codes returned by pool operations.

@section api_exceptions Error Handling

All errors are returned via `std::expected<T, pool_error>`:

- **NoMoreResources** - @ref resource_pool::borrow_from_pool() "borrow_from_pool()" on exhausted pool with no factory
- **UnderCapacityNoAutoGrow** - @ref resource_pool::borrow_from_pool() "borrow_from_pool()" when under capacity but no auto-grow policy
- **ShutdownInitiated** - Any operation during pool shutdown

@section api_thread_safety Thread Safety

**Guarantees**:
- All public methods of @ref resource_pool "resource_pool" are thread-safe
- Multiple threads can safely call @ref resource_pool::borrow_from_pool() "borrow_from_pool()" and @ref resource_pool::add_to_pool() "add_to_pool()" concurrently
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
| @ref resource_pool::borrow_from_pool() "borrow_from_pool()" | O(1) amortized | Resource creation outside lock |
| @ref resource_pool::add_to_pool() "add_to_pool()" | O(1) amortized | Simple push_back operation |
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

- **@ref resource_pool::borrow_from_pool() "borrow_from_pool()"**: Strong - if factory throws, state remains consistent
- **@ref resource_pool::add_to_pool() "add_to_pool()"**: noexcept
- **@ref resource_pool::clear() "clear()"**: noexcept
- **@ref resource_pool::size() "size()"**: noexcept
- **@ref resource_pool::to_json() "to_json()"**: Strong

@section api_best_practices Best Practices

1. **Always use RAII**: Let @ref scoped_resource "scoped_resource" handle resource return
2. **Factory callbacks**: Only create and return resources, never call @ref resource_pool "resource_pool" methods
3. **Error handling**: Check `std::expected` return values from @ref resource_pool::borrow_from_pool() "borrow_from_pool()"
4. **Resource types**: Prefer shared_ptr or unique_ptr over raw pointers
5. **Monitoring**: Use @ref resource_pool::to_json() "to_json()" to track pool utilization
6. **Thread safety**: No external synchronization needed
7. **Invalidation**: Use @ref scoped_resource::invalidate() "invalidate()" only when resource is moved out or consumed

@section api_quick_links Quick Links

- @ref resource_pool "resource_pool class" - Main pool implementation
- @ref scoped_resource "scoped_resource class" - RAII wrapper
- @ref auto_add_policy "auto_add_policy enum" - Growth policy control
- @ref pool_error "pool_error enum" - Error codes
- @ref usage_guide "Usage Guide" - Practical examples and patterns
