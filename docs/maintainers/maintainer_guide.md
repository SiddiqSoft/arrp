# Maintainer Guide

Codebase architecture, development guidelines, formatting standards, and maintainer documentation index for `siddiqsoft::sip2json`.

---

## Documentation Index

The maintainer documentation is organized into modular topic guides:

| Topic Guide | Description |
| :--- | :--- |
| [**CI/CD Pipelines**](pipelines.md) | Azure Pipelines architecture, build matrix, platform triggers, and parameters |
| [**CMake Presets**](cmake_presets.md) | Decoupled presets hierarchy, `project-base.json`, and preset reference |
| development workflow | Local building, testing, standalone validation subproject, and macOS toolchain |
| [**Build Agent Requirements**](build_agents.md) | Prerequisites and configuration for macOS, Linux, and Windows self-hosted agents |
| release guidelines | GitVersion, SemVer tagging, GitHub Releases, and NuGet package publishing |
| documentation guidelines | MkDocs Material, Doxygen XML, custom CSS tokens, hooks, and local preview |

---

## Codebase Architecture & UML Class Diagram

The following UML class diagram illustrates the primary classes, relationships, and exception hierarchy in `siddiqsoft::sip2json`. The diagram is auto-generated from the C++ source AST via Doxygen XML. Each node in the diagram links directly to its source header file on GitHub.

<!-- @@uml-diag:complete -->

<!-- @@uml-diag:source-table -->

---

### Generating & Previewing Documentation Locally

The API documentation, inheritance graphs, and structural markdown are entirely auto-generated from the Doxygen XML output, guaranteeing that the `docs/` folder accurately reflects the latest C++ headers.

To build the documentation and spin up a live-reloading preview server locally:

1. **Install Prerequisites**: Ensure you have Python 3 and the `mkdocs-material` stack installed (along with `doxygen` and `graphviz` for AST generation).
   ```bash
   pip install mkdocs-material mkdocs-git-revision-date-localized-plugin
   ```

2. **Run the Rebuild & Serve Script**: Use the unified shell script which orchestrates Doxygen generation, the Python API generator, and the MkDocs live server.
   ```bash
   ./docs/rebuild-docs.sh serve
   ```
   *The documentation site will be served locally at `http://127.0.0.1:8000` and will auto-reload when you edit any Markdown files or modify the C++ headers.*

3. **Verify Build Health**: Before submitting a Pull Request, ensure that the build emits zero dead links or schema errors by running:
   ```bash
   ./docs/rebuild-docs.sh
   ```
   *This executes `mkdocs build --strict` which treats warnings (like broken links) as fatal errors.*
---

## Source Code Formatting (Clang-Format)

