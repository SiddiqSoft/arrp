/**
@page usage_guide Usage guide

@section usage_basics Borrowing seeded resources

Construct the pool, add resources, then keep each borrowed guard in a scope that
ends before the pool's scope:

@code{.cpp}
#include <siddiqsoft/arrp.hpp>
#include <memory>

class connection {
public:
    void query(const char* statement);
};

int main()
{
    siddiqsoft::arrp::resource_pool<std::unique_ptr<connection>> pool {8};
    pool.seed(std::make_unique<connection>());

    {
        auto connection = pool.try_borrow();
        if (!connection) {
            return 1;
        }
        connection->get()->query("SELECT 1");
    } // The unique_ptr is returned to pool.
}
@endcode

Borrowing, seeding, clearing, sizing, setting the factory callback, and JSON
statistics are synchronized; a borrowed `resource_guard` is not. Move a guard to
transfer its ownership, and do not share the same guard between threads.

@section usage_factory Creating on demand

Use a factory when the pool may initially be empty or when invalidated resources
should be replaced on the next request:

@code{.cpp}
siddiqsoft::arrp::resource_pool<std::unique_ptr<connection>> pool {8};
pool.set_factory_callback([] {
    return std::make_unique<connection>();
});

auto connection = pool.try_borrow_create();
if (connection) {
    connection->get()->query("SELECT 1");
}
@endcode

`try_borrow()` only uses resources already in the pool. `try_borrow_create()`
uses the factory whenever no resource is available, including after a positive
wait timeout. Factory callbacks have no arguments, must return `T` or the pool's
scoped type, and must not call pool methods.

@section usage_waiting Waiting for a resource

Pass a positive duration to wait for another guard to return a resource:

@code{.cpp}
using namespace std::chrono_literals;

auto resource = pool.try_borrow(250ms);
if (!resource) {
    if (resource.error() == siddiqsoft::arrp::pool_error::Timeout) {
        // No resource returned within 250 ms.
    }
}
@endcode

With the default zero duration, borrowing is non-blocking and an empty pool
reports `pool_error::NoMoreResources`.

@section usage_invalidating Discarding a resource

Invalidate a resource that is unusable, or move it out through the rvalue
conversion. Both paths prevent it from returning to the pool:

@code{.cpp}
auto resource = pool.try_borrow();
if (resource && resource->get()->failed_health_check()) {
    resource.invalidate();
}

// Or transfer ownership out of a guard:
auto owned_connection =
    static_cast<std::unique_ptr<connection>>(std::move(resource));
@endcode

Do not use `operator*()` after invalidating a guard. `operator->()` returns
`nullptr` after invalidation.

@section usage_cleanup Cleaning up available resources

Supply a cleanup callback for resources that need explicit cleanup. It runs for
resources currently available when `clear()` or pool destruction occurs:

@code{.cpp}
siddiqsoft::arrp::resource_pool<FILE*> pool {
    4,
    [](FILE*& file) {
        if (file != nullptr) {
            std::fclose(file);
            file = nullptr;
        }
    }
};

pool.seed(std::fopen("output.log", "w"));
pool.clear();
@endcode

The cleanup callback is called under the pool lock. Keep it short and never call
any method on the same pool from it. `clear()` does not invalidate outstanding
guards; their valid resources may return later. Ensure all guards have been
destroyed before the pool itself is destroyed.

@section usage_statistics Inspecting statistics

Include nlohmann JSON before arrp to enable `to_json()`:

@code{.cpp}
#include <nlohmann/json.hpp>
#include <siddiqsoft/arrp.hpp>
#include <iostream>

auto stats = pool.to_json();
std::cout << stats.dump(2) << '\n';
@endcode

Useful fields are `size` (available resources), `loans` (currently borrowed
resources), `abandons` (discarded guards), and `autoadds` (factory-created
resources). See @ref api_reference for the complete schema.

@section usage_debugging Debugger visualization

The repository ships a Natvis file at [SiddiqSoft.arrp.natvis](../../SiddiqSoft.arrp.natvis) for Visual Studio and VS Code. It adds custom displays for `resource_pool` and `resource_guard`, making available resources, checked-out counts, counters, and validity state easier to inspect while debugging.

The same file is also included in the NuGet package so tooling can load it automatically when consuming the package.

@section usage_capacity Capacity guidance

The constructor's capacity is clamped to 1 through 255 and appears in statistics,
but it is not a hard maximum. `seed()` and a factory can add more resources than
the configured value, so size the pool responsibly in application code.
*/
