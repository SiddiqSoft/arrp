# siddiqsoft::arrp::resource_guard Class Reference

<div class="grid" markdown="1">
<div class="api-intro-col" markdown="1">
<div class="api-header-block">
  <div class="api-module-name">Namespace siddiqsoft</div>
  <div class="api-header-file">#include &lt;siddiqsoft/private/resource_guard.hpp&gt;</div>
</div>

RAII wrapper for managing resource lifecycle in a resource pool.

</div>
<div class="api-diag-col" markdown="1">

**Class Hierarchy & Inheritance**

The following UML class diagram highlights `siddiqsoft::arrp::resource_guard` and its direct relationships. Click the node to navigate to its source file on GitHub.

<!-- @@uml-diag:resource_guard -->

</div>
</div>

## Member Functions Summary

### Constructors & Destructors

<table class="api-summary-table">
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1af5949392e4dac60ef492b5585d4ac35b"><strong>resource_guard</strong></a> ((const resource_guard &amp;)=delete)
      <div class="mdesc">Copy constructor is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a6f177e84230f911f734db42ab5998e42"><strong>resource_guard</strong></a> (const pool_error &amp;err)
      <div class="mdesc">Constructs an invalid guard carrying a borrow error.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1ad0c276206bf4f34c86d054cc44c9b431"><strong>resource_guard</strong></a> (resource_guard &amp;&amp;src)
      <div class="mdesc">Move constructor.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a69b8647595ad0631073515864038e6af"><strong>~resource_guard</strong></a> (() noexcept)
      <div class="mdesc">Destructor - invokes callback to handle resource return or abandonment.</div>
    </td>
  </tr>
</table>

### Core Accessors & Modifiers

<table class="api-summary-table">
  <tr>
    <td class="memtype"><code>resource_guard &amp;</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1acb1054fba7fd688648f517925b1d6219"><strong>operator=</strong></a> ((const resource_guard &amp;)=delete)
      <div class="mdesc">Copy assignment operator is deleted.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_guard &amp;</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1adf7d8051c7b0eb8ae4f8b96780266996"><strong>operator=</strong></a> (resource_guard &amp;&amp;src)
      <div class="mdesc">Move assignment operator.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>T &amp;</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1ac2e51222141722283fa988b5621f8b38"><strong>operator*</strong></a> ()
      <div class="mdesc">Dereference operator to access the wrapped resource.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>T *</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a9f23b9bc4f7682f1a61d91966cb416bc"><strong>operator-></strong></a> ()
      <div class="mdesc">Pointer-like access to the wrapped resource.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>const T *</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1af478b8c10cbbc642a39a84b758884fc7"><strong>operator-></strong></a> (() const)
      <div class="mdesc">Provides const pointer-like access to the wrapped resource.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1ae271c2e559ef43933a3f5da356b5f0ff"><strong>operator T&</strong></a> (() &amp;)
      <div class="mdesc">Explicit conversion to resource reference.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a77ffadcb9626e0ce57aa273ca108b8ab"><strong>operator const T &</strong></a> (() const &amp;)
      <div class="mdesc">Provides a const reference to the wrapped resource.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a44091eca34cb9c5e7e562d511f0d06fa"><strong>operator bool</strong></a> (() const noexcept)
      <div class="mdesc">Tests whether the guard holds a resource eligible for return.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a44f989d5565149e31d9ecbf1f858bdf0"><strong>operator InnerType</strong></a> (() const)
      <div class="mdesc">Converts through a conversion supplied by the stored resource type.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>resource_guard &amp;</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a9c668bed1a5a10556f237ecc0abc54ad"><strong>operator=</strong></a> (T &amp;&amp;src)
      <div class="mdesc">Assignment operator for resource value.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>void</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1abda03d642ffe1096d182ae43914681e4"><strong>invalidate</strong></a> ()
      <div class="mdesc">Marks the resource as invalid (abandoned).</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>bool</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a899a309271834f7a616cf86e2753d52b"><strong>is_valid</strong></a> (() const)
      <div class="mdesc">Checks if the resource is valid.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>auto &amp;</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a19fa5028409bbd0e300ac9265d530456"><strong>set_error</strong></a> (pool_error err)
      <div class="mdesc">Sets the error reported by error().</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>pool_error</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1ab806fff0061d73685a71ee835fe22b49"><strong>error</strong></a> (() const)
      <div class="mdesc">Gets the error associated with this guard.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>bool</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1aaaa62a004ab8cecaa5be5406b58ba2ff"><strong>has_value</strong></a> (() const)
      <div class="mdesc">Tests whether the guard holds a valid resource.</div>
    </td>
  </tr>
  <tr>
    <td class="memtype"><code>nlohmann::json</code></td>
    <td class="memitemleft"><a href="#classsiddiqsoft_1_1arrp_1_1resource__guard_1a169de596197570ae4cf3d0b590081097"><strong>to_json</strong></a> (() const)
      <div class="mdesc">Serializes the resource_guard to JSON.</div>
    </td>
  </tr>
