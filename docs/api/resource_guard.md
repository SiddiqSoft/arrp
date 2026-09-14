# siddiqsoft::arrp::resource_guard

<div class="grid" markdown="1">
<div class="api-intro-col" markdown="1">
<div class="api-header-block">
  <div class="api-module-name">Namespace siddiqsoft</div>
  <div class="api-header-file">#include &lt;siddiqsoft/private/resource_guard.hpp&gt;</div>
</div>

`resource_guard` component of `arrp`.

</div>
<div class="api-diag-col" markdown="1">

**Class Hierarchy & Inheritance**

The following UML class diagram highlights `siddiqsoft::arrp::resource_guard` and its direct relationships. Click the node to navigate to its source file on GitHub.

<!-- @@uml-diag:resource_guard -->

</div>
</div>

## Member Functions Summary

<table class="api-summary-table">
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#resource_guard"><strong>resource_guard</strong></a> ((const resource_guard &amp;)=delete)
      <div class="mdesc">Copy constructor is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_guard &amp;</code></td>
    <td class="memitemleft"><a href="#operator="><strong>operator=</strong></a> ((const resource_guard &amp;)=delete)
      <div class="mdesc">Copy assignment operator is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#resource_guard"><strong>resource_guard</strong></a> (const pool_error &amp;err)
      <div class="mdesc">Constructs an invalid guard carrying a borrow error.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#resource_guard"><strong>resource_guard</strong></a> (resource_guard &amp;&amp;src)
      <div class="mdesc">Move constructor.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_guard &amp;</code></td>
    <td class="memitemleft"><a href="#operator="><strong>operator=</strong></a> (resource_guard &amp;&amp;src)
      <div class="mdesc">Move assignment operator.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#~resource_guard"><strong>~resource_guard</strong></a> (() noexcept)
      <div class="mdesc">Destructor - invokes callback to handle resource return or abandonment.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>T &amp;</code></td>
    <td class="memitemleft"><a href="#operator*"><strong>operator*</strong></a> ()
      <div class="mdesc">Dereference operator to access the wrapped resource.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>T *</code></td>
    <td class="memitemleft"><a href="#operator->"><strong>operator-></strong></a> ()
      <div class="mdesc">Pointer-like access to the wrapped resource.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>const T *</code></td>
    <td class="memitemleft"><a href="#operator->"><strong>operator-></strong></a> (() const)
      <div class="mdesc">Provides const pointer-like access to the wrapped resource.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#operator t&"><strong>operator T&</strong></a> (() &amp;)
      <div class="mdesc">Explicit conversion to resource reference.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#operator const t &"><strong>operator const T &</strong></a> (() const &amp;)
      <div class="mdesc">Provides a const reference to the wrapped resource.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#operator bool"><strong>operator bool</strong></a> (() const noexcept)
      <div class="mdesc">Tests whether the guard holds a resource eligible for return.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#operator innertype"><strong>operator InnerType</strong></a> (() const)
      <div class="mdesc">Converts through a conversion supplied by the stored resource type.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_guard &amp;</code></td>
    <td class="memitemleft"><a href="#operator="><strong>operator=</strong></a> (T &amp;&amp;src)
      <div class="mdesc">Assignment operator for resource value.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#invalidate"><strong>invalidate</strong></a> ()
      <div class="mdesc">Member function.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>bool</code></td>
    <td class="memitemleft"><a href="#is_valid"><strong>is_valid</strong></a> (() const)
      <div class="mdesc">Checks if the resource is valid.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>auto &amp;</code></td>
    <td class="memitemleft"><a href="#set_error"><strong>set_error</strong></a> (pool_error err)
      <div class="mdesc">Sets the error reported by error().</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>pool_error</code></td>
    <td class="memitemleft"><a href="#error"><strong>error</strong></a> (() const)
      <div class="mdesc">Gets the error associated with this guard.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>bool</code></td>
    <td class="memitemleft"><a href="#has_value"><strong>has_value</strong></a> (() const)
      <div class="mdesc">Tests whether the guard holds a valid resource.</div>
    </td>
  </tr>
</table>

## Member Function Details

### <a id="resource_guard"></a>`resource_guard`

```cpp
void resource_guard::resource_guard(const resource_guard &)=delete;
```

Copy constructor is deleted.

### <a id="operator="></a>`operator=`

```cpp
resource_guard & resource_guard::operator=(const resource_guard &)=delete;
```

Copy assignment operator is deleted.

### <a id="resource_guard"></a>`resource_guard`

```cpp
void resource_guard::resource_guard(const pool_error &err);
```

Constructs an invalid guard carrying a borrow error.

### <a id="resource_guard"></a>`resource_guard`

```cpp
void resource_guard::resource_guard(resource_guard &&src);
```

Move constructor.

### <a id="operator="></a>`operator=`

```cpp
resource_guard & resource_guard::operator=(resource_guard &&src);
```

Move assignment operator.

### <a id="~resource_guard"></a>`~resource_guard`

```cpp
void resource_guard::~resource_guard() noexcept;
```

Destructor - invokes callback to handle resource return or abandonment.

### <a id="operator*"></a>`operator*`

```cpp
T & resource_guard::operator*();
```

Dereference operator to access the wrapped resource.

### <a id="operator->"></a>`operator->`

```cpp
T * resource_guard::operator->();
```

Pointer-like access to the wrapped resource.

### <a id="operator->"></a>`operator->`

```cpp
const T * resource_guard::operator->() const;
```

Provides const pointer-like access to the wrapped resource.

### <a id="operator t&"></a>`operator T&`

```cpp
void resource_guard::operator T&() &;
```

Explicit conversion to resource reference.

### <a id="operator const t &"></a>`operator const T &`

```cpp
void resource_guard::operator const T &() const &;
```

Provides a const reference to the wrapped resource.

### <a id="operator bool"></a>`operator bool`

```cpp
void resource_guard::operator bool() const noexcept;
```

Tests whether the guard holds a resource eligible for return.

### <a id="operator innertype"></a>`operator InnerType`

```cpp
void resource_guard::operator InnerType() const;
```

Converts through a conversion supplied by the stored resource type.

### <a id="operator="></a>`operator=`

```cpp
resource_guard & resource_guard::operator=(T &&src);
```

Assignment operator for resource value.

### <a id="invalidate"></a>`invalidate`

```cpp
void resource_guard::invalidate();
```

Executes component operation.

### <a id="is_valid"></a>`is_valid`

```cpp
bool resource_guard::is_valid() const;
```

Checks if the resource is valid.

### <a id="set_error"></a>`set_error`

```cpp
auto & resource_guard::set_error(pool_error err);
```

Sets the error reported by error().

### <a id="error"></a>`error`

```cpp
pool_error resource_guard::error() const;
```

Gets the error associated with this guard.

### <a id="has_value"></a>`has_value`

```cpp
bool resource_guard::has_value() const;
```

Tests whether the guard holds a valid resource.

## Source Code Reference

- Header: [`include/siddiqsoft/private/resource_guard.hpp`](https://github.com/SiddiqSoft/arrp/blob/master/include/siddiqsoft/private/resource_guard.hpp#L118)
