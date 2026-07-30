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
| `resource_pool(uint8_t capacity, auto_add_policy policy)` | Create pool with capacity and growth policy |
| `resource_pool(uint8_t capacity, factory_callback, cleanup_callback)` | Create pool with custom factory and cleanup |
| `resource_pool(cleanup_callback)` | Create pool with only cleanup callback |

@subsection rp_methods Methods

| Method | Returns | Description |
|---|---|---|
| `borrow_from_pool()` | `std::expected<SRT, pool_error>` | Borrow resource from pool |
| `seed_to_pool(Args...)` | `std::expected<void, pool_error>` | Add resource via in-place construction |
| `seed_to_pool(T&&)` | `std::expected<void, pool_error>` | Add resource via move |
| `size()` | `std::expected<size_t, pool_error>` | Get available resources count |
| `clear()` | `std::expected<void, pool_error>` | Clear all resources |
| `create_resource(Args...)` | `SRT` | Create scoped resource (for custom use) |
| `to_json()` | `std::expected<json_ref, pool_error>` | Get statistics as JSON |

@subsection rp_callbacks Callbacks

**Factory Callback**
```cpp
std::function<std::expected<SRT, pool_error>(resource_pool&)>
```
Called when pool needs a resource. Must NOT call pool methods.

**Cleanup Callback**
```cpp
std::function<void(T&&)>
```
Called for each resource during destruction. Must NOT call pool methods.

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
- `seeds`: Added via seed_to_pool()
- `autoadds`: Created on-demand

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
| `to_json()` | `nlohmann::json` | Serialize to JSON |

@subsection sr_semantics Move Semantics

- **Move Constructor**: Transfers ownership, invalidates source
- **Move Assignment**: Returns current resource before taking new one
- **Copy Operations**: Deleted (move-only)

---

@section enums Enumerations

@subsection auto_add_policy auto_add_policy

Controls auto-grow behavior:
- `NoGrow`: Pool returns error when exhausted
- `AutoGrow`: Pool creates resources on-demand

@subsection pool_error pool_error

Error codes:
- `NoMoreResources`: Pool exhausted, no factory available
- `UnderCapacityNoAutoGrow`: Under capacity but auto-grow disabled
- `ShutdownInitiated`: Pool is shutting down
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
pool.seed_to_pool(Resource());
pool.seed_to_pool(Resource());
// ...
auto res = pool.borrow_from_pool();
```

@subsection pattern_autogrow Auto-Growing Pool

```cpp
siddiqsoft::arrp::resource_pool<Resource> pool(
    10,
    siddiqsoft::arrp::auto_add_policy::AutoGrow
);
// Resources created on-demand up to capacity
```

@subsection pattern_custom_factory Custom Factory

```cpp
siddiqsoft::arrp::resource_pool<Resource> pool(
    10,
    [](auto& pool) {
        return pool.create_resource(/* args */);
    }
);
```

@subsection pattern_cleanup_callback Cleanup Callback

```cpp
siddiqsoft::arrp::resource_pool<Resource> pool(
    10,
    [](auto& pool) { return pool.create_resource(); },
    [](Resource&& res) {
        res.cleanup();
    }
);
```

@subsection pattern_invalidate Invalidate Resource

```cpp
auto res = pool.borrow_from_pool();
if (res) {
    auto extracted = std::move(*res);
    res.invalidate();  // Don't return to pool
    // Use extracted elsewhere
}
```

---

@section thread_safety_details Thread Safety Details

**Thread-Safe Operations**
- `borrow_from_pool()`
- `seed_to_pool()`
- `size()`
- `clear()`
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
