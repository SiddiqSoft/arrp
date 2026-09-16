# siddiqsoft::arrp::resource_pool Class Reference

<div class="grid" markdown="1">
<div class="api-intro-col" markdown="1">
<div class="api-header-block">
  <div class="api-module-name">Namespace {root_namespace}</div>
  <div class="api-header-file">#include &lt;siddiqsoft/private/resource_pool.hpp&gt;</div>
</div>

Thread-safe auto-returning resource pool.

</div>
<div class="api-diag-col" markdown="1">

**Class Hierarchy & Inheritance**

The following UML class diagram highlights `siddiqsoft::arrp::resource_pool` and its direct relationships.

<div class="uml-diagram-container graphviz-uml" data-graph-type="coll">
<span class="uml-diagram-figure" style="display: block;">
<span class="uml-diagram-viewport" style="display: block;">
<svg class="graphviz-uml-svg" style="max-width: 100%; height: auto;" width="128pt" height="344pt"
 viewBox="0.00 0.00 128.00 344.00" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
<g id="graph0" class="graph" transform="scale(1 1) rotate(0) translate(4 339.5)">
<title>siddiqsoft::arrp::resource_pool&lt; T &gt;</title>
<!-- Node1 -->
<g id="Node000001" class="node">
<title>Node1</title>
<g id="a_Node000001"><a xlink:title="Thread&#45;safe auto&#45;returning resource pool.">
<polygon fill="#999999" stroke="none" points="120,-335.5 0,-335.5 0,0 120,0 120,-335.5"/>
<polygon fill="#666666" stroke="#666666" points="0,-307 0,-307 120,-307 120,-307 0,-307"/>
<polygon fill="#666666" stroke="#666666" points="0,-291.75 0,-291.75 120,-291.75 120,-291.75 0,-291.75"/>
<polygon fill="none" stroke="#666666" points="0,0 0,-335.5 120,-335.5 120,0 0,0"/>
<text xml:space="preserve" text-anchor="start" x="6" y="-323.25"  font-size="10.00">siddiqsoft::arrp::resource</text>
<text xml:space="preserve" text-anchor="start" x="36.38" y="-312"  font-size="10.00">_pool&lt; T &gt;</text>
<text xml:space="preserve" text-anchor="start" x="58.5" y="-296.75"  font-size="10.00"> </text>
<text xml:space="preserve" text-anchor="start" x="4" y="-281.5"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-281.5"  font-size="10.00">resource_pool()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-266.25"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-266.25"  font-size="10.00">resource_pool()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-251"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-251"  font-size="10.00">operator=()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-235.75"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-235.75"  font-size="10.00">operator=()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-220.5"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-220.5"  font-size="10.00">resource_pool()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-205.25"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-205.25"  font-size="10.00">resource_pool()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-190"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-190"  font-size="10.00">~resource_pool()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-174.75"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-174.75"  font-size="10.00">set_factory_callback()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-159.5"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-159.5"  font-size="10.00">clear()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-144.25"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-144.25"  font-size="10.00">size()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-129"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-129"  font-size="10.00">try_borrow()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-113.75"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-113.75"  font-size="10.00">try_borrow_create()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-98.5"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-98.5"  font-size="10.00">seed()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-83.25"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-83.25"  font-size="10.00">seed()</text>
<text xml:space="preserve" text-anchor="start" x="4" y="-68"  font-size="10.00">+</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-68"  font-size="10.00">to_json()</text>
<text xml:space="preserve" text-anchor="start" x="4.38" y="-52.75"  font-size="10.00">#</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-52.75"  font-size="10.00">make_resource_guard()</text>
<text xml:space="preserve" text-anchor="start" x="4.38" y="-37.5"  font-size="10.00">#</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-37.5"  font-size="10.00">create_from_callback()</text>
<text xml:space="preserve" text-anchor="start" x="4.38" y="-22.25"  font-size="10.00">#</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-22.25"  font-size="10.00">borrow_impl()</text>
<text xml:space="preserve" text-anchor="start" x="4.38" y="-7"  font-size="10.00">#</text>
<text xml:space="preserve" text-anchor="start" x="14" y="-7"  font-size="10.00">return_to_pool()</text>
</a>
</g>
</g>
</g>
</svg>
</span>
<span class="uml-diagram-figcaption" style="display: block; text-align: center; font-style: italic; margin-top: 0.5em;">Figure: GraphViz UML Collaboration diagram</span>
</span>
</div>

