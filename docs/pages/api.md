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

### Template Definition

```cpp
template <typename T, typename SRT = scoped_resource<T>, uint8_t InitCapacity = resource_pool_limits::DefaultCapacity>
    requires((InitCapacity <= resource_pool_limits::MaxCapacity)) && 
            NonNumericMoveConstructible<T> &&
            std::derived_from<SRT, scoped_resource<T>>
class resource_pool
{
    // ...
};
```

### Template Parameters

#### T - Resource Type

The resource type that will be managed by the pool.

- **Requirements**: Must satisfy `NonNumericMoveConstructible<T>` concept
- **Move-Constructible**: The type must support move construction (`std::move_constructible<T>`)
- **Non-Arithmetic**: The type must NOT be an arithmetic type (`!std::is_arithmetic_v<T>`)
- **Purpose**: This is the actual resource type that will be pooled and managed
- **Examples of Valid Types**:
  - `std::shared_ptr<Connection>` - Shared pointer to a connection
  - `std::unique_ptr<Resource>` - Unique pointer to a resource
  - `std::string` - String objects
  - `std::vector<T>` - Vector containers
  - Custom classes and structs
  - File handles wrapped in classes
  - Database connections
  - Network sockets

#### SRT - Scoped Resource Wrapper Type

The RAII wrapper type that manages the resource lifecycle.

- **Default**: `scoped_resource<T>`
- **Requirements**: Must derive from `scoped_resource<T>` (checked via `std::derived_from<SRT, scoped_resource<T>>`)
- **Purpose**: Implements RAII pattern for automatic resource return to pool
- **Customization**: Allows custom wrapper implementations that extend `scoped_resource<T>`
- **Responsibility**: The wrapper is responsible for:
  - Storing the resource
  - Managing the return callback
  - Tracking resource validity
  - Implementing the destructor that returns the resource to the pool
- **Example Custom Wrapper**:
  ```cpp
  template<typename T>
  class CustomScopedResource : public scoped_resource<T> {
      // Custom implementation with additional features
      void customMethod() { /* ... */ }
  };
  
  // Use with custom wrapper
  resource_pool<MyResource, CustomScopedResource<MyResource>, 16> pool;
  ```

#### InitCapacity - Initial Pool Capacity

The maximum number of resources that can be in the pool (in pool + checked out).

- **Default**: `resource_pool_limits::DefaultCapacity` (typically 8)
- **Maximum**: `resource_pool_limits::MaxCapacity` (typically 255)
- **Type**: `uint8_t` (unsigned 8-bit integer, range 0-255)
- **Constraint**: Must satisfy `InitCapacity <= resource_pool_limits::MaxCapacity` (compile-time checked)
- **Purpose**: Limits total resources to prevent unbounded growth
- **Examples**:
  ```cpp
  // Pool with default capacity (8)
  resource_pool<MyResource> pool1;
  
  // Pool with custom capacity (16)
  resource_pool<MyResource, scoped_resource<MyResource>, 16> pool2;
  
  // Pool with maximum capacity (255)
  resource_pool<MyResource, scoped_resource<MyResource>, 255> pool3;
  
  // Compile error: capacity exceeds maximum
  // resource_pool<MyResource, scoped_resource<MyResource>, 256> pool4;
  ```

### Template Constraints (Requires Clause)

The template has three compile-time constraints that are enforced by the `requires` clause:

#### 1. Capacity Constraint

```cpp
InitCapacity <= resource_pool_limits::MaxCapacity
```

- **Purpose**: Ensures the initial capacity does not exceed the maximum allowed capacity
- **Benefit**: Prevents accidental creation of pools with invalid capacity
- **Checked**: At compile-time
- **Example Error**:
  ```cpp
  // Compile error: InitCapacity (256) exceeds MaxCapacity (255)
  resource_pool<MyResource, scoped_resource<MyResource>, 256> pool;
  // error: constraints not satisfied for class template 'resource_pool'
  ```

