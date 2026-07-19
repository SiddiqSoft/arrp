@page examples Examples

@section ex_database_connections Database Connection Pool

Manage a pool of database connections:

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <memory>
#include <iostream>

class DatabaseConnection {
private:
    std::string m_host;
    int m_port;
    
public:
    DatabaseConnection(const std::string& host, int port) 
        : m_host(host), m_port(port) {}
    
    void execute(const std::string& query) {
        std::cout << "Executing on " << m_host << ":" << m_port 
                  << " - " << query << std::endl;
    }
};

int main() {
    siddiqsoft::arrp::resource_pool<std::shared_ptr<DatabaseConnection>> pool;
    
    // Populate pool with connections
    for (int i = 0; i < 10; ++i) {
        pool.return_to_pool(
            std::make_shared<DatabaseConnection>("localhost", 5432)
        );
    }

    // Use connections
    {
        auto conn = pool.borrow_from_pool();
        conn->execute("SELECT * FROM users");
        // Automatically returned to pool
    }

    return 0;
}
```

@section ex_http_client HTTP Client Connection Pool

Manage HTTP client connections:

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <memory>
#include <iostream>

class HttpClient {
private:
    std::string m_base_url;
    
public:
    HttpClient(const std::string& base_url) : m_base_url(base_url) {}
    
    std::string get(const std::string& path) {
        std::cout << "GET " << m_base_url << path << std::endl;
        return "response";
    }
    
    std::string post(const std::string& path, const std::string& body) {
        std::cout << "POST " << m_base_url << path << " - " << body << std::endl;
        return "response";
    }
};

int main() {
    siddiqsoft::arrp::resource_pool<std::shared_ptr<HttpClient>> pool;
    
    // Populate pool
    for (int i = 0; i < 5; ++i) {
        pool.return_to_pool(
            std::make_shared<HttpClient>("https://api.example.com")
        );
    }

    // Use clients
    {
        auto client = pool.borrow_from_pool();
        auto response = client->get("/users");
        // Automatically returned to pool
    }

    return 0;
}
```

@section ex_file_handles File Handle Pool

Manage a pool of file handles:

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <memory>
#include <fstream>
#include <iostream>

class FileHandle {
private:
    std::unique_ptr<std::ofstream> m_file;
    
public:
    FileHandle(const std::string& filename) 
        : m_file(std::make_unique<std::ofstream>(filename, std::ios::app)) {}
    
    void write(const std::string& data) {
        if (m_file && m_file->is_open()) {
            *m_file << data << std::endl;
        }
    }
};

int main() {
    siddiqsoft::arrp::resource_pool<std::shared_ptr<FileHandle>> pool;
    
    // Populate pool with file handles
    for (int i = 0; i < 5; ++i) {
        pool.return_to_pool(
            std::make_shared<FileHandle>("output.log")
        );
    }

    // Use file handles
    {
        auto file = pool.borrow_from_pool();
        file->write("Log entry 1");
        file->write("Log entry 2");
        // Automatically returned to pool
    }

    return 0;
}
```

@section ex_thread_pool_integration Thread Pool Integration

Use resource pool with a thread pool:

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <thread>
#include <vector>
#include <memory>

class Worker {
public:
    void process(const std::string& data) {
        std::cout << "Processing: " << data << std::endl;
    }
};

int main() {
    siddiqsoft::arrp::resource_pool<std::shared_ptr<Worker>> pool;
    
    // Populate pool
    for (int i = 0; i < 4; ++i) {
        pool.return_to_pool(std::make_shared<Worker>());
    }

    // Use from multiple threads
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&pool]() {
            for (int i = 0; i < 10; ++i) {
                auto worker = pool.borrow_from_pool();
                worker->process("task-" + std::to_string(i));
                // Automatically returned
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

@section ex_custom_factory Custom Resource Factory

Create resources on-demand with a factory:

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <iostream>

class ExpensiveResource {
private:
    int m_id;
    
public:
    ExpensiveResource(int id) : m_id(id) {
        std::cout << "Creating resource " << m_id << std::endl;
    }
    
    ~ExpensiveResource() {
        std::cout << "Destroying resource " << m_id << std::endl;
    }
    
    void use() {
        std::cout << "Using resource " << m_id << std::endl;
    }
};

int main() {
    static int resource_counter = 0;
    
    siddiqsoft::arrp::resource_pool<ExpensiveResource> pool{
        [](auto& p) -> siddiqsoft::arrp::scoped_resource<ExpensiveResource> {
            return siddiqsoft::arrp::scoped_resource<ExpensiveResource>(
                ExpensiveResource(++resource_counter),
                [&p](ExpensiveResource&& res) { 
                    p.return_to_pool(std::move(res)); 
                }
            );
        }
    };

    // Resources are created on-demand
    for (int i = 0; i < 5; ++i) {
        auto res = pool.borrow_from_pool();
        res.use();
        // Automatically returned
    }

    return 0;
}
```