</table>

## Member Function Documentation

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1af5949392e4dac60ef492b5585d4ac35b" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">resource_guard()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_guard::resource_guard(const resource_guard &)=delete;
```

</div>
<div class="memdoc" markdown="1">

Copy constructor is deleted.
`resource_guard` is move-only to prevent resource ownership ambiguity and ensure proper RAII semantics. Only one `resource_guard` can own a resource.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1acb1054fba7fd688648f517925b1d6219" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator=()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
resource_guard & resource_guard::operator=(const resource_guard &)=delete;
```

</div>
<div class="memdoc" markdown="1">

Copy assignment operator is deleted.
Copy assignment is not allowed to maintain move-only semantics and prevent resource ownership ambiguity.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a6f177e84230f911f734db42ab5998e42" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">resource_guard()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_guard::resource_guard(const pool_error &err);
```

</div>
<div class="memdoc" markdown="1">

Constructs an invalid guard carrying a borrow error.
<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>err</code> &mdash; Error reported by `error()` </li>
</ul>

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1ad0c276206bf4f34c86d054cc44c9b431" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">resource_guard()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_guard::resource_guard(resource_guard &&src);
```

</div>
<div class="memdoc" markdown="1">

Move constructor.
Transfers ownership from another `resource_guard` to this one. The source is invalidated to prevent double-return.

<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>src</code> &mdash; The source `resource_guard` to move from</li>
</ul>

<div class="memdoc-section-title">Note</div>

The source's callback is cleared to prevent double-return 

<div class="memdoc-section-title">Note</div>

The source is marked as invalid 

<div class="memdoc-section-title">Note</div>

This constructor is using new syntax for noexcept specification based on the move-constructibility of T and the callback function.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1adf7d8051c7b0eb8ae4f8b96780266996" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator=()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
resource_guard & resource_guard::operator=(resource_guard &&src);
```

</div>
<div class="memdoc" markdown="1">

Move assignment operator.
Transfers ownership from another `resource_guard` to this one. Before taking ownership, the currently-held resource (if valid) is returned to the pool via the putback callback. The source is then invalidated to prevent double-return.

<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>src</code> &mdash; The source `resource_guard` to move from </li>
</ul>

<div class="memdoc-section-title">Returns</div>

Reference to this `resource_guard`

<div class="memdoc-section-title">Note</div>

Self-assignment is checked via pointer comparison 

<div class="memdoc-section-title">Note</div>

The currently-held resource is returned to the pool before overwrite 

<div class="memdoc-section-title">Note</div>

The source's callback is cleared to prevent double-return 

<div class="memdoc-section-title">Note</div>

The source is marked as invalid after the move 

<div class="memdoc-section-title">Note</div>

NOT noexcept: T's move-assignment may throw; declaring noexcept here would call std::terminate if T::operator=(T&&) throws after the putback callback has already fired (state would be inconsistent).

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a69b8647595ad0631073515864038e6af" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">~resource_guard()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_guard::~resource_guard() noexcept;
```

