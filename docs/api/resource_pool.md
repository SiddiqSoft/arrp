# siddiqsoft::arrp::resource_pool Class Reference

<div class="grid" markdown="1">
<div class="api-intro-col" markdown="1">
<div class="api-header-block">
  <div class="api-module-name">Namespace siddiqsoft</div>
  <div class="api-header-file">#include &lt;siddiqsoft/private/resource_pool.hpp&gt;</div>
</div>

`resource_pool` component of `arrp`.

</div>
<div class="api-diag-col" markdown="1">

**Class Hierarchy & Inheritance**

The following UML class diagram highlights `siddiqsoft::arrp::resource_pool` and its direct relationships. Click the node to navigate to its source file on GitHub.

<!-- @@uml-diag:resource_pool -->

</div>
</div>

## Member Functions Summary

### Constructors & Destructors

<table class="api-summary-table">
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#resource_pool"><strong>resource_pool</strong></a> ((resource_pool &amp;)=delete)
      <div class="mdesc">Copy constructor is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#resource_pool"><strong>resource_pool</strong></a> ((resource_pool &amp;&amp;src)=delete)
      <div class="mdesc">Move constructor is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#resource_pool"><strong>resource_pool</strong></a> (<div class="param-wrap">uint8_t init_capacity=resource_pool_limits::DefaultCapacity,</div><div class="param-wrap">std::function&lt; void(T &amp;)&gt; &amp;&amp;on_shutdown_callback={}</div>)
      <div class="mdesc">Constructs a resource pool with an optional cleanup callback.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#resource_pool"><strong>resource_pool</strong></a> (<div class="param-wrap">std::function&lt; void(T &amp;)&gt; &amp;&amp;on_shutdown_callback</div>)
      <div class="mdesc">Constructs a resource pool with only cleanup callback.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#~resource_pool"><strong>~resource_pool</strong></a> ()
      <div class="mdesc">Destructor - cleans up all resources in the pool.</div>
    </td>
  </tr>
</table>

### Core Accessors & Modifiers

<table class="api-summary-table">
  <tr>
    <td class="memtype"><code>resource_pool &amp;</code></td>
    <td class="memitemleft"><a href="#operator="><strong>operator=</strong></a> ((resource_pool &amp;)=delete)
      <div class="mdesc">Copy assignment operator is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_pool &amp;</code></td>
    <td class="memitemleft"><a href="#operator="><strong>operator=</strong></a> ((resource_pool &amp;&amp;src)=delete)
      <div class="mdesc">Move assignment operator is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#set_factory_callback"><strong>set_factory_callback</strong></a> (F &amp;&amp;f)
      <div class="mdesc">Sets the factory used by try_borrow_create() when no resource is available.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>pool_error</code></td>
    <td class="memitemleft"><a href="#clear"><strong>clear</strong></a> ()
      <div class="mdesc">Clears all resources from the pool.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>auto</code></td>
    <td class="memitemleft"><a href="#size"><strong>size</strong></a> (() const)
      <div class="mdesc">Gets the current size of the pool.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_guard&lt; T &gt;</code></td>
    <td class="memitemleft"><a href="#try_borrow"><strong>try_borrow</strong></a> (<div class="param-wrap">std::chrono::nanoseconds timeout={}</div>)
      <div class="mdesc">Borrows an available resource without creating one.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_guard&lt; T &gt;</code></td>
    <td class="memitemleft"><a href="#try_borrow_create"><strong>try_borrow_create</strong></a> (<div class="param-wrap">std::chrono::nanoseconds timeout={}</div>)
      <div class="mdesc">Borrows an available resource or creates one through the factory.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>pool_error</code></td>
    <td class="memitemleft"><a href="#seed"><strong>seed</strong></a> (Args &amp;&amp;... args)
      <div class="mdesc">Adds a resource to the pool by constructing it in-place.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>pool_error</code></td>
    <td class="memitemleft"><a href="#seed"><strong>seed</strong></a> (T &amp;&amp;item)
      <div class="mdesc">Adds a resource to the pool by moving it.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>nlohmann::json</code></td>
    <td class="memitemleft"><a href="#to_json"><strong>to_json</strong></a> (() const)
      <div class="mdesc">Member function.</div>
    </td>
  </tr>
