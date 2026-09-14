# arrp

<div class="hero-tagline">Thread-safe resource pool library.</div>

[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.arrp?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=33&branchName=main)
[![NuGet Version](https://img.shields.io/nuget/v/SiddiqSoft.arrp?logo=nuget)](https://www.nuget.org/packages/SiddiqSoft.arrp/)
[![NuGet Downloads](https://img.shields.io/nuget/dt/SiddiqSoft.arrp?logo=nuget)](https://www.nuget.org/packages/SiddiqSoft.arrp/)
[![Tests](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/33/main.svg)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=33&branchName=main)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![License BSD-3](https://img.shields.io/badge/License-BSD--3--Clause-blue)](LICENSE)

**arrp** (`Auto Returning Resource Pool`) is a lightweight, thread-safe, header-only C++20 resource pool library.

**[Documentation: https://siddiqsoft.github.io/arrp/](https://siddiqsoft.github.io/arrp/)**

## Quick Example

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
    } // resource returns to the pool automatically
    
    return 0;
}
```

## Dependencies

```mermaid
graph TD
    arrp["arrp::arrp {{ version }}"]

    subgraph Core["Core Dependencies (via CPM)"]
        RUNONEND["RunOnEnd 1.4.5"]
    end

    subgraph TestBench["Test & Diagnostic Dependencies (Conditional)"]
        GTEST["gtest v1.17.0"]
        NLOHMANNJSON["nlohmann_json v3.12.0"]
    end

    arrp --> RUNONEND
    arrp -.->|BUILD_TESTS=ON| GTEST
    arrp -.->|BUILD_TESTS=ON| NLOHMANNJSON
```

See the [Getting Started guide](https://siddiqsoft.github.io/arrp/quickstart/#dependencies) for full installation instructions.

## License

BSD 3-Clause License. See [LICENSE](LICENSE).
