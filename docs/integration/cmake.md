# CMake Integration

`arrp` integrates directly into CMake projects via `FetchContent` or as a target via `add_subdirectory`.

---

## Using CMake `FetchContent` (Recommended)

Add the following to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
    arrp
    GIT_REPOSITORY https://github.com/SiddiqSoft/arrp.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(arrp)

# Link arrp::arrp to your application or library target
target_link_libraries(your_target PRIVATE arrp::arrp)
```

---

## Using Git Submodule / `add_subdirectory`

If you add `arrp` as a Git submodule in your repository (e.g. under `third_party/arrp`):

```bash
git submodule add https://github.com/SiddiqSoft/arrp.git third_party/arrp
```

In your root `CMakeLists.txt`:

```cmake
add_subdirectory(third_party/arrp)

target_link_libraries(your_target PRIVATE arrp::arrp)
```

---

## CMake Target Details

The library provides an alias target `arrp::arrp` which automatically sets up:

- **Include Directories**: Appends `<arrp_source_dir>/include` to target include paths.
- **C++ Standard**: Requires C++23 (`cxx_std_23`).