All C++ source code (`include/`, `tests/`, and `benchmarks/`) adheres to the formatting rules defined in [`.clang-format`](https://github.com/SiddiqSoft/sip2json/blob/master/.clang-format) at the repository root.

The configuration is based on the **WebKit** style with modern C++20 conventions:
* **Column Limit**: 132 characters
* **Indentation**: 4 spaces (tabs are never used)
* **Brace Style**: WebKit (braces break before functions, classes, and catch/else blocks)
* **Pointer Alignment**: Left (`const sipmessage& msg`, `std::string_view* ptr`)
* **Standard**: C++20

---

### How to Bulk Clang-Format the Source Code

Maintainers have multiple ways to format the entire codebase in bulk:

#### Option 1: Via CMake Build Target (Recommended)

When configured locally, CMake generates a dedicated `format` (and `clang-format`) target:

```bash
# Format using an active preset build directory:
cmake --build --preset Apple-Clang-Debug --target format

# Or format directly against any existing build folder:
cmake --build build/Apple-Clang-Debug --target format
```

!!! tip
    **Automatic Debug Formatting**: On non-CI local builds, CMake enables `sip2json_ENABLE_CLANG_FORMAT=ON` by default in `Debug` configuration. Formatting runs automatically prior to every local Debug compilation whenever a compatible `clang-format` executable is detected.

---

#### Option 2: Bulk Command-Line via Find (macOS & Linux)

To reformat all C++ header and source files across `include/`, `tests/`, and `benchmarks/` in one command:

```bash
find include tests benchmarks -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" \) -exec clang-format -i --style=file {} +
```

To verify formatting without modifying files, pass `--dry-run --Werror`:

```bash
find include tests benchmarks -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" \) -exec clang-format --dry-run --Werror --style=file {} +
```

---

#### Option 3: Bulk Command-Line via Git (Cross-Platform)

Format all tracked C++ files in the repository using `git ls-files`:

=== "macOS & Linux (bash / zsh)"
    ```bash
    git ls-files '*.hpp' '*.cpp' '*.h' | xargs clang-format -i --style=file
    ```

=== "Windows (PowerShell)"
    ```powershell
    git ls-files '*.hpp', '*.cpp', '*.h' | ForEach-Object { clang-format -i --style=file $_ }
    ```

---

#### Option 4: Bulk PowerShell Script (Windows)

On Windows systems without Git bash, run the following PowerShell one-liner:

```powershell
Get-ChildItem -Path include, tests, benchmarks -Include *.hpp, *.cpp, *.h -Recurse | ForEach-Object {
    clang-format -i --style=file $_.FullName
}
```

---

#### Option 5: Editor & IDE Integration

* **Visual Studio 2022**: Visual Studio automatically detects `.clang-format` at the repository root. Press `Ctrl+K, Ctrl+D` to format the active document, or `Ctrl+K, Ctrl+F` to format a selection.
* **Visual Studio Code**: Ensure the [C/C++ Extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) is installed. In `.vscode/settings.json`:
  ```json
  {
    "C_Cpp.clang_format_style": "file",
    "editor.formatOnSave": true
  }
  ```
* **CLion**: Navigate to **Settings > Editor > Code Style > C/C++** and verify **Enable ClangFormat** is checked. Press `Ctrl+Alt+L` (`Cmd+Alt+L` on macOS) to format.

---

## Maintainer Pre-Flight Checklist

Before submitting a pull request or pushing to `master`:

1. **Format Code**: Run the bulk clang-format command or `cmake --build <preset> --target format`.
2. **Build Cleanly**: Compile with zero compiler warnings using your platform preset:
   ```bash
   cmake --build --preset <preset-name>
   ```
3. **Execute All Tests**: Ensure 100% of tests pass:
   ```bash
   ctest --preset <preset-name> --output-on-failure
   ```
4. **Validate Documentation**: Verify zero broken links or markdown syntax errors:
   ```bash
   mkdocs build --strict
   ```
5. **Major Version Updates**: When introducing breaking API changes or preparing a major version release, manually update the `next-version:` entry in [`GitVersion.yml`](https://github.com/SiddiqSoft/sip2json/blob/master/GitVersion.yml) (e.g. `next-version: 4.0.0`) so GitVersion establishes the new major version baseline.


## Codebase Architecture & UML Class Diagram

<!-- UML_CLASS_DIAGRAM_START -->
The following UML class diagram illustrates the primary classes, relationships, and inheritance. The diagram is auto-generated from the C++ source AST via Doxygen XML. Each node in the diagram links directly to its source header file on GitHub.

```mermaid
classDiagram
    direction TB

    classDef coreClass fill:rgba(35,73,109,0.08),stroke:#23496d,stroke-width:2px;
    classDef utilityClass fill:rgba(15,118,110,0.08),stroke:#0f766e,stroke-width:2px;
    classDef exceptionClass fill:rgba(185,28,28,0.06),stroke:#b91c1c,stroke-width:1.5px;
    classDef externalClass fill:rgba(100,116,139,0.06),stroke:#64748b,stroke-width:1.5px,stroke-dasharray: 4 3;
    classDef highlightClass fill:rgba(2,132,199,0.18),stroke:#0284c7,stroke-width:3px;

    class resource_guard["siddiqsoft::arrp::resource_guard"] {
        +resource_guard(const resource_guard &) void
        +operator_assign(const resource_guard &) resource_guard &
        +resource_guard(const pool_error &err) void
        +resource_guard(resource_guard &&src) void
        +operator_assign(resource_guard &&src) resource_guard &
    }
    class resource_guard:::coreClass

    class resource_pool["siddiqsoft::arrp::resource_pool"] {
        +resource_pool(resource_pool &) void
        +resource_pool(resource_pool &&src) void
        +operator_assign(resource_pool &) resource_pool &
        +operator_assign(resource_pool &&src) resource_pool &
        +resource_pool(uint8_t init_capacity, std::function~ void(T &)~ &&on_shutdown_callback) void
    }
    class resource_pool:::coreClass


    link resource_guard "https://github.com/SiddiqSoft/arrp/blob/master/include/siddiqsoft/private/resource_guard.hpp#L99" "Source: include/siddiqsoft/private/resource_guard.hpp"
    link resource_pool "https://github.com/SiddiqSoft/arrp/blob/master/include/siddiqsoft/private/resource_pool.hpp#L77" "Source: include/siddiqsoft/private/resource_pool.hpp"
```

### Source Code Mapping

| Component / Class | Header File | Source Link | Purpose & Architectural Role |
| :--- | :--- | :--- | :--- |
| [`siddiqsoft::arrp::resource_guard`](../api/resource_guard.md) | <code><span class="filepath-dir">include/siddiqsoft/private/</span><wbr><span class="filepath-name">resource_guard.hpp</span></code> | [`resource_guard.hpp`](https://github.com/SiddiqSoft/arrp/blob/main/include/siddiqsoft/private/resource_guard.hpp#L99) | RAII wrapper for managing resource lifecycle in a resource pool. |
| [`siddiqsoft::arrp::resource_pool`](../api/resource_pool.md) | <code><span class="filepath-dir">include/siddiqsoft/private/</span><wbr><span class="filepath-name">resource_pool.hpp</span></code> | [`resource_pool.hpp`](https://github.com/SiddiqSoft/arrp/blob/main/include/siddiqsoft/private/resource_pool.hpp#L77) | Thread-safe auto-returning resource pool. |
<!-- UML_CLASS_DIAGRAM_END -->