</table>

## Member Function Documentation

<div class="memitem" id="resource_pool" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">resource_pool()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_pool::resource_pool(resource_pool &)=delete;
```

</div>
<div class="memdoc" markdown="1">

Copy constructor is deleted.
`resource_pool` is not copyable to prevent resource duplication

</div>
</div>

<div class="memitem" id="resource_pool" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">resource_pool()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_pool::resource_pool(resource_pool &&src)=delete;
```

</div>
<div class="memdoc" markdown="1">

Move constructor is deleted.
`resource_pool` is not movable to maintain resource ownership

</div>
</div>

<div class="memitem" id="operator=" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator=()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
resource_pool & resource_pool::operator=(resource_pool &)=delete;
```

</div>
<div class="memdoc" markdown="1">

Copy assignment operator is deleted.
`resource_pool` is not copyable to prevent resource duplication

</div>
</div>

<div class="memitem" id="operator=" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator=()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
resource_pool & resource_pool::operator=(resource_pool &&src)=delete;
```

</div>
<div class="memdoc" markdown="1">

Move assignment operator is deleted.
`resource_pool` is not movable to maintain resource ownership

</div>
</div>

<div class="memitem" id="resource_pool" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">resource_pool()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_pool::resource_pool(
    uint8_t init_capacity=resource_pool_limits::DefaultCapacity,
    std::function< void(
    T &
)> &&on_shutdown_callback={}
);
```

</div>
<div class="memdoc" markdown="1">

Constructs a resource pool with an optional cleanup callback.
<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>init_capacity</code> &mdash; Initial capacity of the pool </li>
  <li><code>on_shutdown_callback</code> &mdash; Optional cleanup callback invoked on destruction</li>
</ul>

<div class="memdoc-section-title">Note</div>

Register a factory separately with `set_factory_callback()`. 

<div class="memdoc-section-title">Note</div>

Capacity is clamped to [MinimumCapacity, MaxCapacity] but does not enforce a maximum number of seeded or factory-created resources.

</div>
</div>

<div class="memitem" id="resource_pool" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">resource_pool()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_pool::resource_pool(std::function< void(T &)> &&on_shutdown_callback);
```

</div>
<div class="memdoc" markdown="1">

Constructs a resource pool with only cleanup callback.
<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>on_shutdown_callback</code> &mdash; Cleanup callback invoked on destruction</li>
</ul>

<div class="memdoc-section-title">Note</div>

Uses the default capacity. The cleanup callback is invoked for resources available during `clear()` or destruction.

</div>
</div>

<div class="memitem" id="~resource_pool" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">~resource_pool()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_pool::~resource_pool();
```

</div>
<div class="memdoc" markdown="1">

Destructor - cleans up all resources in the pool.
Sets the shutdown flag and delegates to `clear()` to clean up resources. The cleanup callback (if provided) is invoked for each resource during cleanup.

<div class="memdoc-section-title">Note</div>

Exceptions derived from std::exception in the cleanup callback are caught and written to stderr.

</div>
</div>

<div class="memitem" id="set_factory_callback" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">set_factory_callback()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_pool::set_factory_callback(F &&f);
```

</div>
<div class="memdoc" markdown="1">

Sets the factory used by try_borrow_create() when no resource is available.
<div class="memdoc-section-title">Template Parameters</div>

<ul>
  <li><code>F</code> &mdash; Callable type invokable with no arguments returning <code>`resource_guard`<T></code> or <code>T</code> </li>
</ul>

<div class="memdoc-section-title">Note</div>

Safe to call concurrently with `borrow_impl()`: assignment is synchronized under m_pool_lock, matching the read sites in `borrow_impl()`. A borrow in flight may still use the factory that was registered just before or after this call (no ordering is guaranteed relative to a specific concurrent borrow), but the read/write of the underlying std::function is race-free.

</div>
</div>