</div>
<div class="memdoc" markdown="1">

Destructor - invokes callback to handle resource return or abandonment.
Invokes the putback callback if one exists, passing the resource and its validity status. The callback is responsible for deciding whether to return the resource to the pool (if valid) or discard it (if invalid). Exceptions from the callback are caught and logged to stderr.

<div class="memdoc-section-title">Note</div>

Noexcept: Exceptions are caught and logged, not propagated 

<div class="memdoc-section-title">Note</div>

The callback is always invoked if set, regardless of validity 

<div class="memdoc-section-title">Note</div>

The callback receives the validity flag to make the appropriate decision 

<div class="memdoc-section-title">Note</div>

The callback is cleared after invocation 

<div class="memdoc-section-title">Note</div>

The resource is marked as invalid after callback invocation

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1ac2e51222141722283fa988b5621f8b38" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator*()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
T & resource_guard::operator*();
```

</div>
<div class="memdoc" markdown="1">

Dereference operator to access the wrapped resource.
<div class="memdoc-section-title">Returns</div>

Reference to the wrapped resource

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a9f23b9bc4f7682f1a61d91966cb416bc" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator->()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
T * resource_guard::operator->();
```

</div>
<div class="memdoc" markdown="1">

Pointer-like access to the wrapped resource.
<div class="memdoc-section-title">Example:</div>

 

<div class="memdoc-section-title">Returns</div>

Pointer to the wrapped resource, or nullptr if invalid 

<div class="memdoc-section-title">Note</div>

Returns nullptr if resource is invalid

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1af478b8c10cbbc642a39a84b758884fc7" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator->()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
const T * resource_guard::operator->() const;
```

</div>
<div class="memdoc" markdown="1">

Provides const pointer-like access to the wrapped resource.
<div class="memdoc-section-title">Returns</div>

The resource address, or nullptr if the guard is invalid.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1ae271c2e559ef43933a3f5da356b5f0ff" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator T&()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_guard::operator T&() &;
```

</div>
<div class="memdoc" markdown="1">

Explicit conversion to resource reference.
<div class="memdoc-section-title">Returns</div>

Reference to the wrapped resource

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a77ffadcb9626e0ce57aa273ca108b8ab" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator const T &()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_guard::operator const T &() const &;
```

</div>
<div class="memdoc" markdown="1">

Provides a const reference to the wrapped resource.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a44091eca34cb9c5e7e562d511f0d06fa" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator bool()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_guard::operator bool() const noexcept;
```

</div>
<div class="memdoc" markdown="1">

Tests whether the guard holds a resource eligible for return.
<div class="memdoc-section-title">Returns</div>

true when the guard is valid

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a44f989d5565149e31d9ecbf1f858bdf0" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator InnerType()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_guard::operator InnerType() const;
```

</div>
<div class="memdoc" markdown="1">

Converts through a conversion supplied by the stored resource type.
<div class="memdoc-section-title">Template Parameters</div>

<ul>
  <li><code>InnerType</code> &mdash; Requested conversion target. </li>
</ul>

<div class="memdoc-section-title">Returns</div>

The result of converting the stored resource to InnerType.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a9c668bed1a5a10556f237ecc0abc54ad" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">operator=()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
resource_guard & resource_guard::operator=(T &&src);
```

</div>
<div class="memdoc" markdown="1">

Assignment operator for resource value.
Replaces the held resource value in place. The old resource is returned to the pool, and the guard retains ownership of the new resource, which will be returned to the pool when destroyed.

<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>src</code> &mdash; The new resource value (moved) </li>
</ul>

<div class="memdoc-section-title">Returns</div>

