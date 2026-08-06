# arrp – Copilot Instructions

## Overview

`arrp` is a **header-only C++23** library implementing an auto-returning resource pool. All implementation lives under `include/siddiqsoft/private/`; the public entry point is `include/siddiqsoft/arrp.hpp`.

## Build, Test, and Lint

Tests are **off by default**. Enable them with `-D arrp_BUILD_TESTS=ON`.

```bash
# Configure (macOS default Xcode toolchain)
cmake --fresh --preset=Apple-Debug -D arrp_BUILD_TESTS=ON

# Build
cmake --build --preset=Apple-Debug

# Run all tests
ctest --preset=Apple-Debug

# Run a single test binary directly (after build)
./build/Apple-Debug/tests/arrp_tests --gtest_filter=<TestSuiteName>.<TestName>
```

Other presets: `Apple-LLVM-Debug/Release`, `Linux-Clang-Debug/Release`, `Linux-GCC-Debug/Release`, `Windows-x64-Debug/Release`, `Windows-arm64-Debug/Release`.

`clang-tidy` runs automatically during Debug builds via the preset config (checks: `modernize-*`, `cppcoreguidelines-*`, excluding `avoid-magic-numbers` and `use-trailing-return-type`).

## Architecture

```
include/siddiqsoft/
  arrp.hpp                    ← public header (include this)
  private/
    common.hpp                ← enums: pool_error, release_reason, resource_pool_limits
    resource_guard.hpp        ← move-only RAII wrapper returned by try_borrow()
    resource_pool.hpp         ← thread-safe pool owning a std::deque<T>
```

- `resource_pool<T>` holds resources in a `std::deque` protected by `std::mutex` (or `std::recursive_mutex` when `arrp_USE_RECURSIVE_MUTEX=ON`, enabled automatically in Debug builds).
- `resource_guard<T>` is the only way to borrow a resource; it returns the resource on destruction (or discards it if `invalidate()` was called or the value was moved out via `static_cast<T>(std::move(guard))`).
- JSON statistics (`to_json()`) are available only when `nlohmann/json.hpp` is included **before** `siddiqsoft/arrp.hpp`.
- Tests define `arrp_TESTING_MODE=1` to access protected members.

## Dependencies

Managed via **CPM** (CMake Package Manager). Cache lives at `~/.cache/.cpmcache` (Linux/macOS) or `%LOCALAPPDATA%/CPM/.cpmcache` (Windows).

- Runtime: `siddiqsoft/RunOnEnd` (RAII scope-exit helper)
- Test-only: `google/googletest`, `nlohmann/json`

## Key Conventions

- **Namespace**: `siddiqsoft::arrp`
- **Header guard style**: both `#pragma once` and `#ifndef ARRP_*_HPP` guards are used together.
- **`std::formatter` specializations** are defined in `common.hpp` for all enums — always add one when introducing a new enum.
- **No hard capacity limit**: configured capacity (clamped 1–255) is for reporting only; `seed()` and factory callbacks can exceed it.
- **Factory callback** set via `set_factory_callback()` is used only by `try_borrow_create()`, never by `try_borrow()`.
- **Cleanup callback** passed to `clear()` or destructor runs **under the pool lock** — it must not call any pool methods.
- **Formatting**: `.clang-format` enforces `ColumnLimit: 132`, 4-space indent, brace-on-new-line for functions/classes, `SortIncludes: false`.
- **CI**: Azure Pipelines (`azure-pipelines.yml`). CI builds set `CI_BUILDID` to the GitVersion value and enable `arrp_BUILD_TESTS=ON`.
