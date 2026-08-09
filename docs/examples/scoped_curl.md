# Scoped cURL Example

The **Scoped cURL** example demonstrates how to manage expensive `libcurl` easy handles (`CURL*`) in multi-threaded asynchronous workflows using `arrp::resource_pool`, dynamic factory callbacks, timeouts, and JSON statistics diagnostics.

Source directory in repository: [`examples/scoped_curl/`](https://github.com/SiddiqSoft/arrp/tree/master/examples/scoped_curl)  
Source file: [`examples/scoped_curl/src/scoped_curl.cpp`](https://github.com/SiddiqSoft/arrp/tree/master/examples/scoped_curl/src/scoped_curl.cpp)

---

## Overview

Initializing `libcurl` easy handles for network requests can be expensive. Reusing handles across concurrent HTTP requests improves performance while ensuring that active handles are non-shared per request.

This example illustrates:
1. Wrapping `CURL*` in a move-only RAII `ScopedCurl` handle.
2. Registering a factory callback (`set_factory_callback`) to create `ScopedCurl` instances on demand.
3. Using `try_borrow_create(timeout)` with a wait duration to borrow an existing handle or instantiate a new one if unavailable.
4. Exporting pool telemetry via `pool.to_json()`.

---

## The RAII Wrapper (`ScopedCurl`)

```cpp
class ScopedCurl
{
    CURL* m_curlhandle {nullptr};

public:
    ScopedCurl()
        : m_curlhandle(curl_easy_init())
    {
        if (!m_curlhandle) {
            throw std::runtime_error("Failed to initialize curl handle");
        }
    }

    ScopedCurl(const ScopedCurl&) = delete;
    ScopedCurl& operator=(const ScopedCurl&) = delete;

    ScopedCurl(ScopedCurl&& other) noexcept
        : m_curlhandle(other.m_curlhandle)
    {
        other.m_curlhandle = nullptr;
    }

    ScopedCurl& operator=(ScopedCurl&& other) noexcept
    {
        if (this != &other) {
            if (m_curlhandle) {
                curl_easy_cleanup(m_curlhandle);
            }
            m_curlhandle = other.m_curlhandle;
            other.m_curlhandle = nullptr;
        }
        return *this;
    }

    operator CURL*() const noexcept { return m_curlhandle; }

    ~ScopedCurl()
    {
        if (m_curlhandle) {
            curl_easy_cleanup(m_curlhandle);
            m_curlhandle = nullptr;
        }
    }
};
```

---

## Concurrent Requests & Dynamic Creation

```cpp
void do_request(siddiqsoft::arrp::resource_pool<ScopedCurl>& pool, const char* url)
{
    // Try borrowing a resource from the pool, or create a new one using factory callback
    // Wait up to 500ms if pool is busy
    auto sc = pool.try_borrow_create(std::chrono::milliseconds(500));
    if (sc.has_value()) {
        curl_easy_setopt(sc, CURLOPT_URL, url);
        curl_easy_perform(sc);
    } // Handle returned to pool automatically
}

int main()
{
    curl_global_init(CURL_GLOBAL_ALL);

    siddiqsoft::arrp::resource_pool<ScopedCurl> pool {};
    
    // Register factory callback for dynamic handle creation
    pool.set_factory_callback([&] {
        return ScopedCurl {};
    });

    // Launch asynchronous worker threads borrowing handles from pool
    auto f1 = std::async(std::launch::async, do_request, std::ref(pool), "https://www.example.com");
    auto f2 = std::async(std::launch::async, do_request, std::ref(pool), "https://www.duckduckgo.com");

    f1.get();
    f2.get();

    // Print pool diagnostics in JSON format
    std::cout << pool.to_json().dump(2) << std::endl;

    curl_global_cleanup();
    return 0;
}
```

---

## Building and Running

Navigating to the example directory in the repository:

```bash
cd examples/scoped_curl
cmake --fresh --preset=Apple-Debug
cmake --build --preset=Apple-Debug
./build/Apple-Debug/scoped_curl
```