Reference to this `resource_guard`

<div class="memdoc-section-title">Note</div>

Returns existing resource to pool before taking ownership of new resource.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1abda03d642ffe1096d182ae43914681e4" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">invalidate()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
void resource_guard::invalidate();
```

</div>
<div class="memdoc" markdown="1">

Marks the resource as invalid (abandoned).
<div class="memdoc-section-title">Example:</div>



<div class="memdoc-section-title">Note</div>

Virtual for interface consistency, but `resource_guard` is <code>final</code>, so there is currently no derived class to override this. 

<div class="memdoc-section-title">Note</div>

The callback is still invoked; only the validity flag changes 

<div class="memdoc-section-title">Note</div>

Typically called when the resource is corrupted, moved out, or consumed

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a899a309271834f7a616cf86e2753d52b" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">is_valid()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
bool resource_guard::is_valid() const;
```

</div>
<div class="memdoc" markdown="1">

Checks if the resource is valid.
<div class="memdoc-section-title">Returns</div>

true if the resource is valid and will be returned to pool, false otherwise

<div class="memdoc-section-title">Note</div>

Virtual for interface consistency, but `resource_guard` is <code>final</code>, so there is currently no derived class to override this. 

<div class="memdoc-section-title">Note</div>

Const: Does not modify the resource

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a19fa5028409bbd0e300ac9265d530456" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">set_error()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
auto & resource_guard::set_error(pool_error err);
```

</div>
<div class="memdoc" markdown="1">

Sets the error reported by error().
<div class="memdoc-section-title">Parameters</div>

<ul>
  <li><code>err</code> &mdash; Error code to store. </li>
</ul>

<div class="memdoc-section-title">Returns</div>

This guard.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1ab806fff0061d73685a71ee835fe22b49" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">error()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
pool_error resource_guard::error() const;
```

</div>
<div class="memdoc" markdown="1">

Gets the error associated with this guard.
<div class="memdoc-section-title">Returns</div>

The stored error code; valid guards normally report `pool_error::Ok`.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1aaaa62a004ab8cecaa5be5406b58ba2ff" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">has_value()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
bool resource_guard::has_value() const;
```

</div>
<div class="memdoc" markdown="1">

Tests whether the guard holds a valid resource.
<div class="memdoc-section-title">Returns</div>

true when `is_valid()` would return true.

</div>
</div>

<div class="memitem" id="classsiddiqsoft_1_1arrp_1_1resource__guard_1a169de596197570ae4cf3d0b590081097" markdown="1">
<div class="memitem-header">
  <span class="memitem-diamond">&#9670;</span>
  <h4 class="memitem-title">to_json()</h4>
</div>
<div class="memproto" markdown="1">

```cpp
nlohmann::json resource_guard::to_json() const;
```

</div>
<div class="memdoc" markdown="1">

Serializes the resource_guard to JSON.
Returns a JSON object containing the resource state and validity. Only available if nlohmann/json.hpp is included before this header.

<div class="memdoc-section-title">Returns</div>

JSON object with:
_typver: Type and version string ("siddiqsoft.arrp.resource_guard/1.0.0")
valid: Whether the resource is valid (boolean)
value: The resource value (if serializable, otherwise "-noserializer-")



<div class="memdoc-section-title">Note</div>

Available only when nlohmann/json.hpp was included before this header. 

<div class="memdoc-section-title">Note</div>

If T is not serializable, value is set to "-noserializer-"

<div class="memdoc-section-title">JSON Schema:</div>

{
"_typver":"siddiqsoft.arrp.resource_guard/1.0.0",
"valid":true,
"value":<resource_value>
}

</div>
</div>

## Source Code Reference

- Header: [`include/siddiqsoft/private/resource_guard.hpp`](https://github.com/SiddiqSoft/arrp/blob/master/include/siddiqsoft/private/resource_guard.hpp#L99)
