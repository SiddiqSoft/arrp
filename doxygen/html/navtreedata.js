/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "arrp", "index.html", [
    [ "arrp - Auto Returning Resource Pool", "index.html", "index" ],
    [ "api", "md_docs_2pages_2api.html", null ],
    [ "API Reference", "api_reference.html", [
      [ "resource_pool&lt;T, SRT&gt;", "api_reference.html#resource_pool_class", [
        [ "Template Parameters", "api_reference.html#rp_template", null ],
        [ "Constructors", "api_reference.html#rp_constructors", null ],
        [ "Methods", "api_reference.html#rp_methods", null ],
        [ "Callbacks", "api_reference.html#rp_callbacks", null ],
        [ "Statistics", "api_reference.html#rp_statistics", null ]
      ] ],
      [ "scoped_resource&lt;T&gt;", "api_reference.html#scoped_resource_class", [
        [ "Template Parameters", "api_reference.html#sr_template", null ],
        [ "Operators", "api_reference.html#sr_operators", null ],
        [ "Methods", "api_reference.html#sr_methods", null ],
        [ "Move Semantics", "api_reference.html#sr_semantics", null ]
      ] ],
      [ "Enumerations", "api_reference.html#enums", [
        [ "auto_add_policy", "api_reference.html#auto_add_policy", null ],
        [ "pool_error", "api_reference.html#pool_error", null ],
        [ "resource_pool_limits", "api_reference.html#resource_pool_limits", null ]
      ] ],
      [ "Concepts", "api_reference.html#concepts_section", [
        [ "NonNumericMoveConstructible&lt;T&gt;", "api_reference.html#non_numeric_move_constructible", null ]
      ] ],
      [ "Usage Patterns", "api_reference.html#usage_patterns", [
        [ "Fixed-Size Pool", "api_reference.html#pattern_fixed_pool", null ],
        [ "Auto-Growing Pool", "api_reference.html#pattern_autogrow", null ],
        [ "Custom Factory", "api_reference.html#pattern_custom_factory", null ],
        [ "Cleanup Callback", "api_reference.html#pattern_cleanup_callback", null ],
        [ "Invalidate Resource", "api_reference.html#pattern_invalidate", null ]
      ] ],
      [ "Thread Safety Details", "api_reference.html#thread_safety_details", null ],
      [ "Performance Considerations", "api_reference.html#performance", null ],
      [ "Limitations", "api_reference.html#limitations", null ]
    ] ],
    [ "mainpage", "md_docs_2pages_2mainpage.html", null ],
    [ "usage_guide", "md_docs_2pages_2usage__guide.html", null ],
    [ "Usage Guide", "usage_guide.html", [
      [ "Getting Started", "usage_guide.html#getting_started", [
        [ "Include Header", "usage_guide.html#include_header", null ],
        [ "Basic Example", "usage_guide.html#basic_example", null ]
      ] ],
      [ "Pool Creation", "usage_guide.html#pool_creation", [
        [ "Simple Pool", "usage_guide.html#create_simple", null ],
        [ "Pool with Factory", "usage_guide.html#create_factory", null ],
        [ "Pool with Cleanup", "usage_guide.html#create_cleanup", null ]
      ] ],
      [ "Borrowing Resources", "usage_guide.html#resource_borrowing", [
        [ "Basic Borrowing", "usage_guide.html#borrow_basic", null ],
        [ "Error Handling", "usage_guide.html#borrow_error", null ],
        [ "Move Resource", "usage_guide.html#borrow_move", null ]
      ] ],
      [ "Seeding Resources", "usage_guide.html#resource_seeding", [
        [ "Seed via Move", "usage_guide.html#seed_move", null ],
        [ "Seed via In-Place Construction", "usage_guide.html#seed_inplace", null ]
      ] ],
      [ "Resource Invalidation", "usage_guide.html#resource_invalidation", [
        [ "Invalidate Resource", "usage_guide.html#invalidate_example", null ],
        [ "Check Validity", "usage_guide.html#check_validity", null ],
        [ "Get Statistics", "usage_guide.html#get_statistics", null ],
        [ "Statistics Fields", "usage_guide.html#statistics_fields", null ]
      ] ],
      [ "Threading", "usage_guide.html#threading", [
        [ "Thread-Safe Operations", "usage_guide.html#thread_safe", null ],
        [ "NOT Thread-Safe", "usage_guide.html#thread_unsafe", null ],
        [ "Correct Threading", "usage_guide.html#thread_correct", null ]
      ] ],
      [ "Advanced Usage", "usage_guide.html#advanced", [
        [ "Custom Scoped Resource", "usage_guide.html#custom_scoped_resource", null ],
        [ "Clear Pool", "usage_guide.html#pool_clear", null ],
        [ "Get Pool Size", "usage_guide.html#pool_size", null ]
      ] ],
      [ "Best Practices", "usage_guide.html#best_practices", [
        [ "Choose Appropriate Capacity", "usage_guide.html#bp_capacity", null ],
        [ "Keep Callbacks Fast", "usage_guide.html#bp_callbacks", null ],
        [ "Don't Call Pool Methods in Callbacks", "usage_guide.html#bp_no_deadlock", null ],
        [ "Always Check Results", "usage_guide.html#bp_error_handling", null ],
        [ "Invalidate Corrupted Resources", "usage_guide.html#bp_invalidate", null ]
      ] ],
      [ "Troubleshooting", "usage_guide.html#troubleshooting", [
        [ "Pool Exhausted", "usage_guide.html#ts_pool_exhausted", null ],
        [ "Deadlock", "usage_guide.html#ts_deadlock", null ],
        [ "Memory Leak", "usage_guide.html#ts_memory_leak", null ]
      ] ]
    ] ],
    [ "Concepts", "concepts.html", "concepts" ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ],
        [ "Typedefs", "functions_type.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ]
    ] ],
    [ "Examples", "examples.html", "examples" ]
  ] ]
];

var NAVTREEINDEX =
[
"Consider-example.html"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';