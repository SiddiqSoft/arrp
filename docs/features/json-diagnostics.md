# JSON Diagnostics & Debugger Visualizers

`arrp` provides built-in metrics gathering and debugging visualizers to monitor resource usage and state during development and in production.

---

## Enabling JSON Serialization

JSON support is conditionally compiled. To enable the `.to_json()` methods on `resource_pool<T>` and `resource_guard<T>`, include `nlohmann/json.hpp` **before** `<siddiqsoft/arrp.hpp>`:

```cpp
#include <nlohmann/json.hpp>
#include <siddiqsoft/arrp.hpp>
#include <iostream>

int main()
{
    siddiqsoft::arrp::resource_pool<std::string> pool {8};
    pool.seed("item-1");

    // Dump pool diagnostics to JSON
    nlohmann::json stats = pool.to_json();
    std::cout << stats.dump(2) << '\n';
}
```

---

## Pool JSON Schema & Fields

Calling `pool.to_json()` returns a JSON object with the following telemetry fields:

| Field Name | Type | Description |
|---|---|---|
| `_typver` | `string` | Type identifier string (e.g. `"siddiqsoft.arrp.resource_pool/0.0.0"`). |
| `capacity` | `number` | Configured initial capacity (clamped between 1 and 255). |
| `size` | `number` | Current number of available (idle) resources in pool. |
| `peaksize` | `number` | Maximum number of idle resources observed in the pool concurrently. |
| `deficit` | `number` | Calculated capacity deficit (`capacity - (size + loans)`). |
| `seeds` | `number` | Total number of resources added via `seed()`. |
| `autoadds` | `number` | Total number of resources created dynamically by the factory callback. |
| `borrows` | `number` | Total number of successful borrow operations (`try_borrow` / `try_borrow_create`). |
| `returns` | `number` | Total number of resources returned back to pool by valid guards. |
| `abandons` | `number` | Total number of resources discarded (invalidated or moved out). |
| `loans` | `number` | Currently checked-out active guards (`borrows - returns - abandons`). |
| `items` | `array` | Array representation of available items (only available for `std::string` and `nlohmann::json` pools). |

### Example Output

```json
{
  "_typver": "siddiqsoft.arrp.resource_pool/0.0.0",
  "capacity": 8,
  "size": 2,
  "peaksize": 4,
  "deficit": 5,
  "seeds": 4,
  "autoadds": 1,
  "borrows": 3,
  "returns": 2,
  "abandons": 0,
  "loans": 1
}
```

---

## Visual Studio & VS Code Natvis Visualizer

The repository includes a custom Natvis file located at [`SiddiqSoft.arrp.natvis`](https://github.com/SiddiqSoft/arrp/blob/main/SiddiqSoft.arrp.natvis).

When debugging C++ code in Visual Studio or VS Code (with C/C++ extension), the Natvis file formats `resource_pool<T>` and `resource_guard<T>` objects cleanly in the Watch and Locals windows:

- **`resource_pool<T>`**: Displays active pool size, capacity, loan count, borrow/return metrics, and expands available queue elements cleanly.
- **`resource_guard<T>`**: Displays validity state (`valid` / `invalid`), held value reference, and underlying error code.

> [!TIP]
> When using `arrp` via the **NuGet package**, the Natvis file is linked into MSBuild targets automatically!
