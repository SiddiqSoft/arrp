@page api API Reference

## Table of Contents

- [resource_pool](#resource_pool)
- [scoped_resource](#scoped_resource)
- [Concepts](#concepts)

---

## resource_pool

Thread-safe auto-returning resource pool for managing reusable resources with automatic lifecycle management.

### Overview

The `resource_pool` class implements a thread-safe object pool pattern with automatic resource management using RAII principles. Resources are automatically returned to the pool when the `scoped_resource` wrapper goes out of scope, eliminating manual resource management and reducing the risk of resource leaks.

### Template Parameters

- `T` - The resource type to manage (must satisfy `NonNumericMoveConstructible` concept)
- `SRT` - The resource wrapper type (default: `scoped_resource<T>`)
- `InitCapacity` - Initial capacity limit (default: `resource_pool_limits::DefaultCapacity`, max: `resource_pool_limits::MaxCapacity`)

### Key Features

- **Thread-Safe**: All operations are protected by mutexes for concurrent access
- **RAII Pattern**: Resources are automatically returned via scoped_resource destructors
- **Capacity Management**: Enforces a maximum capacity limit to prevent unbounded growth
- **FIFO Ordering**: Resources are retrieved from the front and added to the back
- **Customizable Factory**: Supports custom resource creation callbacks
- **Diagnostic Counters**: Tracks borrow, return, and auto-add operations
- **JSON Serialization**: Provides pool state diagnostics via JSON (when nlohmann/json is available)

### Constructor

```cpp
resource_pool(std::function<SRT(resource_pool&)>&& new_resource_callback = {});
```

Creates a resource pool with optional custom resource factory callback.

**Parameters:**
- `new_resource_callback` - Optional callback to create new resources. If not provided, a default factory creates `T{}` wrapped in `SRT{}`

**Example:**
```cpp
// Using default factory
auto pool1 = siddiqsoft::arrp::resource_pool<MyResource>();

// Using custom factory
auto pool2 = siddiqsoft::arrp::resource_pool<MyResource>(
    [](auto& p) -> scoped_resource<MyResource> {
        return scoped_resource<MyResource>(
            MyResource::create(),
            [&p](MyResource&& res) { p.return_to_pool(std::move(res)); }
        );
    }
);
```

### Methods

#### size

```cpp
auto size() const;
```

Returns the current number of available resources in the pool.

**Returns:** Number of resources currently in the pool

**Thread Safety:** Thread-safe. Protected by mutex.

**Example:**
```cpp
auto available = pool.size();
std::cout << "Available resources: " << available << std::endl;
```

#### borrow_from_pool

```cpp
[[nodiscard]] auto borrow_from_pool() -> SRT;
```

Borrows a resource from the pool, wrapping it in a `scoped_resource` that automatically returns it when destroyed.

**Returns:** A `scoped_resource<T>` containing the borrowed resource

**Throws:** `std::runtime_error` if unable to obtain a resource (pool at capacity and no factory callback available)

**Thread Safety:** Thread-safe. Protected by mutex. Resource creation happens outside the lock.

**Algorithm:**
1. If the pool is non-empty, return the first resource (FIFO)
2. If the pool is empty but under capacity, create a new resource via factory
3. If at capacity and no resources available, throw `std::runtime_error`

**Example:**
```cpp
try {
    auto resource = pool.borrow_from_pool();
    // Use the resource...
    resource->doSomething();
    // Automatically returned when resource goes out of scope
} catch (const std::runtime_error& e) {
    std::cerr << "Failed to borrow resource: " << e.what() << std::endl;
}
```

#### return_to_pool

```cpp
void return_to_pool(T&& raw_resource);
```

Returns a resource to the pool, making it available for future checkout operations.

**Parameters:**
- `raw_resource` - The resource to return (moved into the pool)

**Thread Safety:** Thread-safe. Protected by mutex.

**Note:** This method is typically called automatically by the `scoped_resource` destructor, but can also be called manually if needed.

**Example:**
```cpp
// Typically called automatically:
{
    auto resource = pool.borrow_from_pool();
    // Use resource...
}  // Automatically returned here

// Manual return (advanced usage):
auto resource = pool.borrow_from_pool();
// ... use resource ...
pool.return_to_pool(std::move(*resource));
```

#### clear

```cpp
void clear();
```

Removes and destroys all resources currently in the pool.

**Thread Safety:** Thread-safe. Protected by mutex.

**Note:** Any checked-out resources are NOT affected by this operation.

**Example:**
```cpp
pool.clear();  // Remove all pooled resources
```

#### to_json

```cpp
nlohmann::json to_json() const;
```

Serializes pool state to JSON for diagnostics and monitoring.

**Returns:** nlohmann::json object with pool statistics

**Thread Safety:** Thread-safe. Protected by mutex.

**JSON Structure:**
```json
{
  "_typver": "siddiqsoft.arrp.resource_pool/0.0.0",
  "capacity": 10,
  "size": 5,
  "load": 8,
  "invalidated": 0,
  "checkedout": 3,
  "counters": {
    "autoreturns": 3,
    "newitems": 3,
    "return": 5,
    "borrow": 8
  }
}
```

**Fields:**
- `_typver`: Version identifier for the serialization format
- `capacity`: Maximum number of resources the pool can hold
- `size`: Number of resources currently in the pool
- `load`: Total resources (in pool + checked out)
- `invalidated`: Number of invalidated resources
- `checkedout`: Number of resources currently checked out
- `counters`: Operation counters
  - `autoreturns`: Number of automatic resource returns (via default factory callback)
  - `newitems`: Number of on-demand resource creations via factory callback
  - `return`: Number of return operations
  - `borrow`: Number of borrow operations

**Example:**
```cpp
auto state = pool.to_json();
std::cout << state.dump(2) << std::endl;
```

### Thread Safety

All public methods are thread-safe:
- `borrow_from_pool()` can be called from multiple threads
- `return_to_pool()` can be called from multiple threads
- `size()` is an atomic read
- `clear()` is protected by mutex
- `to_json()` is protected by mutex

---

## scoped_resource

RAII wrapper for managing resource lifecycle with automatic return to pool.

### Overview

The `scoped_resource` class implements the Resource Acquisition Is Initialization (RAII) pattern for managing resources that should be returned to a resource pool. It wraps a resource and automatically invokes a callback when the wrapper is destroyed, enabling automatic resource management without manual cleanup.

### Template Parameters

- `T` - The resource type to wrap (must satisfy `NonNumericMoveConstructible` concept)

### Key Features

- **RAII Pattern**: Automatically returns resources via destructor
- **Move Semantics**: Supports move construction and move assignment for efficient transfers
- **Validity Tracking**: Tracks whether a resource should be returned to the pool
- **Callback Support**: Executes custom callback on destruction
- **Debug Support**: Includes debug identifiers for tracking in DEBUG builds
- **Dereference Access**: Provides operator*, get() method, and type conversion for resource access
- **Invalidation**: Allows explicit invalidation to prevent automatic return

### Constructor

```cpp
explicit scoped_resource(T&& src, std::function<void(T&&)>&& f = {});
```

Constructs a scoped_resource wrapper around the provided resource.

**Parameters:**
- `src` - R-value reference to the resource to wrap
- `f` - Optional callback function to return resource to pool

**Details:**
- The resource is marked as valid upon construction
- The callback is typically provided by `resource_pool` to automatically return the resource
- The constructor is marked explicit to prevent accidental implicit conversions

**Example:**
```cpp
// Direct construction (rarely used)
MyResource res;
auto wrapped = scoped_resource<MyResource>(
    std::move(res),
    [](MyResource&& r) { /* return to pool */ }
);

// Typical usage via resource_pool
auto wrapped = pool.borrow_from_pool();
```

### Move Constructor

```cpp
scoped_resource(scoped_resource&& src) noexcept;
```

Moves the resource and callback from another wrapper.

**Details:**
- Essential for returning wrapped resources from functions
- The source wrapper is reset to prevent double-return
- Source callback is cleared and validity flag is set to false

**Example:**
```cpp
scoped_resource<MyResource> src = /* ... */;
scoped_resource<MyResource> dst = std::move(src);
// src is now invalid; only dst will return the resource
```

### Move Assignment Operator

```cpp
scoped_resource& operator=(T&& src);
```

Assigns a new resource to this wrapper and marks it as valid.

**Parameters:**
- `src` - R-value reference to the resource to assign

**Returns:** Reference to this scoped_resource

**Details:**
- The previous resource (if any) is discarded without invoking its callback
- The new resource is marked as valid
- The callback is NOT changed by this operation

**Example:**
```cpp
auto resource = pool.borrow_from_pool();
MyResource new_res;
resource = std::move(new_res);  // Replace the resource
```

### Dereference Operator

```cpp
auto operator*() -> T&;
```

Dereferences the wrapped resource.

**Returns:** Reference to the wrapped resource

**Example:**
```cpp
auto resource = pool.borrow_from_pool();
(*resource).doSomething();  // Access via dereference
```

### get Method

```cpp
T& get();
```

Gets a reference to the wrapped resource.

**Returns:** Reference to the wrapped resource

**Example:**
```cpp
auto resource = pool.borrow_from_pool();
resource.get().doSomething();  // Explicit access via get()

// Equivalent to dereference operator
(*resource).doSomething();
```

### Type Conversion Operator

```cpp
explicit operator T&();
```

Explicit conversion to the resource type.

**Returns:** Reference to the wrapped resource

**Example:**
```cpp
auto resource = pool.borrow_from_pool();
auto& ref = static_cast<T&>(resource);  // Explicit conversion
ref.doSomething();
```

### Destructor

```cpp
~scoped_resource();
```

Automatically returns resource to pool if valid.

**Details:**
- Implements the RAII pattern
- If `m_is_valid` is true and `m_putback_callback` exists: returns resource to pool
- If `m_is_valid` is false: resource is discarded (not returned)
- Ensures resources are always properly managed, even if an exception occurs

**Example:**
```cpp
{
    auto resource = pool.borrow_from_pool();
    // Use resource...
}  // Destructor called here; resource returned to pool
```

### invalidate

```cpp
void invalidate();
```

Marks the resource as invalid to prevent automatic return to the pool.

**Details:**
- Call this method when you've moved the resource out or want to prevent automatic return
- After calling this, the destructor will NOT return the resource to the pool
- Safe to call multiple times (subsequent calls have no effect)

**Use Cases:**
- You've moved the resource out and it's no longer valid
- You want to take ownership and prevent automatic return
- You're implementing custom resource management
- The resource has been consumed or transferred elsewhere

**Example:**
```cpp
auto resource = pool.borrow_from_pool();
auto ptr = std::move(*resource);
resource.invalidate();  // Don't return the moved-out resource
// Resource is NOT returned to pool when resource goes out of scope

// Another example: consuming the resource
auto resource = pool.borrow_from_pool();
process_and_consume(*resource);
resource.invalidate();  // Resource was consumed, don't return it
```

---

## Concepts

### NonNumericMoveConstructible

This concept is used by `resource_pool` and `scoped_resource` to enforce proper resource management:

```cpp
template<typename T>
concept NonNumericMoveConstructible = 
    std::move_constructible<T> && !std::is_arithmetic_v<T>;
```

**Requirements:**
1. Type `T` must be move-constructible
2. Type `T` must NOT be an arithmetic type (int, float, double, bool, etc.)

**Rationale:**

The constraint prevents using arithmetic types directly with `resource_pool` because:

- Arithmetic types are cheap to copy and don't benefit from pooling
- Pooling is designed for expensive resources (connections, file handles, buffers)
- The constraint encourages proper resource management patterns
- It prevents accidental misuse of the library

**Valid Types:**

- `std::string` - Strings are non-numeric and move-constructible
- `std::shared_ptr<T>` - Smart pointers for shared ownership
- `std::unique_ptr<T>` - Smart pointers for exclusive ownership
- `std::vector<T>` - Dynamic arrays
- Custom classes and structs
- File handles wrapped in classes
- Database connections
- Network sockets

**Invalid Types:**

- `int`, `float`, `double`, `bool` - Arithmetic types
- `std::array<int, 10>` - Arrays of arithmetic types
- Any type where `std::is_arithmetic_v<T>` is true

**Examples:**

```cpp
// CORRECT: Use std::string instead of int
siddiqsoft::arrp::resource_pool<std::string> pool;
pool.return_to_pool(std::string("resource-1"));

// CORRECT: Use shared_ptr for managed resources
siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;
pool.return_to_pool(std::make_shared<DatabaseConnection>("localhost"));

// INCORRECT: int is arithmetic
// siddiqsoft::arrp::resource_pool<int> pool;  // Compilation error!
// error: constraints not satisfied for class template 'resource_pool'
// because 'int' does not satisfy 'NonNumericMoveConstructible'
```

---

## Common Patterns

### Basic Resource Pool Usage

```cpp
siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;

// Populate pool
for (int i = 0; i < 10; ++i) {
    pool.return_to_pool(std::make_shared<DatabaseConnection>("localhost"));
}

// Use in worker
auto conn = pool.borrow_from_pool();
conn->execute("SELECT * FROM users");
// conn automatically returned to pool when scope exits
```

### Resource Pool with Custom Factory

```cpp
siddiqsoft::arrp::resource_pool<Connection> pool{
    [](auto& p) -> scoped_resource<Connection> {
        return scoped_resource<Connection>(
            Connection::create(),
            [&p](Connection&& conn) { p.return_to_pool(std::move(conn)); }
        );
    }
};

// Resources are created on-demand up to capacity
auto conn = pool.borrow_from_pool();
```

### Resource Invalidation

```cpp
auto resource = pool.borrow_from_pool();
auto ptr = std::move(*resource);
resource.invalidate();  // Don't return the moved-out resource
// Resource is NOT returned to pool
```

---

## Thread Safety

All classes are thread-safe for their public interfaces:

- `borrow_from_pool()` can be called from multiple threads
- `return_to_pool()` can be called from multiple threads
- `size()` is an atomic read
- `clear()` is protected by mutex
- `to_json()` is protected by mutex
- Internal synchronization uses `std::mutex` and `std::scoped_lock`

---

## Performance Considerations

### resource_pool

- Ideal capacity should match `std::thread::hardware_concurrency()`
- Each borrow/return operation acquires a lock
- Resources are stored in a deque for efficient FIFO access
- There is overhead from the `scoped_resource` wrapper
- Use for expensive resources (connections, file handles, buffers)

### scoped_resource

- Minimal overhead for wrapper
- Move operations are efficient
- Callback invocation is fast
- No performance penalty for validity tracking
