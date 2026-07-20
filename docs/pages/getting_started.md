@page getting_started Getting Started

@section installation Installation

### Using CMake (Recommended)

Add the library to your CMakeLists.txt:

```cmake
include(FetchContent)
FetchContent_Declare(arrp
    GIT_REPOSITORY https://github.com/SiddiqSoft/arrp.git
    GIT_TAG main
)
FetchContent_MakeAvailable(arrp)

target_link_libraries(your_target PRIVATE arrp::arrp)
```

### Using NuGet (Windows)

```bash
nuget install SiddiqSoft.aarp
```

@section compiler_setup Compiler Setup

#### Visual Studio 2019 or later

- Set C++ Language Standard to `/std:c++20` or `/std:c++latest`
- No additional flags required

#### GCC 10+

```bash
g++ -std=c++20 -pthread your_file.cpp
```

#### Clang 10+

```bash
clang++ -std=c++20 -fexperimental-library -pthread your_file.cpp
```

@section first_program Your First Program

Create a simple program that uses the arrp library:

```cpp
#include <iostream>
#include <memory>
#include "siddiqsoft/resource_pool.hpp"

class DatabaseConnection {
public:
    void execute(const std::string& query) {
        std::cout << "Executing: " << query << std::endl;
    }
};

int main() {
    // Create a resource pool
    siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;

    // Populate the pool with resources
    for (int i = 0; i < 5; ++i) {
        pool.checkin(std::make_shared<DatabaseConnection>());
    }

    // Borrow a resource from the pool
    {
        auto conn = pool.checkout();
        conn->execute("SELECT * FROM users");
        // Resource is automatically returned to the pool when going out of scope
    }

    std::cout << "Done!" << std::endl;
    return 0;
}
```

@section common_patterns Common Patterns

### Pattern 1: Basic Resource Borrowing

Borrow a resource, use it, and let it automatically return:

```cpp
siddiqsoft::arrp::resource_pool<std::shared_ptr<Connection>> pool;

{
    auto conn = pool.checkout();
    conn->query("SELECT * FROM users");
    // Automatically returned when going out of scope
}
```

### Pattern 2: Custom Resource Factory

Create resources on-demand with a custom factory:

```cpp
siddiqsoft::arrp::resource_pool<Connection> pool{
    [](auto& p) -> siddiqsoft::arrp::scoped_resource<Connection> {
        return siddiqsoft::arrp::scoped_resource<Connection>(
            Connection::create(),
            [&p](Connection&& conn) { p.checkin(std::move(conn)); }
        );
    }
};

auto conn = pool.checkout();
```

### Pattern 3: Multi-threaded Usage

Use the pool safely from multiple threads:

```cpp
siddiqsoft::arrp::resource_pool<std::shared_ptr<Connection>> pool;

std::thread t1([&pool]() {
    auto conn = pool.checkout();
    // Use connection
});

std::thread t2([&pool]() {
    auto conn = pool.checkout();
    // Use connection
});

t1.join();
t2.join();
```

### Pattern 4: Resource Invalidation

Prevent a resource from being returned to the pool:

```cpp
auto resource = pool.checkout();
auto ptr = std::move(*resource);
resource.invalidate();  // Don't return the moved-out resource
// Resource is NOT returned to pool
```

@section troubleshooting Troubleshooting

### Compilation Errors

**Error**: `'deque' is not a member of 'std'`
- **Solution**: Ensure you're using C++20 or later. Update your compiler flags to `-std=c++20` or `/std:c++20`.

**Error**: `undefined reference to pthread_*`
- **Solution**: Link against pthread library: `-pthread` flag or `target_link_libraries(... pthread)`

**Error**: `'concepts' is not a member of 'std'`
- **Solution**: Ensure your compiler supports C++20. Update to GCC 10+, MSVC 16.11+, or Clang 10+.

**Error**: `resource_pool<int>` compilation error
- **Solution**: The resource_pool requires non-arithmetic types. Use `std::string` or a wrapper class instead.

### Runtime Issues

**Issue**: `checkout()` throws std::runtime_error
- **Solution**: The pool is empty and at capacity. Populate the pool first or increase capacity.
- **Solution**: Ensure resources are being returned to the pool properly.

**Issue**: Resources not being returned
- **Solution**: Ensure the scoped_resource wrapper is going out of scope
- **Solution**: Check that you're not calling `invalidate()` unintentionally

**Issue**: Deadlock or hanging
- **Solution**: Ensure callbacks don't block indefinitely
- **Solution**: Avoid circular dependencies between pools

**Issue**: Memory leak
- **Solution**: Ensure scoped_resource wrappers are properly destroyed
- **Solution**: Check that resources are being returned to the pool

@section next_steps Next Steps

- Read the @ref usage_guide for detailed usage examples
- Check the @ref quick_reference for API quick lookup
- Explore the @ref examples for more complex scenarios
- Review the @ref api for complete API documentation