@section ex_monitoring Pool Monitoring

Monitor pool state and diagnostics:

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    siddiqsoft::arrp::resource_pool<std::shared_ptr<int>> pool;
    
    // Populate pool
    for (int i = 0; i < 10; ++i) {
        pool.return_to_pool(std::make_shared<int>(i));
    }

    // Monitor pool state
    for (int i = 0; i < 5; ++i) {
        auto state = pool.to_json();
        std::cout << "Pool state: " << state.dump(2) << std::endl;
        
        // Borrow some resources
        std::vector<decltype(pool.borrow_from_pool())> borrowed;
        for (int j = 0; j < 3; ++j) {
            borrowed.push_back(pool.borrow_from_pool());
        }
        
        std::cout << "After borrowing 3 resources:" << std::endl;
        std::cout << pool.to_json().dump(2) << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
```

@section ex_resource_invalidation Resource Invalidation

Prevent resources from being returned to the pool:

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <memory>
#include <iostream>

class Resource {
public:
    void use() { std::cout << "Using resource" << std::endl; }
};

int main() {
    siddiqsoft::arrp::resource_pool<std::shared_ptr<Resource>> pool;
    
    // Populate pool
    for (int i = 0; i < 5; ++i) {
        pool.return_to_pool(std::make_shared<Resource>());
    }

    // Borrow and invalidate
    {
        auto res = pool.borrow_from_pool();
        res->use();
        
        // Take ownership of the resource
        auto ptr = std::move(*res);
        
        // Invalidate to prevent automatic return
        res.invalidate();
        
        // Resource is NOT returned to pool
        std::cout << "Pool size after invalidation: " << pool.size() << std::endl;
    }

    return 0;
}
```

@section ex_error_handling Error Handling

Handle pool exhaustion and errors:

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    siddiqsoft::arrp::resource_pool<std::shared_ptr<int>> pool;
    
    // Populate with limited resources
    for (int i = 0; i < 2; ++i) {
        pool.return_to_pool(std::make_shared<int>(i));
    }

    // Try to borrow more than available
    try {
        auto res1 = pool.borrow_from_pool();
        auto res2 = pool.borrow_from_pool();
        
        // Pool is now empty and at capacity
        auto res3 = pool.borrow_from_pool();  // This will throw
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Pool exhausted!" << std::endl;
    }

    return 0;
}
```

@section ex_scoped_usage Scoped Resource Usage

Demonstrate scoped_resource features:

```cpp
#include "siddiqsoft/resource_pool.hpp"
#include <iostream>

class Resource {
public:
    void doSomething() { std::cout << "Doing something" << std::endl; }
};

int main() {
    siddiqsoft::arrp::resource_pool<Resource> pool;
    
    // Populate pool
    for (int i = 0; i < 5; ++i) {
        pool.return_to_pool(Resource{});
    }

    // Dereference operator
    {
        auto res = pool.borrow_from_pool();
        (*res).doSomething();
    }

    // Implicit conversion
    {
        auto res = pool.borrow_from_pool();
        // Can pass to functions expecting Resource&
        auto& ref = static_cast<Resource&>(res);
        ref.doSomething();
    }

    // Move semantics
    {
        auto res1 = pool.borrow_from_pool();
        auto res2 = std::move(res1);
        // res1 is now invalid, only res2 will return the resource
    }

    return 0;
}
```
