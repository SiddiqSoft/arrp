# Resource Management Mechanics

`arrp` decouples resource ownership from resource access. The `resource_pool<T>` manages the collection of available resources, while `resource_guard<T>` is a temporary RAII ownership token given to caller threads.

---

## Borrowing Mechanics & Ordering

Resources are stored internally in a FIFO queue (`std::deque<T>`). When a caller invokes `try_borrow()`, the pool removes the oldest available resource and wraps it in a `resource_guard<T>`.

```cpp
siddiqsoft::arrp::resource_pool<std::string> pool {4};
pool.seed("res-1");
pool.seed("res-2");

// Borrows "res-1" first
auto g1 = pool.try_borrow(); 

// Borrows "res-2" second
auto g2 = pool.try_borrow(); 
```

---

## Seeding Pre-allocated Resources

You can populate the pool by seeding items either by moving an existing instance or using in-place constructor arguments:

```cpp
siddiqsoft::arrp::resource_pool<std::string> pool {8};

// 1. Move-seed an existing object
std::string my_str = "initial-value";
pool.seed(std::move(my_str));

// 2. In-place construction seed
pool.seed(10, 'A'); // Constructs std::string(10, 'A') -> "AAAAAAAAAA"
```

---

## On-Demand Factory Creation

When a pool is empty, `try_borrow()` returns an invalid guard containing `pool_error::NoMoreResources`. To create resources dynamically on demand when the pool is exhausted, register a factory callback:

```cpp
siddiqsoft::arrp::resource_pool<std::string> pool {8};

pool.set_factory_callback([]() {
    return std::string {"created-by-factory"};
});

// try_borrow_create() checks the pool first.
// If empty, it calls the factory callback automatically.
auto resource = pool.try_borrow_create();
```

> [!NOTE]
> `try_borrow()` **never** calls the factory callback; only `try_borrow_create()` does. The factory callback must take no parameters and return `T` (or `resource_guard<T>`). It must not perform recursive operations on the same pool instance.

---

## Discarding and Invalidating Resources

If a resource encounters an unrecoverable error during use (such as a disconnected database handle or network failure), or if its contents are moved out, it should **not** be returned to the pool.

Call `invalidate()` on the guard, or move the resource out using rvalue cast:

=== "Calling invalidate()"

    ```cpp
    auto guard = pool.try_borrow();
    if (guard) {
        if (!guard->is_healthy()) {
            // Mark as abandoned; destructor will discard it instead of returning it
            guard.invalidate(); 
        }
    }
    ```

=== "Moving Out via Rvalue Cast"

    ```cpp
    auto guard = pool.try_borrow();
    if (guard) {
        // Extracting value via rvalue cast invalidates the guard automatically
        std::string raw_string = static_cast<std::string>(std::move(guard));
    }
    ```

---

## Custom Cleanup Callbacks

When constructing a `resource_pool<T>`, you can specify an optional shutdown/cleanup callback. This callback executes for every resource removed when `clear()` is invoked or when the pool destructor runs:

```cpp
siddiqsoft::arrp::resource_pool<FILE*> file_pool {
    4,
    [](FILE*& f) {
        if (f != nullptr) {
            std::fclose(f);
            f = nullptr;
        }
    }
};

file_pool.seed(std::fopen("log.txt", "w"));

// Destroys available FILE* resources using the cleanup callback
file_pool.clear(); 
```