</div>
</div>

## Member Functions Summary

### Constructors & Destructors

<table class="api-summary-table" markdown="1">
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1a0909de3000c0be9da4fd96f120f8b9e7"><strong>resource_pool</strong></a> ((resource_pool &amp;)=delete)
      <div class="mdesc">Copy constructor is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1a3e13115fd9102b69c57df805675f8572"><strong>resource_pool</strong></a> ((resource_pool &amp;&amp;src)=delete)
      <div class="mdesc">Move constructor is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1a1ad6399df589bfd554f3e32078e06512"><strong>resource_pool</strong></a> (<div class="param-wrap">uint8_t init_capacity=resource_pool_limits::DefaultCapacity,</div><div class="param-wrap">std::function&lt; void(T &amp;)&gt; &amp;&amp;on_shutdown_callback={}</div>)
      <div class="mdesc">Constructs a resource pool with an optional cleanup callback.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1aec9b588016ce92d69bd21688c74b5bdb"><strong>resource_pool</strong></a> (<div class="param-wrap">std::function&lt; void(T &amp;)&gt; &amp;&amp;on_shutdown_callback</div>)
      <div class="mdesc">Constructs a resource pool with only cleanup callback.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1aae9455993d18cc191173f87ed435ee38"><strong>~resource_pool</strong></a> ()
      <div class="mdesc">Destructor - cleans up all resources in the pool.</div>
    </td>
  </tr>
</table>

### Core Accessors & Modifiers

<table class="api-summary-table" markdown="1">
  <tr>
    <td class="memtype"><code>resource_pool &amp;</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1af0e172abce0481219076b29c6b9610a9"><strong>operator=</strong></a> ((resource_pool &amp;)=delete)
      <div class="mdesc">Copy assignment operator is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_pool &amp;</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1a17f548e5e73a6e6a2926cf4555742d89"><strong>operator=</strong></a> ((resource_pool &amp;&amp;src)=delete)
      <div class="mdesc">Move assignment operator is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1ae19dc46cd16d9f9594d74ffd14f9881c"><strong>set_factory_callback</strong></a> (F &amp;&amp;f)
      <div class="mdesc">Sets the factory used by try_borrow_create() when no resource is available.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>pool_error</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1ad2d417ee4983a95c3a365d2b5fec377b"><strong>clear</strong></a> ()
      <div class="mdesc">Clears all resources from the pool.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>auto</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1a5711ec0d9ded4266a521f54a62830189"><strong>size</strong></a> (() const)
      <div class="mdesc">Gets the current size of the pool.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_guard&lt; T &gt;</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1a68145b72a23eb91af6ed2bdce3cf980f"><strong>try_borrow</strong></a> (<div class="param-wrap">std::chrono::nanoseconds timeout={}</div>)
      <div class="mdesc">Borrows an available resource without creating one.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_guard&lt; T &gt;</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1a4e42d634ae61df8943c91fc444c5d2b2"><strong>try_borrow_create</strong></a> (<div class="param-wrap">std::chrono::nanoseconds timeout={}</div>)
      <div class="mdesc">Borrows an available resource or creates one through the factory.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>pool_error</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1a694d99ee21becce11d5d01f5eda13aa9"><strong>seed</strong></a> (Args &amp;&amp;... args)
      <div class="mdesc">Adds a resource to the pool by constructing it in-place.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>pool_error</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1aeb57311374cb1d1369b6c0fc8f7f4916"><strong>seed</strong></a> (T &amp;&amp;item)
      <div class="mdesc">Adds a resource to the pool by moving it.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>nlohmann::json</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__pool_1abb45bf3587651539bf8d5bf8dbb00d0d"><strong>to_json</strong></a> (() const)
      <div class="mdesc">Serializes pool statistics to JSON.</div>
    </td>
  </tr>
</table>

## Member Function Documentation

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1a0909de3000c0be9da4fd96f120f8b9e7" markdown="1">
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

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1a3e13115fd9102b69c57df805675f8572" markdown="1">
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

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1af0e172abce0481219076b29c6b9610a9" markdown="1">
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

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1a17f548e5e73a6e6a2926cf4555742d89" markdown="1">
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

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1a1ad6399df589bfd554f3e32078e06512" markdown="1">
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
<table class="params" markdown="0">
  <tr>
    <td class="paramtype"><code>uint8_t</code></td>
    <td class="paramname">init_capacity</td>
    <td class="paramdesc">Initial capacity of the pool </td>
  </tr>
  <tr>
    <td class="paramtype"><code>std::function< void(T &)> &&</code></td>
    <td class="paramname">on_shutdown_callback</td>
    <td class="paramdesc">Optional cleanup callback invoked on destruction</td>
  </tr>
