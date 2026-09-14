# Package Managers & Manual Integration

`arrp` can be consumed through package managers such as NuGet or incorporated manually into any C++ project build pipeline.

---

## NuGet Integration (Windows / MSBuild)

`arrp` is published as a NuGet package for MSBuild and Visual Studio users.

### Visual Studio Package Manager Console

```powershell
Install-Package SiddiqSoft.arrp
```

### .NET CLI / nuget.exe

```bash
nuget install SiddiqSoft.arrp
```

> [!TIP]
> The NuGet package automatically includes MSBuild targets that append the include path to your project settings and register the Visual Studio debugger Natvis visualizer (`SiddiqSoft.arrp.natvis`).

---

## Manual Header Integration

Since `arrp` is header-only, manual integration requires no compilation or static linking.

1. Download or copy the [`include/`](https://github.com/SiddiqSoft/arrp/tree/main/include) directory into your project.
2. Add `include/` to your compiler's include paths (`-Iinclude` or `/Iinclude`).
3. Include the main header in your C++ source code:

```cpp
#include <siddiqsoft/arrp.hpp>
```

---

## Optional JSON Support Setup

If you want to use the `to_json()` statistics functionality:

1. Ensure [nlohmann/json](https://github.com/nlohmann/json) is available in your include paths.
2. Include `<nlohmann/json.hpp>` **before** `<siddiqsoft/arrp.hpp>`:

```cpp
#include <nlohmann/json.hpp>
#include <siddiqsoft/arrp.hpp>
```
