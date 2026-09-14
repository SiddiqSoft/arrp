# Architecture & Diagnostics

## Diagnostics & Natvis

`arrp` integrates seamlessly with Visual Studio and VS Code debugging environments through the provided `.natvis` file.

When debugging, a `resource_pool<T>` will display:
- Total capacity
- Number of currently available items
- In-flight borrow operations tracking
- Total borrow operations performed

## JSON Telemetry

If `nlohmann/json` is available in your project, `arrp` provides `to_json` integration for runtime diagnostics and telemetry:

```cpp
#include <nlohmann/json.hpp>
#include <siddiqsoft/arrp.hpp>

siddiqsoft::arrp::resource_pool<std::string> pool {4};
nlohmann::json state = pool; // Invokes to_json(nlohmann::json& j, const resource_pool<T>& p)
```