</table>

!!! note
    Register a factory separately with `set_factory_callback()`. Capacity is clamped to [MinimumCapacity, MaxCapacity] but does not enforce a maximum number of seeded or factory-created resources.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1aec9b588016ce92d69bd21688c74b5bdb" markdown="1">
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
<table class="params" markdown="0">
  <tr>
    <td class="paramtype"><code>std::function< void(T &)> &&</code></td>
    <td class="paramname">on_shutdown_callback</td>
    <td class="paramdesc">Cleanup callback invoked on destruction</td>
  </tr>
</table>

!!! note
    Uses the default capacity. The cleanup callback is invoked for resources available during `clear()` or destruction.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1aae9455993d18cc191173f87ed435ee38" markdown="1">
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


!!! note
    Exceptions derived from std::exception in the cleanup callback are caught and written to stderr.


!!! warning
    Guards borrowed from this pool must be destroyed before the pool.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1ae19dc46cd16d9f9594d74ffd14f9881c" markdown="1">
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
<table class="params" markdown="0">
  <tr>
    <td class="paramname">F</td>
    <td class="paramdesc">Callable type invokable with no arguments returning <code>`resource_guard`<T></code> or <code>T</code> </td>
  </tr>
</table>

!!! note
    Safe to call concurrently with `borrow_impl()`: assignment is synchronized under m_pool_lock, matching the read sites in `borrow_impl()`. A borrow in flight may still use the factory that was registered just before or after this call (no ordering is guaranteed relative to a specific concurrent borrow), but the read/write of the underlying std::function is race-free.


!!! warning
    The callback must not call methods on this pool.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1ad2d417ee4983a95c3a365d2b5fec377b" markdown="1">
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
<div class="memdoc-section-title">Example:</div>

```cpp
// Source: tests/doxygen_examples.cpp:L58-L64
    siddiqsoft::arrp::resource_pool<std::string> pool;
    pool.seed("A");
    pool.seed("B");
    
    // Empties the pool completely. Any resources currently borrowed by guards 
    // will be destroyed rather than returned upon guard destruction.
    pool.clear();
```

Removes currently available resources and invokes the cleanup callback for each. Borrowed resources can return after `clear()` completes.

<div class="memdoc-section-title">Returns</div>
`pool_error::Ok`


!!! note
    The cleanup callback runs under the pool lock. Exceptions derived from std::exception are caught and written to stderr. Non-blocking by design: if a concurrent borrow has already claimed a resource's semaphore permit but not yet popped it from the pool (it is waiting on the same lock `clear()` holds), that item is left in place for the borrower rather than drained here. This avoids a deadlock; it means a racing `clear()` call is not guaranteed to empty every resource that was visible to `size()` just before it ran.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1a5711ec0d9ded4266a521f54a62830189" markdown="1">
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
<div class="memdoc-section-title">Example:</div>

```cpp
// Source: tests/doxygen_examples.cpp:L71-L76
    siddiqsoft::arrp::resource_pool<int> pool(10);
    pool.seed(1);
    pool.seed(2);
    
    // size() returns the total number of resources (both idle in queue and currently borrowed)
    std::cout << "Total resources tracked: " << pool.size() << std::endl; // Outputs 2
```

<div class="memdoc-section-title">Returns</div>
Number of currently available resources


!!! note
    Does not include checked-out resources

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1a68145b72a23eb91af6ed2bdce3cf980f" markdown="1">
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
<div class="memdoc-section-title">Example:</div>

```cpp
// Source: tests/doxygen_examples.cpp:L23-L33
    siddiqsoft::arrp::resource_pool<int> pool;
    pool.seed(42);
    
    {
        // Borrow the resource. It is removed from the pool queue.
        auto guard = pool.try_borrow();
        if (guard.is_valid()) {
            std::cout << "Borrowed: " << guard.get() << std::endl;
        }
        // When 'guard' goes out of scope, the resource is automatically returned to the pool.
    }
```

<div class="memdoc-section-title">Parameters</div>
<table class="params" markdown="0">
  <tr>
    <td class="paramtype"><code>std::chrono::nanoseconds</code></td>
    <td class="paramname">timeout</td>
    <td class="paramdesc">Maximum time to wait; zero performs a non-blocking attempt. </td>
  </tr>
