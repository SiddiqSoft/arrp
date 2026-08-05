/**
@page api_reference API Reference

@section resource_pool_class resource_pool<T, SRT>

Thread-safe auto-returning resource pool.

@subsection rp_template Template Parameters
- `T`: Resource type (must satisfy NonNumericMoveConstructible)
- `SRT`: Scoped resource type (defaults to scoped_resource<T>)

@subsection rp_constructors Constructors

| Constructor | Description |
|---|---|
| `resource_pool(uint8_t capacity = resource_pool_limits::DefaultCapacity, std::function<void(T&)>&& on_shutdown_callback = {})` | Create a pool with fixed capacity and optional cleanup callback |
| `resource_pool(std::function<void(T&)>&& on_shutdown_callback)` | Create a pool with default capacity and cleanup callback |

@subsection rp_methods Methods

| Method | Returns | Description |
|---|---|---|
| `try_borrow()` | `SRT` | Borrow a resource from the pool. Returns an invalid scoped resource when unavailable. |
| `try_borrow_create(std::chrono::nanoseconds timeout = {})` | `SRT` | Borrow a resource or create one on demand using the registered factory callback. |
| `seed(Args&&...)` | `pool_error` | Add a resource to the pool by constructing it in place. |
| `seed(T&&)` | `pool_error` | Add a moved resource to the pool. |
| `size()` | `size_t` | Get the number of available resources in the pool. |
| `clear()` | `pool_error` | Remove all resources from the pool. |
| `set_factory_callback(F&&)` | `void` | Register a callback to create resources when the pool is empty. |
| `to_json()` | `nlohmann::json` | Export pool statistics to JSON when nlohmann/json support is enabled. |

@subsection rp_callbacks Callbacks

**Factory Callback**
```cpp
std::function<SRT()>
```
Called when the pool is empty and a resource must be created on demand. The callback may return either `T` or `SRT`.

**Cleanup Callback**
```cpp
std::function<void(T&)>
```
Called for each pooled resource during cleanup. Must NOT call pool methods.

@subsection rp_statistics Statistics

Access via `to_json()`:
- `capacity`: Maximum resources
- `size`: Available resources
- `deficit`: Resources needed
- `peaksize`: Peak size reached
- `borrows`: Total borrowed
- `returns`: Total returned
- `loans`: Currently borrowed
- `abandons`: Invalidated resources
- `seeds`: Added via seed()
- `autoadds`: Created on demand

---

@section scoped_resource_class scoped_resource<T>

RAII wrapper for managing resource lifecycle.

@subsection sr_template Template Parameters
- `T`: Resource type (must satisfy NonNumericMoveConstructible)

@subsection sr_operators Operators

| Operator | Returns | Description |
|---|---|---|
| `operator*()` | `T&` | Dereference to resource |
| `operator->()` | `T*` | Pointer access (nullptr if invalid) |
| `operator=(T&&)` | `scoped_resource&` | Assign new resource |
| `operator=(scoped_resource&&)` | `scoped_resource&` | Move assignment |

@subsection sr_methods Methods

| Method | Returns | Description |
|---|---|---|
| `invalidate()` | `void` | Mark resource as invalid (abandoned) |
| `is_valid()` | `bool` | Check if resource is valid |
| `has_value()` | `bool` | Check if the wrapper contains a valid resource |
| `error()` | `pool_error` | Retrieve the failure code when the wrapper is invalid |

@subsection sr_semantics Move Semantics

- **Move Constructor**: Transfers ownership, invalidates source
- **Move Assignment**: Returns current resource before taking new one
- **Copy Operations**: Deleted (move-only)

---

@section enums Enumerations

@subsection pool_error pool_error

Error codes:
- `Ok`: Operation succeeded
- `NoMoreResources`: Pool exhausted and no factory callback available
- `ShutdownInitiated`: Pool is shutting down
- `Timeout`: Resource was not available within the specified timeout
- `Unknown`: Unknown error

@subsection resource_pool_limits resource_pool_limits

Capacity constraints:
- `MinimumCapacity`: 1
- `DefaultCapacity`: 8
- `MaxCapacity`: 255

---

@section concepts_section Concepts

@subsection non_numeric_move_constructible NonNumericMoveConstructible<T>

Type T must satisfy:
- `std::move_constructible<T>`
- `std::move_assignable<T>`
- `!std::is_arithmetic<T>`

Prevents wrapping primitive types.

---

@section usage_patterns Usage Patterns

@subsection pattern_fixed_pool Fixed-Size Pool

```cpp
siddiqsoft::arrp::resource_pool<Resource> pool(10);
pool.seed(Resource());
```

@subsection pattern_factory_callback Factory Callback

```cpp
siddiqsoft::arrp::resource_pool<Resource> pool(10);
pool.set_factory_callback([] {
    return Resource();
});

auto res = pool.try_borrow_create();
```

@subsection pattern_invalidate Invalidate Resource

```cpp
auto res = pool.try_borrow();
if (res) {
    auto extracted = std::move(*res);
    res.invalidate();
}
```

---

@section thread_safety_details Thread Safety Details

**Thread-Safe Operations**
- `try_borrow()`
- `try_borrow_create()`
- `seed()`
- `size()`
- `clear()`
- `set_factory_callback()`
- `to_json()`

**NOT Thread-Safe**
- Individual `scoped_resource` instances
- Resource access via `operator*()` or `operator->()`

**Recommendation**: Each thread should have its own `scoped_resource` instance.

---

@section performance Performance Considerations

- **Mutex Type**: Use `std::mutex` for performance (default)
- **Capacity**: Keep capacity reasonable (1-255)
- **Callbacks**: Keep factory/cleanup callbacks fast
- **Statistics**: `to_json()` acquires lock; use sparingly in hot paths

---

@section limitations Limitations

- Capacity limited to 255 resources
- Callbacks must not call pool methods (deadlock risk)
- `scoped_resource` not thread-safe
- No support for arithmetic types (int, float, etc.)

*/