<div class="memitem" id="clear" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">clear()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
pool_error resource_pool::clear();
```

</div>
<div class="memdoc" markdown="1">

Clears all resources from the pool.
Removes currently available resources and invokes the cleanup callback for each. Borrowed resources can return after `clear()` completes.

<div class="memdoc-section-title">Returns</div>

`pool_error::Ok`

<div class="memdoc-section-title">Note</div>

The cleanup callback runs under the pool lock. Exceptions derived from std::exception are caught and written to stderr. 

<div class="memdoc-section-title">Note</div>

Non-blocking by design: if a concurrent borrow has already claimed a resource's semaphore permit but not yet popped it from the pool (it is waiting on the same lock `clear()` holds), that item is left in place for the borrower rather than drained here. This avoids a deadlock; it means a racing `clear()` call is not guaranteed to empty every resource that was visible to `size()` just before it ran.

</div>
</div>

<div class="memitem" id="size" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">size()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
auto resource_pool::size() const;
```

</div>
<div class="memdoc" markdown="1">

Gets the current size of the pool.
<div class="memdoc-section-title">Returns</div>

Number of currently available resources

<div class="memdoc-section-title">Note</div>

Does not include checked-out resources

</div>
</div>

<div class="memitem" id="try_borrow" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">try_borrow()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
resource_guard< T > resource_pool::try_borrow(std::chrono::nanoseconds timeout={});
```

</div>
<div class="memdoc" markdown="1">

Borrows an available resource without creating one.
<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>timeout</code> &mdash; Maximum time to wait; zero performs a non-blocking attempt. </li>
</ul>

<div class="memdoc-section-title">Returns</div>

A valid scoped resource, or an invalid one with NoMoreResources, Timeout, ShutdownInitiated, or Unknown set as its error.

</div>
</div>

<div class="memitem" id="try_borrow_create" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">try_borrow_create()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
resource_guard< T > resource_pool::try_borrow_create(std::chrono::nanoseconds timeout={});
```

</div>
<div class="memdoc" markdown="1">

Borrows an available resource or creates one through the factory.
<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>timeout</code> &mdash; Maximum time to wait; zero performs a non-blocking attempt. </li>
</ul>

<div class="memdoc-section-title">Returns</div>

A valid scoped resource, or an invalid one when shutdown or an implementation or factory error prevents borrowing.

</div>
</div>

<div class="memitem" id="seed" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">seed()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
pool_error resource_pool::seed(Args &&... args);
```

</div>
<div class="memdoc" markdown="1">

Adds a resource to the pool by constructing it in-place.
<div class="memdoc-section-title">Template Parameters</div>

<ul>
  <li><code>Args</code> &mdash; Types of arguments to forward to T's constructor </li>
</ul>

<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>args</code> &mdash; Arguments to forward to T's constructor for in-place construction </li>
</ul>

<div class="memdoc-section-title">Returns</div>

`pool_error::Ok`, or `pool_error::ShutdownInitiated` during destruction

<div class="memdoc-section-title">Note</div>

Resource is constructed in-place 

<div class="memdoc-section-title">Note</div>

Does not enforce the configured capacity. Do not use this to return a borrowed resource; guards return resources automatically.

</div>
</div>

<div class="memitem" id="seed" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">seed()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
pool_error resource_pool::seed(T &&item);
```

</div>
<div class="memdoc" markdown="1">

Adds a resource to the pool by moving it.
<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>item</code> &mdash; The resource to add (moved) </li>
</ul>

<div class="memdoc-section-title">Returns</div>

`pool_error::Ok`, or `pool_error::ShutdownInitiated` during destruction

<div class="memdoc-section-title">Note</div>

Resource is moved into the pool 

<div class="memdoc-section-title">Note</div>

Does not enforce the configured capacity. Do not use this to return a borrowed resource; guards return resources automatically.

</div>
</div>

<div class="memitem" id="to_json" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">to_json()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
nlohmann::json resource_pool::to_json() const;
```

</div>
<div class="memdoc" markdown="1">

Executes component operation.

</div>
</div>

## Source Code Reference

- Header: [`include/siddiqsoft/private/resource_pool.hpp`](https://github.com/SiddiqSoft/arrp/blob/master/include/siddiqsoft/private/resource_pool.hpp#L88)
