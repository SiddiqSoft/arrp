#include <nlohmann/json.hpp>
#include <gtest/gtest.h>
#include <siddiqsoft/arrp.hpp>
#include <string>
#include <iostream>

TEST(doxygen_examples, seed)
{
// [seed_example]
    siddiqsoft::arrp::resource_pool<std::string> pool(10);
    
    // Seed by constructing in-place
    pool.seed(5, 'A'); // "AAAAA"
    
    // Seed by moving an existing object
    std::string existing = "Hello";
    pool.seed(std::move(existing));
// [seed_example]
}

TEST(doxygen_examples, try_borrow)
{
// [try_borrow_example]
    siddiqsoft::arrp::resource_pool<std::string> pool;
    pool.seed("42");
    
    {
        // Borrow the resource. It is removed from the pool queue.
        auto guard = pool.try_borrow();
        if (guard.is_valid()) {
            std::cout << "Borrowed: " << (*guard) << std::endl;
        }
        // When 'guard' goes out of scope, the resource is automatically returned to the pool.
    }
// [try_borrow_example]
}

TEST(doxygen_examples, try_borrow_create)
{
// [try_borrow_create_example]
    siddiqsoft::arrp::resource_pool<std::string> pool(5);
    
    // Set a factory callback to generate missing resources
    pool.set_factory_callback([]() {
        return std::string("99");
    });
    
    // The pool is currently empty, so try_borrow_create will invoke the factory
    auto guard = pool.try_borrow_create();
    if (guard.is_valid()) {
        std::cout << "Created on demand: " << (*guard) << std::endl;
    }
// [try_borrow_create_example]
}

TEST(doxygen_examples, clear)
{
// [clear_example]
    siddiqsoft::arrp::resource_pool<std::string> pool;
    pool.seed("A");
    pool.seed("B");
    
    // Empties the pool completely. Any resources currently borrowed by guards 
    // will be destroyed rather than returned upon guard destruction.
    pool.clear();
// [clear_example]
}

TEST(doxygen_examples, to_json)
{
// [to_json_example]
    siddiqsoft::arrp::resource_pool<std::string> pool(10);
    pool.seed("1");
    pool.seed("2");
    
    auto borrowed = pool.try_borrow();
    
    // Export telemetry statistics to a JSON object
    nlohmann::json stats = pool.to_json();
    std::cout << stats.dump(4) << std::endl;
// [to_json_example]
}

TEST(doxygen_examples, size)
{
// [size_example]
    siddiqsoft::arrp::resource_pool<std::string> pool(10);
    pool.seed("1");
    pool.seed("2");
    
    // size() returns the total number of resources (both idle in queue and currently borrowed)
    std::cout << "Total resources tracked: " << pool.size() << std::endl; // Outputs 2
// [size_example]
}

TEST(doxygen_examples, invalidate)
{
// [invalidate_example]
    siddiqsoft::arrp::resource_pool<std::string> pool;
    pool.seed("42");
    
    {
        auto guard = pool.try_borrow();
        if ((*guard) == "42") {
            // Resource is corrupted or no longer needed.
            // Invalidate the guard so the resource is destroyed instead of returning to the pool.
            guard.invalidate();
        }
    }
// [invalidate_example]
}

TEST(doxygen_examples, get)
{
// [get_example]
    siddiqsoft::arrp::resource_pool<std::string> pool;
    pool.seed("Hello");
    
    auto guard = pool.try_borrow();
    if (guard.is_valid()) {
        // Access the underlying resource
        (*guard) += " World";
    }
// [get_example]
}