#### 2. Resource Type Constraint

```cpp
NonNumericMoveConstructible<T>
```

- **Purpose**: Ensures the resource type is move-constructible and non-arithmetic
- **Benefit**: Prevents pooling of cheap-to-copy types like int, float, double, bool
- **Checked**: At compile-time
- **Example Error**:
  ```cpp
  // Compile error: int does not satisfy NonNumericMoveConstructible
  resource_pool<int> pool;
  // error: constraints not satisfied for class template 'resource_pool'
  // because 'int' does not satisfy 'NonNumericMoveConstructible'
  ```

#### 3. Wrapper Type Constraint

```cpp
std::derived_from<SRT, scoped_resource<T>>
```

- **Purpose**: Ensures the wrapper type derives from `scoped_resource<T>`
- **Benefit**: Allows custom wrapper implementations while ensuring RAII semantics
- **Checked**: At compile-time
- **Example Error**:
  ```cpp
  // Compile error: CustomWrapper does not derive from scoped_resource<MyResource>
  resource_pool<MyResource, CustomWrapper> pool;
  // error: constraints not satisfied for class template 'resource_pool'
  ```

### Key Features

- **Thread-Safe**: All operations are protected by mutexes for concurrent access
- **RAII Pattern**: Resources are automatically returned via scoped_resource destructors
- **Capacity Management**: Enforces a maximum capacity limit to prevent unbounded growth
- **FIFO Ordering**: Resources are retrieved from the front and added to the back
- **Customizable Factory**: Supports custom resource creation callbacks
- **Diagnostic Counters**: Tracks borrow, return, and auto-add operations
- **JSON Serialization**: Provides pool state diagnostics via JSON (when nlohmann/json is available)

### Constructors

#### Constructor with Custom Factory

```cpp
resource_pool(std::function<SRT(resource_pool&)>&& new_resource_callback);
```

Creates a resource pool with a custom resource factory callback.

**Parameters:**
- `new_resource_callback` - Callback function to create new resources. Must not be empty.

**Details:**
- The callback is stored and called later when resources are needed
- The callback is invoked outside the lock to minimize contention
- If an empty callback is provided, defaults to `CallbackDoNotAutoAddResource`

**Example:**
```cpp
// Using custom factory
auto pool = siddiqsoft::arrp::resource_pool<MyResource>(
    [](auto& p) -> scoped_resource<MyResource> {
        return scoped_resource<MyResource>(
            MyResource::create(),
            [&p](MyResource&& res) { p.checkin(std::move(res)); }
        );
    }
);
```

#### Constructor with Auto-Add Policy

```cpp
resource_pool(auto_add_policy add_policy = auto_add_policy::NoGrow);
```

Creates a resource pool with specified auto-add policy.

**Parameters:**
- `add_policy` - Policy for automatic resource creation:
  - `NoGrow` (default): Pool does not grow; throws when empty
  - `AutoGrow`: Pool automatically creates resources up to capacity
  - `Custom`: User provides custom factory via other constructor

**Details:**
- `NoGrow`: Uses `CallbackDoNotAutoAddResource` which throws `std::runtime_error`
- `AutoGrow`: Creates default-constructed resources and wires up auto-return callback
- Resources are created on-demand up to the capacity limit

**Example:**
```cpp
// Using default factory with auto-grow
auto pool = siddiqsoft::arrp::resource_pool<MyResource>(
    siddiqsoft::arrp::resource_pool<MyResource>::auto_add_policy::AutoGrow
);

// Using default factory without auto-grow
auto pool2 = siddiqsoft::arrp::resource_pool<MyResource>();
```

### Methods

#### size

```cpp
[[nodiscard]] size_t size() const;
```

Returns the current number of available resources in the pool.

**Returns:** Number of resources currently in the pool (not including checked-out resources)

**Thread Safety:** Thread-safe. Protected by mutex.

