# arrp

<div class="hero-tagline">Thread-safe resource pool library.</div>

<div class="badge-container" markdown="1">
  <a href="https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=33&branchName=main">
    <img src="https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status%2FSiddiqSoft.arrp?branchName=main" alt="Build Status" />
  </a>
  <a href="https://www.nuget.org/packages/SiddiqSoft.arrp">
    <img src="https://img.shields.io/nuget/v/SiddiqSoft.arrp" alt="Package Version" />
  </a>
  <a href="https://www.nuget.org/packages/SiddiqSoft.arrp">
    <img src="https://img.shields.io/nuget/dt/SiddiqSoft.arrp" alt="Package Downloads" />
  </a>
  <a href="https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=33&branchName=main">
    <img src="https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/33" alt="Test Results" />
  </a>
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue.svg" alt="C++20 Standard" />
  <a href="https://github.com/SiddiqSoft/arrp/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/License-BSD_3--Clause-blue.svg" alt="License" />
  </a>
</div>

<div class="grid" markdown="1">

<div class="feature-col-intro" markdown="1">

**arrp** (`Auto Returning Resource Pool`) is a lightweight, thread-safe, header-only C++20 resource pool library.

- **Pool Storage**: Synchronized access to a collection of available resources.
- **Automated Returns**: Resources return to the pool upon guard destruction.
- **Factory Fallback**: Register a factory callback for creating resources on-the-fly.

</div>

<div class="feature-span-all" markdown="1">

### Usage Examples

=== "Seeded Pool Borrowing"

    ```cpp
    #include <siddiqsoft/arrp.hpp>
    #include <string>
    #include <iostream>

    int main() {
        siddiqsoft::arrp::resource_pool<std::string> pool {8};

        pool.seed("connection-1");
        pool.seed("connection-2");

        {
            auto resource = pool.try_borrow();
            if (resource) {
                resource->append(" [active]");
                std::cout << "Using resource: " << *resource << '\n';
            }
        }
        
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

    int main() {
        siddiqsoft::arrp::resource_pool<std::unique_ptr<DatabaseConnection>> pool {4};

        pool.set_factory_callback([]() {
            return std::make_unique<DatabaseConnection>();
        });

        auto conn = pool.try_borrow_create();
        if (conn) {
            conn->get()->query("SELECT 1;");
        }

        return 0;
    }
    ```

</div>
</div>

## Documentation Sections

<div class="grid" markdown="1">

<div class="card" markdown="1">

### [Getting Started](quickstart/index.md)

Integration instructions for CMake FetchContent, and NuGet. Includes system requirements and dependencies breakdown.

[Go to Getting Started :octicons-arrow-right-24:](quickstart/index.md)

</div>

<div class="card" markdown="1">

### [API Reference](api/index.md)

Complete API reference for `siddiqsoft::arrp::resource_pool` and `siddiqsoft::arrp::resource_guard`.

[Go to API Reference :octicons-arrow-right-24:](api/index.md)

</div>

<div class="card" markdown="1">

### [Related Pages](architecture/index.md)

Architecture overviews, threading mechanics, and maintainer guidelines.

[Go to Related Pages :octicons-arrow-right-24:](architecture/index.md)

</div>

</div>
