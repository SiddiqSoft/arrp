@page quick_reference Quick Reference

@section qr_components Components at a Glance

| Component | Use Case | Thread-Safe | Capacity |
|-----------|----------|-------------|----------|
| `resource_pool` | Resource management | Yes | Configurable |
| `scoped_resource` | RAII wrapper | N/A | N/A |

@section qr_includes Include Files

```cpp
#include "siddiqsoft/resource_pool.hpp"      // Resource pool
```

@section qr_basic_patterns Basic Patterns

@subsection qr_pattern_basic Basic Pool Usage

```cpp
siddiqsoft::arrp::resource_pool<std::shared_ptr<Resource>> pool;

// Populate
for (int i = 0; i < 10; ++i) {
    pool.checkin(std::make_shared<Resource>());
}

// Use
auto res = pool.checkout();
res->doSomething();
// Automatically returned
```

@subsection qr_pattern_factory Custom Factory

```cpp
siddiqsoft::arrp::resource_pool<Resource> pool{
    [](auto& p) -> siddiqsoft::arrp::scoped_resource<Resource> {
        return siddiqsoft::arrp::scoped_resource<Resource>(
            Resource::create(),
            [&p](Resource&& r) { p.checkin(std::move(r)); }
        );
    }
};
```

@subsection qr_pattern_multithreaded Multi-threaded Usage

```cpp
std::vector<std::thread> threads;
for (int t = 0; t < 4; ++t) {
    threads.emplace_back([&pool]() {
        auto res = pool.checkout();
        // Use resource
    });
}
for (auto& t : threads) t.join();
```

@subsection qr_pattern_invalidate Resource Invalidation

```cpp
auto res = pool.checkout();
auto ptr = std::move(*res);
res.invalidate();  // Don't return
```

@section qr_template_params Template Parameters

### resource_pool<T, SRT, InitCapacity>
- `T`: Resource type (must satisfy NonNumericMoveConstructible)
- `SRT`: Resource wrapper type (default: scoped_resource<T>)
- `InitCapacity`: Initial capacity (default: 8, max: 128)

### scoped_resource<T>
- `T`: Resource type (must satisfy NonNumericMoveConstructible)

@section qr_methods Common Methods

### checkout()
Borrow a resource from the pool
```cpp
auto resource = pool.checkout();  // throws if empty and at capacity
```

### checkin(T&&)
Return a resource to the pool
```cpp
pool.checkin(std::move(resource));
```

### size()
Get current pool size
```cpp
auto sz = pool.size();
```

### clear()
Clear all resources from pool
```cpp
pool.clear();
```

### to_json()
Get JSON representation (requires nlohmann/json)
```cpp
auto json = pool.to_json();
std::cout << json.dump(2) << std::endl;
```

### invalidate()
Prevent resource from being returned
```cpp
resource.invalidate();
```

@section qr_requirements Requirements

- **C++20** or later
- **Compiler**: GCC 10+, MSVC 16.11+, Clang 10+
- **Platform**: Windows, Linux, macOS
- **Dependencies**: None (header-only for core functionality)
- **Optional**: nlohmann/json for JSON serialization

@section qr_compilation Compilation

**GCC/Clang:**
```bash
g++ -std=c++20 -pthread your_file.cpp
clang++ -std=c++20 -fexperimental-library -pthread your_file.cpp
```

**MSVC:**
```bash
cl /std:c++20 your_file.cpp
```

**CMake:**
```cmake
target_link_libraries(your_target PRIVATE arrp::arrp)
```

@section qr_tips Tips & Tricks

1. **Pre-populate the pool** - Add resources before using
2. **Use move semantics** - Always move resources into the pool
3. **Handle exceptions** - Catch std::runtime_error from checkout()
4. **Monitor pool state** - Use to_json() to check diagnostics
5. **Set capacity appropriately** - Match your thread pool size
6. **Use shared_ptr** - For shared resource ownership
7. **Lifetime management** - Keep pool alive while borrowing
8. **Resource invalidation** - Use when moving resources out