**Details:**
- Prevents TOCTOU (Time-of-Check-Time-of-Use) race conditions
- Returns a snapshot at the moment of the call
- Size may change immediately after due to concurrent access
- Capacity is limited to 255 resources (uint8_t)

**Example:**
```cpp
auto available = pool.size();
std::cout << "Available resources: " << available << std::endl;
```

#### checkout

```cpp
[[nodiscard]] auto checkout() -> SRT;
```

Borrows a resource from the pool, wrapping it in a `scoped_resource` that automatically returns it when destroyed.

**Returns:** A `scoped_resource<T>` (or custom SRT) containing the borrowed resource

**Throws:** `std::runtime_error` if unable to obtain a resource (pool at capacity and no factory callback available)

**Thread Safety:** Thread-safe. Protected by mutex. Resource creation happens outside the lock to minimize contention.

**Algorithm:**
1. If the pool is non-empty, return the first resource (FIFO)
2. If the pool is empty but under capacity, create a new resource via factory
3. If at capacity and no resources available, throw `std::runtime_error`

**Details:**
- Increments the borrow counter only on successful borrow
- Resources are returned in FIFO order
- The resource is marked as valid upon return
- Uses `unique_lock` to allow resource creation outside the critical section
- Increments `m_resources_checkedout` counter

**Example:**
```cpp
try {
    auto resource = pool.checkout();
    // Use the resource...
    resource->doSomething();
    // Automatically returned when resource goes out of scope
} catch (const std::runtime_error& e) {
    std::cerr << "Failed to borrow resource: " << e.what() << std::endl;
}
```

#### checkin

```cpp
void checkin(T&& raw_resource);
```

Returns a resource to the pool, making it available for future checkout operations.

**Parameters:**
- `raw_resource` - R-value reference to the resource to return to the pool

**Thread Safety:** Thread-safe. Protected by mutex.

**Details:**
- Adds resource to the back of the deque (FIFO ordering)
- Decrements the checked-out counter
- Increments the return counter
- This method is typically called automatically by the `scoped_resource` destructor
- Can be called manually for advanced use cases
- Only valid resources should be checked in

**Example:**
```cpp
// Typically called automatically:
{
    auto resource = pool.checkout();
    // Use resource...
}  // Automatically returned here

// Manual return (advanced usage):
auto resource = pool.checkout();
// ... use resource ...
pool.checkin(std::move(*resource));
```

#### clear

```cpp
void clear();
```

Removes and destroys all resources currently in the pool.

**Thread Safety:** Thread-safe. Protected by mutex.

**Details:**
- Removes all resources from the internal deque
- All resources are destroyed
- Any checked-out resources are NOT affected
- Safe to call on an empty pool
- This operation does not affect the capacity limit

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

**Availability:** Only available when `NLOHMANN_JSON_VERSION_MAJOR` is defined (nlohmann/json is included)

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
- `invalidated`: Number of invalidated resources (reserved for future use)
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

### Static Members

#### CallbackDoNotAutoAddResource

```cpp
static inline std::function<SRT(resource_pool&)> CallbackDoNotAutoAddResource;
```

Default callback that throws when the pool is empty and cannot grow.

**Behavior:** Throws `std::runtime_error` with message "No items in the pool; add something first."

**Usage:** Used internally when `auto_add_policy::NoGrow` is specified

### Constraints and Limitations

- **Not copy-constructible or move-constructible**: Pool instances cannot be copied or moved
- **Not copy-assignable or move-assignable**: Pool instances cannot be assigned
- **Capacity limit**: Maximum 255 resources (uint8_t)
- **Resource requirements**: Resources must be move-constructible and non-arithmetic types
- **Factory callback restrictions**: MUST NOT call any pool methods to avoid deadlock

### Thread Safety

All public methods are thread-safe:
- `checkout()` can be called from multiple threads
- `checkin()` can be called from multiple threads
- `size()` is an atomic read
- `clear()` is protected by mutex
- `to_json()` is protected by mutex
- Internal synchronization uses `std::mutex` (or `std::recursive_mutex` if `ARRP_USE_RECURSIVE_MUTEX` is defined)

