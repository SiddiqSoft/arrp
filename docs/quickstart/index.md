# Integration Overview

`arrp` is a header-only library designed for quick integration into modern C++ build systems and project setups.

---

## Integration Methods

<div class="grid">
  <div class="card">
    <h3><a href="cmake/">CMake Integration</a></h3>
    <p>Incorporate <code>arrp</code> seamlessly into CMake projects using <code>FetchContent</code> or <code>find_package</code> with the imported target <code>arrp::arrp</code>.</p>
  </div>
  <div class="card">
    <h3><a href="package-managers/">Package Managers & Headers</a></h3>
    <p>Install via <b>NuGet</b> for MSBuild/Visual Studio workflows or include headers directly in your project include paths.</p>
  </div>
</div>

---

## Quick Reference

| Method | Target / Include | Command / Configuration |
|---|---|---|
| **CMake FetchContent** | `arrp::arrp` | `FetchContent_Declare(arrp ...)` |
| **NuGet Package** | `SiddiqSoft.arrp` | `nuget install SiddiqSoft.arrp` |
| **Manual Header** | `#include <siddiqsoft/arrp.hpp>` | Add `include/` directory to compiler include paths |
