# Examples Overview

The `arrp` repository includes runnable, standalone example applications located in the [`examples/`](https://github.com/SiddiqSoft/arrp/tree/master/examples) directory. These examples demonstrate how to integrate `arrp::resource_pool<T>` and `arrp::resource_guard<T>` into C++ applications for managing various resource types.

---

## Available Examples

<div class="grid">
  <div class="card">
    <h3><a href="scoped_file/">Scoped File Example</a></h3>
    <p>Demonstrates wrapping standard C <code>FILE*</code> handles inside a move-only RAII class (<code>ScopedFile</code>) and managing file handles using a seeded <code>arrp::resource_pool</code>.</p>
  </div>
  <div class="card">
    <h3><a href="scoped_curl/">Scoped cURL Example</a></h3>
    <p>Demonstrates thread-safe asynchronous HTTP request handling with <code>libcurl</code> (<code>CURL*</code>), dynamic on-demand resource creation via <code>try_borrow_create()</code> with timeouts, and JSON statistics diagnostics.</p>
  </div>
</div>

---

## Example Repository Directory

You can explore the source code for all examples directly in the repository:

- **[Repository Examples Folder (`examples/`)](https://github.com/SiddiqSoft/arrp/tree/master/examples)**
  - [`examples/scoped_file/`](https://github.com/SiddiqSoft/arrp/tree/master/examples/scoped_file): Standalone file handle pooling project.
  - [`examples/scoped_curl/`](https://github.com/SiddiqSoft/arrp/tree/master/examples/scoped_curl): Multi-threaded HTTP libcurl handle pooling project.

---

## Building the Examples

Each example project contains its own `CMakeLists.txt` and `CMakePresets.json`, allowing them to be built independently or as part of the overall workspace.

### Building `scoped_file`

```bash
cd examples/scoped_file
cmake --fresh --preset=Apple-Debug
cmake --build --preset=Apple-Debug
./build/Apple-Debug/scoped_file
```

### Building `scoped_curl`

> **Note**: Building `scoped_curl` requires `libcurl` development headers installed on your system.

```bash
cd examples/scoped_curl
cmake --fresh --preset=Apple-Debug
cmake --build --preset=Apple-Debug
./build/Apple-Debug/scoped_curl
```