---

## scoped_resource

RAII wrapper for managing resource lifecycle with automatic return to pool.

### Overview

The `scoped_resource` class implements the Resource Acquisition Is Initialization (RAII) pattern for managing resources that should be returned to a resource pool. It wraps a resource and automatically invokes a callback when the wrapper is destroyed, enabling automatic resource management without manual cleanup.

### Template Definition

```cpp
template <typename T>
    requires NonNumericMoveConstructible<T>
class scoped_resource
{
    // ...
};
```

### Template Parameters

#### T - Resource Type

The resource type to wrap.

- **Requirements**: Must satisfy `NonNumericMoveConstructible<T>` concept
- **Move-Constructible**: The type must support move construction
- **Non-Arithmetic**: The type must NOT be an arithmetic type
- **Purpose**: This is the actual resource type that will be wrapped and managed

### Key Features

- **RAII Pattern**: Automatically returns resources via destructor
- **Move Semantics**: Supports move construction and move assignment for efficient transfers
- **Validity Tracking**: Tracks whether a resource should be returned to the pool
- **Callback Support**: Executes custom callback on destruction
- **Debug Support**: Includes debug identifiers for tracking in DEBUG builds
- **Dereference Access**: Provides operator*, get() method, and type conversion for resource access
- **Invalidation**: Allows explicit invalidation to prevent automatic return

### Constructors

#### Explicit Constructor

```cpp
explicit scoped_resource(T&& src, std::function<void(T&&)>&& f = {});
```

Constructs a scoped_resource wrapper around the provided resource.

**Parameters:**
- `src` - R-value reference to the resource to wrap
- `f` - Optional callback function to return resource to pool (default: empty)

**Details:**
- The resource is marked as valid upon construction
- The callback is typically provided by `resource_pool` to automatically return the resource
- The constructor is marked explicit to prevent accidental implicit conversions
- If no callback is provided, the resource is simply destroyed on scope exit

**Example:**
```cpp
// Direct construction (rarely used)
MyResource res;
auto wrapped = scoped_resource<MyResource>(
    std::move(res),
    [](MyResource&& r) { /* return to pool */ }
);

// Typical usage via resource_pool
auto wrapped = pool.checkout();
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
- Enables efficient transfer of ownership

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
- Useful for replacing resources within the same wrapper

**Example:**
```cpp
auto resource = pool.checkout();
MyResource new_res;
resource = std::move(new_res);  // Replace the resource
```

### Access Methods

#### Dereference Operator

```cpp
auto operator*() -> T&;
```

Dereferences the wrapped resource.

**Returns:** Reference to the wrapped resource

**Example:**
```cpp
auto resource = pool.checkout();
(*resource).doSomething();  // Access via dereference
```

#### Type Conversion Operator

```cpp
explicit operator T&();
```

Explicit conversion to the resource type.

**Returns:** Reference to the wrapped resource

**Example:**
```cpp
auto resource = pool.checkout();
auto& ref = static_cast<T&>(resource);  // Explicit conversion
ref.doSomething();
```

### Lifecycle Methods

#### Destructor

```cpp
~scoped_resource();
```

Automatically returns resource to pool if valid.

**Details:**
- Implements the RAII pattern
- If `m_is_valid` is true and `m_putback_callback` exists: returns resource to pool
- If `m_is_valid` is false: resource is discarded (not returned)
- Ensures resources are always properly managed, even if an exception occurs
- Safe to call even if the resource has been moved out

**Example:**
```cpp
{
    auto resource = pool.checkout();
    // Use resource...
}  // Destructor called here; resource returned to pool
```

#### invalidate

```cpp
void invalidate();
```

Marks the resource as invalid to prevent automatic return to the pool.

**Details:**
- Call this method when you've moved the resource out or want to prevent automatic return
- After calling this, the destructor will NOT return the resource to the pool
- Safe to call multiple times (subsequent calls have no effect)
- Useful for taking ownership of the resource

**Use Cases:**
- You've moved the resource out and it's no longer valid
- You want to take ownership and prevent automatic return
- You're implementing custom resource management
- The resource has been consumed or transferred elsewhere

**Example:**
```cpp
auto resource = pool.checkout();
auto ptr = std::move(*resource);
resource.invalidate();  // Don't return the moved-out resource
// Resource is NOT returned to pool when resource goes out of scope

