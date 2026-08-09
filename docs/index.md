# arrp - Auto Returning Resource Pool

<div class="badge-container">
  <a href="https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=33&branchName=master">
    <img src="https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status%2FSiddiqSoft.arrp?branchName=master" alt="Build Status" />
  </a>
  <a href="https://github.com/SiddiqSoft/arrp/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/License-BSD_3--Clause-blue.svg" alt="License" />
  </a>
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue.svg" alt="C++23 Standard" />
  <img src="https://img.shields.io/badge/Header--Only-Yes-green.svg" alt="Header Only" />
  <a href="https://www.nuget.org/packages/SiddiqSoft.arrp">
    <img src="https://img.shields.io/nuget/v/SiddiqSoft.arrp" alt="nuget" />
  </a>
  <img src="https://img.shields.io/github/v/tag/SiddiqSoft/arrp" alt="version" />
  <img src="https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/33" alt="tests" />
</div>

**arrp** (`Auto Returning Resource Pool`) is a lightweight, thread-safe, header-only C++23 resource pool library. It allows applications to manage and reuse scarce, expensive, or moveable resources seamlessly using RAII semantics.
- Uses std::deque to store objects of type `T`.
- Uses std::mutex to implement thread-safe access to the resources.
- Uses lambdas to return the underlying resource to the std::deque.
- A `resource_pool<T>` owns available resources, while borrowing returns a move-only `resource_guard<T>`.
- Uses RAII via a helper class. When the guard goes out of scope or is destroyed, the borrowed resource is automatically returned to the pool for immediate reuse by other components or threads.

---

## Key Features & Design Goals

<div class="grid">
  <div class="card">
    <h3>RAII Resource Management</h3>
    <p>Resources return to the pool automatically when their <code>resource_guard&lt;T&gt;</code> is destroyed, eliminating leak vectors and manual cleanup logic.</p>
  </div>
  <div class="card">
    <h3>Thread-Safe Pool Storage</h3>
    <p>Borrowing, seeding, clearing, capacity adjustments, and statistics gathering are thread-safe and internally synchronized via mutex locks.</p>
  </div>
  <div class="card">
    <h3>On-Demand Factory Fallback</h3>
    <p>Register a factory callback to create resources on demand via <code>try_borrow_create()</code> whenever the pool runs dry.</p>
  </div>
  <div class="card">
    <h3>JSON Diagnostics & Natvis</h3>
    <p>Integrates with <code>nlohmann/json</code> for detailed runtime pool statistics and includes native Natvis visualizers for Visual Studio and VS Code debugging.</p>
  </div>
</div>

---

## Quick Start Examples

=== "Seeded Pool Borrowing"

    ```cpp
    #include <siddiqsoft/arrp.hpp>
    #include <string>
    #include <iostream>

    int main()
    {
        // Construct a pool with an initial capacity of 8
        siddiqsoft::arrp::resource_pool<std::string> pool {8};

        // Seed available resources into the pool
        pool.seed("connection-1");
        pool.seed("connection-2");

        {
            // Borrow a resource from the pool (FIFO order)
            auto resource = pool.try_borrow();
            if (resource) {
                resource->append(" [active]");
                std::cout << "Using resource: " << *resource << '\n';
            }
        } // 'resource' goes out of scope here; connection-1 returns to the pool automatically!

        return 0;
    }
    ```

=== "On-Demand Factory Creation"

    ```cpp
    #include <siddiqsoft/arrp.hpp>
    #include <memory>

    struct DatabaseConnection {
        void query(const char* sql) {}
    };

    int main()
    {
        siddiqsoft::arrp::resource_pool<std::unique_ptr<DatabaseConnection>> pool {4};

        // Register a factory callback for creating resources when pool is empty
        pool.set_factory_callback([]() {
            return std::make_unique<DatabaseConnection>();
        });

        // Tries to borrow existing resource, or creates a new one via factory
        auto conn = pool.try_borrow_create();
        if (conn) {
            conn->get()->query("SELECT 1;");
        }

        return 0;
    }
    ```

=== "Waiting with Timeout"

    ```cpp
    #include <siddiqsoft/arrp.hpp>
    #include <chrono>

    using namespace std::chrono_literals;

    void process(siddiqsoft::arrp::resource_pool<std::string>& pool)
    {
        // Wait up to 250 milliseconds for a resource to become available
        auto guard = pool.try_borrow(250ms);
        if (!guard) {
            if (guard.error() == siddiqsoft::arrp::pool_error::Timeout) {
                // Handle timeout gracefully
            }
        }
    }
    ```

---

## System Requirements

| Requirement | Details |
|---|---|
| **C++ Standard** | C++23 compiler (MSVC 2022+, GCC 13+, Clang 16+) |
| **Resource Type (`T`)** | Must satisfy `NonNumericMoveConstructible`: move-constructible, move-assignable, non-arithmetic type. |
| **Optional Dependencies** | [nlohmann/json](https://github.com/nlohmann/json) (required only when JSON statistics are enabled via `to_json()`). |

---

## Documentation Navigation

- **[Features Overview](features/index.md)**: Explore resource lifecycle management, thread-safety, and JSON diagnostics.
- **[Integration Guide](integration/index.md)**: Learn how to add `arrp` using CMake FetchContent, NuGet, or direct headers.
- **[Examples Overview](examples/index.md)**: Explore complete runnable examples in the repository [`examples/`](https://github.com/SiddiqSoft/arrp/tree/master/examples) directory.
- **[API Reference](api/index.md)**: Complete detailed documentation of all classes, methods, enums, and concept constraints.
- **[License](license.md)**: Project license details (BSD 3-Clause).