@section qr_constraints Type Constraints

### NonNumericMoveConstructible Concept

The `resource_pool` and `scoped_resource` require types that satisfy the `NonNumericMoveConstructible` concept:

```cpp
template<typename T>
concept NonNumericMoveConstructible = 
    std::move_constructible<T> && !std::is_arithmetic_v<T>;
```

**Valid types for resource_pool:**
- `std::string`
- `std::shared_ptr<T>` (where T is non-numeric)
- `std::unique_ptr<T>` (where T is non-numeric)
- `std::vector<T>`
- Custom classes and structs
- File handles wrapped in classes
- Database connections

**Invalid types for resource_pool:**
- `int`, `float`, `double`, `bool` (arithmetic types)
- Use `std::string` or wrapper classes instead

**Why this constraint?**
- Arithmetic types are cheap to copy and don't benefit from pooling
- Pooling is designed for expensive resources
- The constraint prevents accidental misuse

@section qr_troubleshooting Common Issues

| Issue | Solution |
|-------|----------|
| Compilation error: `deque not found` | Use C++20 or later |
| `checkout()` throws | Pool is empty and at capacity |
| Resources not returned | Ensure scoped_resource goes out of scope |
| Deadlock | Avoid circular dependencies |
| Memory leak | Ensure proper RAII cleanup |
| `resource_pool<int>` error | Use `std::string` or wrapper class |
| Clang compilation fails | Add `-fexperimental-library` flag |
| High memory usage | Reduce pool capacity or clear unused resources |

@section qr_performance Performance Tips

- **Capacity**: Set to match thread pool size
- **Pre-populate**: Add resources before heavy usage
- **Monitor**: Use to_json() to track pool state
- **Avoid blocking**: Keep resource usage quick
- **Use shared_ptr**: For shared ownership patterns
- **Batch operations**: Reduce borrow/return overhead

@section qr_examples Quick Examples

### Example 1: Basic Pool
```cpp
siddiqsoft::arrp::resource_pool<std::shared_ptr<Connection>> pool;
pool.checkin(std::make_shared<Connection>());
auto conn = pool.checkout();
conn->query("SELECT * FROM users");
```

### Example 2: Custom Factory
```cpp
siddiqsoft::arrp::resource_pool<Connection> pool{
    [](auto& p) -> siddiqsoft::arrp::scoped_resource<Connection> {
        return siddiqsoft::arrp::scoped_resource<Connection>(
            Connection::create(),
            [&p](Connection&& c) { p.checkin(std::move(c)); }
        );
    }
};
```

### Example 3: Multi-threaded
```cpp
std::thread t1([&pool]() { auto res = pool.checkout(); });
std::thread t2([&pool]() { auto res = pool.checkout(); });
t1.join(); t2.join();
```

### Example 4: Error Handling
```cpp
try {
    auto res = pool.checkout();
} catch (const std::runtime_error& e) {
    std::cerr << "Pool exhausted: " << e.what() << std::endl;
}
```

### Example 5: Monitoring
```cpp
auto state = pool.to_json();
std::cout << "Available: " << state["size"] << std::endl;
std::cout << "Checked out: " << state["checkedout"] << std::endl;
```

@section qr_json_output JSON Output Format

When calling `to_json()` on a resource pool, the output includes:

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
- `_typver`: Version identifier
- `capacity`: Maximum capacity
- `size`: Available resources in pool
- `load`: Total resources (in pool + checked out)
- `invalidated`: Number of invalidated resources
- `checkedout`: Number of checked-out resources
- `counters.autoreturns`: Automatic resource returns (via default factory callback)
- `counters.newitems`: On-demand resource creations via factory callback
- `counters.return`: Return operations
- `counters.borrow`: Borrow operations