</table>
<div class="memdoc-section-title">Returns</div>
A valid scoped resource, or an invalid one with NoMoreResources, Timeout, ShutdownInitiated, or Unknown set as its error.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1a4e42d634ae61df8943c91fc444c5d2b2" markdown="1">
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
<div class="memdoc-section-title">Example:</div>

```cpp
// Source: tests/doxygen_examples.cpp:L40-L51
    siddiqsoft::arrp::resource_pool<int> pool(5);
    
    // Set a factory callback to generate missing resources
    pool.set_factory_callback([](auto& p) {
        return std::make_unique<int>(99);
    });
    
    // The pool is currently empty, so try_borrow_create will invoke the factory
    auto guard = pool.try_borrow_create();
    if (guard.is_valid()) {
        std::cout << "Created on demand: " << guard.get() << std::endl;
    }
```

<div class="memdoc-section-title">Parameters</div>
<table class="params" markdown="0">
  <tr>
    <td class="paramtype"><code>std::chrono::nanoseconds</code></td>
    <td class="paramname">timeout</td>
    <td class="paramdesc">Maximum time to wait; zero performs a non-blocking attempt. </td>
  </tr>
</table>
<div class="memdoc-section-title">Returns</div>
A valid scoped resource, or an invalid one when shutdown or an implementation or factory error prevents borrowing.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1a694d99ee21becce11d5d01f5eda13aa9" markdown="1">
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
<div class="memdoc-section-title">Example:</div>

```cpp
// Source: tests/doxygen_examples.cpp:L9-L16
    siddiqsoft::arrp::resource_pool<std::string> pool(10);
    
    // Seed by constructing in-place
    pool.seed(5, 'A'); // "AAAAA"
    
    // Seed by moving an existing object
    std::string existing = "Hello";
    pool.seed(std::move(existing));
```

<div class="memdoc-section-title">Template Parameters</div>
<table class="params" markdown="0">
  <tr>
    <td class="paramname">Args</td>
    <td class="paramdesc">Types of arguments to forward to T's constructor </td>
  </tr>
</table>
<div class="memdoc-section-title">Parameters</div>
<table class="params" markdown="0">
  <tr>
    <td class="paramtype"><code>Args &&...</code></td>
    <td class="paramname">args</td>
    <td class="paramdesc">Arguments to forward to T's constructor for in-place construction </td>
  </tr>
</table>
<div class="memdoc-section-title">Returns</div>
`pool_error::Ok`, or `pool_error::ShutdownInitiated` during destruction


!!! note
    Resource is constructed in-place Does not enforce the configured capacity. Do not use this to return a borrowed resource; guards return resources automatically.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1aeb57311374cb1d1369b6c0fc8f7f4916" markdown="1">
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
<table class="params" markdown="0">
  <tr>
    <td class="paramtype"><code>T &&</code></td>
    <td class="paramname">item</td>
    <td class="paramdesc">The resource to add (moved) </td>
  </tr>
</table>
<div class="memdoc-section-title">Returns</div>
`pool_error::Ok`, or `pool_error::ShutdownInitiated` during destruction


!!! note
    Resource is moved into the pool Does not enforce the configured capacity. Do not use this to return a borrowed resource; guards return resources automatically.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__pool_1abb45bf3587651539bf8d5bf8dbb00d0d" markdown="1">
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

Serializes pool statistics to JSON.
<div class="memdoc-section-title">Example:</div>

```cpp
// Source: tests/doxygen_examples.cpp:L71-L79
    siddiqsoft::arrp::resource_pool<int> pool(10);
    pool.seed(1);
    pool.seed(2);
    
    auto borrowed = pool.try_borrow();
    
    // Export telemetry statistics to a JSON object
    nlohmann::json stats = pool.to_json();
    std::cout << stats.dump(4) << std::endl;
```

Returns a JSON object containing pool statistics and configuration. Only available if nlohmann/json.hpp is included before this header file.

<div class="memdoc-section-title">Returns</div>
A JSON object containing a snapshot of pool statistics


!!! note
    Available only when nlohmann/json.hpp was included before this header.

</div>
</div>

## Source Code Reference

- Header: [`include/siddiqsoft/private/resource_pool.hpp`](https://github.com/SiddiqSoft/arrp/blob/master/include/siddiqsoft/private/resource_pool.hpp#L77)
