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
</table>

## Member Function Details

### <a id="resource_pool"></a>`resource_pool`

```cpp
void resource_pool::resource_pool(resource_pool &)=delete;
```

Copy constructor is deleted.

### <a id="resource_pool"></a>`resource_pool`

```cpp
void resource_pool::resource_pool(resource_pool &&src)=delete;
```

Move constructor is deleted.

### <a id="operator="></a>`operator=`

```cpp
resource_pool & resource_pool::operator=(resource_pool &)=delete;
```

Copy assignment operator is deleted.

### <a id="operator="></a>`operator=`

```cpp
resource_pool & resource_pool::operator=(resource_pool &&src)=delete;
```

Move assignment operator is deleted.

### <a id="resource_pool"></a>`resource_pool`

```cpp
void resource_pool::resource_pool(uint8_t init_capacity=resource_pool_limits::DefaultCapacity, std::function< void(T &)> &&on_shutdown_callback={});
```

Constructs a resource pool with an optional cleanup callback.

### <a id="resource_pool"></a>`resource_pool`

```cpp
void resource_pool::resource_pool(std::function< void(T &)> &&on_shutdown_callback);
```

Constructs a resource pool with only cleanup callback.

### <a id="~resource_pool"></a>`~resource_pool`

```cpp
void resource_pool::~resource_pool();
```

Destructor - cleans up all resources in the pool.

### <a id="set_factory_callback"></a>`set_factory_callback`

```cpp
void resource_pool::set_factory_callback(F &&f);
```

Sets the factory used by try_borrow_create() when no resource is available.

### <a id="clear"></a>`clear`

```cpp
pool_error resource_pool::clear();
```

Clears all resources from the pool.

### <a id="size"></a>`size`

```cpp
auto resource_pool::size() const;
```

Gets the current size of the pool.

### <a id="try_borrow"></a>`try_borrow`

```cpp
resource_guard< T > resource_pool::try_borrow(std::chrono::nanoseconds timeout={});
```

Borrows an available resource without creating one.

### <a id="try_borrow_create"></a>`try_borrow_create`

```cpp
resource_guard< T > resource_pool::try_borrow_create(std::chrono::nanoseconds timeout={});
```

Borrows an available resource or creates one through the factory.

### <a id="seed"></a>`seed`

```cpp
pool_error resource_pool::seed(Args &&... args);
```

Adds a resource to the pool by constructing it in-place.

### <a id="seed"></a>`seed`

```cpp
pool_error resource_pool::seed(T &&item);
```

Adds a resource to the pool by moving it.

## Source Code Reference

- Header: [`include/siddiqsoft/private/resource_pool.hpp`](https://github.com/SiddiqSoft/arrp/blob/master/include/siddiqsoft/private/resource_pool.hpp#L88)
