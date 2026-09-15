# Architecture & Diagnostics

`arrp` (Asynchronous Resource Reusable Pool) is designed for modern C++20 workflows requiring deterministic, RAII-enforced pooling of generic resources without triggering `std::bad_alloc` or risking resource exhaustion under heavy load.

## Core Design Tenets

1. **Lock-Aware but Not Lock-Free**: `resource_pool<T>` uses a combination of `std::mutex` (for memory synchronization) and `std::counting_semaphore` (for async/blocking waiters). While not strictly lock-free, this architecture provides extremely high throughput because the critical section only involves `std::vector` `push_back`/`pop_back`.
2. **Move Semantics**: Everything in `arrp` revolves around `<utility>` moves. Objects are moved into the pool memory via `seed(T&&)` and moved out via `try_borrow()`. There are zero allocations occurring on the hot path after the pool is seeded.
3. **Guard-Oriented**: Users never receive a raw `T*`. They receive a `resource_guard<T>`, which behaves like a smart pointer but securely returns the resource back to its parent pool exactly when it falls out of scope, guaranteeing safety even during unwinding from exceptions.

## UML Class Diagram

<!-- UML_CLASS_DIAGRAM_START -->
<!-- @@uml-diag:complete -->

<!-- @@uml-diag:source-table -->
<!-- UML_CLASS_DIAGRAM_END -->
