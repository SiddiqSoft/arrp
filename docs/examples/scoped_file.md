# Scoped File Example

The **Scoped File** example demonstrates how to use `arrp::resource_pool` to manage C file handles (`std::FILE*`) safely using RAII and move semantics.

Source directory in repository: [`examples/scoped_file/`](https://github.com/SiddiqSoft/arrp/tree/master/examples/scoped_file)  
Source file: [`examples/scoped_file/src/scoped_file.cpp`](https://github.com/SiddiqSoft/arrp/tree/master/examples/scoped_file/src/scoped_file.cpp)

---

## Overview

Working with raw C file handles (`FILE*`) directly in C++ can lead to resource leaks if files are not closed explicitly when exceptions or early returns occur.

By wrapping `FILE*` inside a custom RAII move-only wrapper class (`ScopedFile`) and placing instances into an `arrp::resource_pool<ScopedFile>`, file handles are safely borrowed and automatically returned or closed upon scope exit.

---

## The RAII Wrapper (`ScopedFile`)

```cpp
class ScopedFile
{
    std::FILE*  m_filehandle {nullptr};
    std::string m_filename {};

public:
    ScopedFile() = default;

    ScopedFile(const char* filename, const char* mode)
        : m_filename(filename)
        , m_filehandle(std::fopen(filename, mode))
    {
    }

    // Move semantics: transfer ownership of the file handle
    ScopedFile(ScopedFile&& other) noexcept
        : m_filehandle(other.m_filehandle)
        , m_filename(std::move(other.m_filename))
    {
        other.m_filehandle = nullptr;
    }

    ScopedFile& operator=(ScopedFile&& other) noexcept
    {
        if (this != &other) {
            if (m_filehandle != nullptr) {
                std::fflush(m_filehandle);
                fclose(m_filehandle);
            }
            m_filehandle       = other.m_filehandle;
            m_filename         = std::move(other.m_filename);
            other.m_filehandle = nullptr;
        }
        return *this;
    }

    // Explicit conversion operator for raw FILE* access
    operator FILE*() const noexcept { return m_filehandle; }

    ~ScopedFile()
    {
        if (m_filehandle != nullptr) {
            std::fflush(m_filehandle);
            fclose(m_filehandle);
            m_filehandle = nullptr;
        }
    }
};
```

---

## Using `ScopedFile` with `arrp::resource_pool`

```cpp
#include <siddiqsoft/arrp.hpp>
#include <print>

int main()
{
    using Pool = siddiqsoft::arrp::resource_pool<ScopedFile>;
    Pool pool {};

    // Seed the pool with an open file handle
    pool.seed("/tmp/example_scoped_file.txt", "w+");

    // Borrow the file handle from the pool
    auto myfile = pool.try_borrow();
    if (myfile.has_value()) {
        fputs("Hello, World!\n", myfile);
        fflush(myfile);
    } // myfile guard goes out of scope; ScopedFile returns to the pool automatically.

    return 0;
}
```

---

## Building and Running

Navigating to the example directory in the repository:

```bash
cd examples/scoped_file
cmake --fresh --preset=Apple-Debug
cmake --build --preset=Apple-Debug
./build/Apple-Debug/scoped_file
```
