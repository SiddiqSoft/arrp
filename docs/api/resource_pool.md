# Class Template `resource_pool<T>`

Defined in header `<siddiqsoft/arrp.hpp>` / `<siddiqsoft/private/resource_pool.hpp>`.

```cpp
namespace siddiqsoft::arrp {
    template <NonNumericMoveConstructible T>
    class resource_pool;
}
```

`resource_pool<T>` is a thread-safe, non-copyable, non-movable class template that manages a pool of moveable resource instances of type `T`.

---

## Member Functions

### Constructors & Destructor

```cpp
resource_pool(
    uint8_t init_capacity = resource_pool_limits::DefaultCapacity,
    std::function<void(T&)>&& on_shutdown_callback = {}
);

resource_pool(
    std::function<void(T&)>&& on_shutdown_callback
);
```

- **`init_capacity`**: Initial capacity guidance. Clamped internally to `[resource_pool_limits::MinimumCapacity, resource_pool_limits::MaxCapacity]` (1 to 255).
- **`on_shutdown_callback`**: Optional cleanup callback executed for every available resource during `clear()` or pool destruction. The callback executes under pool synchronization lock and must not call pool methods.

---

### Borrowing Methods

#### `try_borrow`

```cpp
resource_guard<T> try_borrow(std::chrono::nanoseconds timeout = {});
```

Borrows the next available resource in FIFO order.

- **Parameters**: `timeout` - Duration to wait if no resource is available. A zero duration (`0ns`, default) returns immediately without blocking.
- **Return Value**: A `resource_guard<T>`. If borrowing succeeded, `operator bool()` is `true`. If exhausted/timed out, returns an invalid guard where `.error()` is `pool_error::NoMoreResources` or `pool_error::Timeout`.

#### `try_borrow_create`

```cpp
resource_guard<T> try_borrow_create(std::chrono::nanoseconds timeout = {});
```

Borrows an available resource from the pool, or invokes the registered factory callback if no resource is available after the timeout.

- **Parameters**: `timeout` - Duration to wait before invoking the factory.
- **Return Value**: A valid `resource_guard<T>` containing a borrowed or factory-created resource, or an invalid guard if no factory callback was set or shutdown was initiated.

---

### Configuration & Modification

#### `set_factory_callback`

```cpp
template <class F>
void set_factory_callback(F&& f);
```

Registers a factory callback used by `try_borrow_create()`.

- **Parameters**: `f` - Callable taking no parameters and returning `T` or `resource_guard<T>`.

#### `seed`

```cpp
template <class... Args>
pool_error seed(Args&&... args);

pool_error seed(T&& item);
```

Adds a new resource to the available pool.

- **In-place construction**: Constructs `T(std::forward<Args>(args)...)` directly in pool storage.
- **Move-seeding**: Moves an existing instance of `T` into pool storage.
- **Return Value**: `pool_error::Ok` on success, or `pool_error::ShutdownInitiated` if the pool is shutting down.

#### `clear`

```cpp
pool_error clear();
```

Removes and destroys all resources currently available (idle) in the pool. If a shutdown callback was provided, it is invoked for each removed resource.

- **Return Value**: `pool_error::Ok`.

---

### Inspection & Metrics

#### `size`

```cpp
size_t size() const noexcept;
```

Returns the current count of available (idle) resources in the pool.

#### `to_json`

```cpp
nlohmann::json to_json() const;
```

Returns a JSON object containing pool metrics (`capacity`, `size`, `borrows`, `returns`, `abandons`, `loans`, etc.). Only compiled when `nlohmann/json.hpp` is included prior to `arrp.hpp`.
