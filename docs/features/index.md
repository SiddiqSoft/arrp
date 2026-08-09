# Features Overview

`arrp` provides a simple yet versatile set of tools for managing poolable resources in modern C++ applications.

---

## Core Feature Areas

<div class="grid">
  <div class="card">
    <h3><a href="resource-management/">Resource Management</a></h3>
    <p>Learn about borrowing resources in FIFO order, seeding pre-allocated instances, factory callbacks for dynamic allocations, and invalidation mechanisms.</p>
  </div>
  <div class="card">
    <h3><a href="threading-lifetime/">Threading & Lifetime Rules</a></h3>
    <p>Understand the thread-safety semantics of <code>resource_pool&lt;T&gt;</code> versus <code>resource_guard&lt;T&gt;</code>, object destruction rules, and pool cleanup handlers.</p>
  </div>
  <div class="card">
    <h3><a href="json-diagnostics/">JSON & Diagnostics</a></h3>
    <p>Enable runtime statistics serialization via <code>nlohmann/json</code>, monitor metrics (loans, returns, abandons), and utilize native Visual Studio Natvis visualizers.</p>
  </div>
</div>

---

## Summary of Capabilities

- **Header-Only Library**: Just add `include/siddiqsoft/arrp.hpp` to your project—no build step required.
- **Move-Only Guards**: Prevents accidental copying or multi-thread data races on borrowed handles.
- **Explicit Invalidation**: Allows discarding corrupted or moved-out resources without returning them to pool rotation.
- **Custom Shutdown Callbacks**: Register cleanup hooks to execute when available resources are cleared or during pool destruction.
- **Zero-Allocation Operation**: When using pre-seeded resource pools, borrowing and returning perform no dynamic memory allocations.
