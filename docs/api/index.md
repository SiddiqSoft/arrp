# API Reference Overview

<div class="api-header-block">
  <div class="api-module-name">arrp C++ Reference</div>
  <div class="api-header-file">Generated from intermediate Doxygen XML</div>
</div>

The `siddiqsoft` namespace provides data structures and utilities for `arrp`.

## Classes & Structures

<table class="api-summary-table">
  <tr>
    <td class="memtype"><code>class</code></td>
    <td class="memitemleft"><a href="resource_guard.md"><strong>siddiqsoft::resource_guard</strong></a><div class="mdesc">RAII wrapper for managing resource lifecycle in a resource pool.</div></td>
  </tr>
  <tr>
    <td class="memtype"><code>class</code></td>
    <td class="memitemleft"><a href="resource_pool.md"><strong>siddiqsoft::resource_pool</strong></a><div class="mdesc">Thread-safe auto-returning resource pool.</div></td>
  </tr>
</table>

## Header Files

| Header File | Include Path | Description |
| :--- | :--- | :--- |
| **`resource_guard.hpp`** | `#include <siddiqsoft/private/resource_guard.hpp>` | RAII wrapper for managing resource lifecycle in a resource pool. |
| **`resource_pool.hpp`** | `#include <siddiqsoft/private/resource_pool.hpp>` | Thread-safe auto-returning resource pool. |

## System UML Class Diagram

The following diagram illustrates the primary classes, inheritance, and relationships in `arrp`. Click any node to navigate to its GitHub source location:

<!-- @@uml-diag:complete -->

### Source Code Mapping

| Component / Class | Header File | Source Link | Purpose & Architectural Role |
| :--- | :--- | :--- | :--- |
| [`siddiqsoft::arrp::resource_guard`](resource_guard.md) | <code><span class="filepath-dir">include/siddiqsoft/private/</span><wbr><span class="filepath-name">resource_guard.hpp</span></code> | [`resource_guard.hpp`](https://github.com/SiddiqSoft/arrp/blob/master/include/siddiqsoft/private/resource_guard.hpp#L99) | RAII wrapper for managing resource lifecycle in a resource pool. |
| [`siddiqsoft::arrp::resource_pool`](resource_pool.md) | <code><span class="filepath-dir">include/siddiqsoft/private/</span><wbr><span class="filepath-name">resource_pool.hpp</span></code> | [`resource_pool.hpp`](https://github.com/SiddiqSoft/arrp/blob/master/include/siddiqsoft/private/resource_pool.hpp#L77) | Thread-safe auto-returning resource pool. |
