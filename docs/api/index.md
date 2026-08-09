# API Reference Overview

All public types and concepts in `arrp` reside within the namespace `siddiqsoft::arrp`.

Include the complete public API with:

```cpp
#include <siddiqsoft/arrp.hpp>
```

---

## Core API Elements

<div class="grid">
  <div class="card">
    <h3><a href="resource_pool/">resource_pool&lt;T&gt;</a></h3>
    <p>Thread-safe resource container template managing available resources, factory callbacks, timeouts, and JSON metrics.</p>
  </div>
  <div class="card">
    <h3><a href="resource_guard/">resource_guard&lt;T&gt;</a></h3>
    <p>Move-only RAII handle returned by borrowing. Automatically returns the resource on scope exit or discards it if invalidated.</p>
  </div>
  <div class="card">
    <h3><a href="types/">Types & Enumerations</a></h3>
    <p>Capacity limits, error codes (<code>pool_error</code>), release reasons (<code>release_reason</code>), and <code>std::formatter</code> specializations.</p>
  </div>
</div>

---

## Type Constraints: `NonNumericMoveConstructible` Concept

`resource_pool<T>` and `resource_guard<T>` require that `T` satisfies the concept `NonNumericMoveConstructible`.

```cpp
template <typename T>
concept NonNumericMoveConstructible = 
    std::move_constructible<T> && 
    std::is_move_assignable_v<T> && 
    !std::is_arithmetic_v<T>;
```

### Type Requirements

1. **Move Constructible**: `T` must be movable (`std::move_constructible<T>`).
2. **Move Assignable**: `T` must be move assignable (`std::is_move_assignable_v<T>`).
3. **Non-Arithmetic**: `T` cannot be a primitive numeric or boolean type (`!std::is_arithmetic_v<T>`).

Suitable resource types include smart pointers (`std::unique_ptr`, `std::shared_ptr`), container objects (`std::string`, `std::vector`), network sockets, file handles, and custom database connection classes.
