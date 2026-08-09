# Class Template `resource_guard<T>`

Defined in header `<siddiqsoft/arrp.hpp>` / `<siddiqsoft/private/resource_guard.hpp>`.

```cpp
namespace siddiqsoft::arrp {
    template <NonNumericMoveConstructible T>
    class resource_guard final;
}
```

`resource_guard<T>` is a move-only, `final` RAII handle wrapping a resource borrowed from a `resource_pool<T>`.

---

## Member Functions

### Constructors & Destructor

```cpp
resource_guard() noexcept;
resource_guard(resource_guard&& source) noexcept;
~resource_guard();
```

- **Default Constructor**: Constructs an invalid guard (`error() == pool_error::NoMoreResources`).
- **Move Constructor**: Transfers ownership of the borrowed resource and callback from `source`. Leaves `source` invalid.
- **Destructor**: If valid, invokes the return callback to send the resource back to the pool (or discards it if marked abandoned/invalidated).

---

### Validity & Inspection

#### `operator bool` / `has_value`

```cpp
explicit operator bool() const noexcept;
bool has_value() const noexcept;
```

Returns `true` if the guard holds a valid, usable resource; `false` otherwise.

#### `is_valid`

```cpp
bool is_valid() const noexcept;
```

Reports whether the destructor will return the resource back to the pool.

#### `error`

```cpp
pool_error error() const noexcept;
```

Returns the error code associated with an invalid guard (e.g. `pool_error::NoMoreResources`, `pool_error::Timeout`, `pool_error::ShutdownInitiated`).

---

### Accessors & Conversion

#### `operator*` & `operator->`

```cpp
T& operator*() &;
const T& operator*() const&;
T* operator->() noexcept;
const T* operator->() const noexcept;
```

Accesses the underlying resource `T`.
- **Precondition**: `operator bool()` must be `true`. `operator->()` returns `nullptr` if the guard is invalid.

#### Rvalue Conversion `explicit operator T() &&`

```cpp
explicit operator T() &&;
```

Moves the held resource out of the guard and marks the guard invalidated (`release_reason::Abandoned`). The resource is **not** returned to the pool on guard destruction.

---

### Mutation & Invalidation

#### `invalidate`

```cpp
void invalidate() noexcept;
```

Marks the guard invalid/abandoned. When the guard's destructor runs, the resource will be discarded rather than returned to the pool.

#### `to_json`

```cpp
nlohmann::json to_json() const;
```

Returns a JSON representation of the guard's state. Only compiled when `nlohmann/json.hpp` is included prior to `arrp.hpp`.