// Another example: consuming the resource
auto resource = pool.checkout();
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
pool.checkin(std::string("resource-1"));

// CORRECT: Use shared_ptr for managed resources
siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;
pool.checkin(std::make_shared<DatabaseConnection>("localhost"));

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
    pool.checkin(std::make_shared<DatabaseConnection>("localhost"));
}

// Use in worker
auto conn = pool.checkout();
conn->execute("SELECT * FROM users");
// conn automatically returned to pool when scope exits
```

### Resource Pool with Custom Factory

```cpp
siddiqsoft::arrp::resource_pool<Connection> pool{
    [](auto& p) -> scoped_resource<Connection> {
        return scoped_resource<Connection>(
            Connection::create(),
            [&p](Connection&& conn) { p.checkin(std::move(conn)); }
        );
    }
};

// Resources are created on-demand up to capacity
auto conn = pool.checkout();
```

### Resource Pool with Auto-Grow Policy

```cpp
// Pool automatically creates resources up to capacity
auto pool = siddiqsoft::arrp::resource_pool<MyResource>(
    siddiqsoft::arrp::resource_pool<MyResource>::auto_add_policy::AutoGrow
);

// Resources are created on-demand
auto resource = pool.checkout();
```

### Resource Invalidation

```cpp
auto resource = pool.checkout();
auto ptr = std::move(*resource);
resource.invalidate();  // Don't return the moved-out resource
// Resource is NOT returned to pool
```

### Monitoring Pool State

```cpp
auto state = pool.to_json();
std::cout << "Pool state: " << state.dump(2) << std::endl;

// Check specific metrics
std::cout << "Available: " << state["size"] << std::endl;
std::cout << "Checked out: " << state["checkedout"] << std::endl;
std::cout << "Total borrows: " << state["counters"]["borrow"] << std::endl;
```

---

## Thread Safety

All classes are thread-safe for their public interfaces:

- `checkout()` can be called from multiple threads
- `checkin()` can be called from multiple threads
- `size()` is an atomic read
- `clear()` is protected by mutex
- `to_json()` is protected by mutex
- Internal synchronization uses `std::mutex` (or `std::recursive_mutex` if `ARRP_USE_RECURSIVE_MUTEX` is defined)

---

## Performance Considerations

### resource_pool

- Ideal capacity should match `std::thread::hardware_concurrency()`
- Each borrow/return operation acquires a lock
- Resources are stored in a deque for efficient FIFO access
- There is overhead from the `scoped_resource` wrapper
- Use for expensive resources (connections, file handles, buffers)
- Resource creation happens outside the lock to minimize contention
- Counters use uint64_t and will wrap around after ~18 quintillion operations

### scoped_resource

- Minimal overhead for wrapper
- Move operations are efficient
- Callback invocation is fast
- No performance penalty for validity tracking
- Supports compiler optimizations like NRVO (Named Return Value Optimization)

---

## Compiler Support

- **GCC**: 10+ (Linux)
- **Clang**: 
  - AppleClang v21+
  - LLVM v20+
- **MSVC**: 
  - Visual Studio 2019
  - Visual Studio 2022
  - Visual Studio 2026

**Language Standard:** C++20 minimum required

---

## Mutex Configuration

By default, `resource_pool` uses `std::mutex` for optimal performance. For testing or specific use cases, you can enable `std::recursive_mutex` by defining:

```cpp
#define ARRP_USE_RECURSIVE_MUTEX
```

This is useful when tests need to relax some deadlock constraints, but should not be used in production code.
