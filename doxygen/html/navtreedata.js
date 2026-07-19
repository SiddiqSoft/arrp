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
    [ "ARRP - Auto Returning Resource Pool for Modern C++", "index.html", "index" ],
    [ "API Reference", "api.html", null ],
    [ "Examples", "examples.html", [
      [ "Database Connection Pool", "examples.html#ex_database_connections", null ],
      [ "HTTP Client Connection Pool", "examples.html#ex_http_client", null ],
      [ "File Handle Pool", "examples.html#ex_file_handles", null ],
      [ "Thread Pool Integration", "examples.html#ex_thread_pool_integration", null ],
      [ "Custom Resource Factory", "examples.html#ex_custom_factory", null ],
      [ "Pool Monitoring", "examples.html#ex_monitoring", null ],
      [ "Resource Invalidation", "examples.html#ex_resource_invalidation", null ],
      [ "Error Handling", "examples.html#ex_error_handling", null ],
      [ "Scoped Resource Usage", "examples.html#ex_scoped_usage", null ]
    ] ],
    [ "Getting Started", "getting_started.html", [
      [ "Installation", "getting_started.html#installation", null ],
      [ "Compiler Setup", "getting_started.html#compiler_setup", null ],
      [ "Your First Program", "getting_started.html#first_program", null ],
      [ "Common Patterns", "getting_started.html#common_patterns", null ],
      [ "Troubleshooting", "getting_started.html#troubleshooting", null ],
      [ "Next Steps", "getting_started.html#next_steps", null ]
    ] ],
    [ "Quick Reference", "quick_reference.html", [
      [ "Components at a Glance", "quick_reference.html#qr_components", null ],
      [ "Include Files", "quick_reference.html#qr_includes", null ],
      [ "Basic Patterns", "quick_reference.html#qr_basic_patterns", [
        [ "Basic Pool Usage", "quick_reference.html#qr_pattern_basic", null ],
        [ "Custom Factory", "quick_reference.html#qr_pattern_factory", null ],
        [ "Multi-threaded Usage", "quick_reference.html#qr_pattern_multithreaded", null ],
        [ "Resource Invalidation", "quick_reference.html#qr_pattern_invalidate", null ]
      ] ],
      [ "Template Parameters", "quick_reference.html#qr_template_params", null ],
      [ "Common Methods", "quick_reference.html#qr_methods", null ],
      [ "Requirements", "quick_reference.html#qr_requirements", null ],
      [ "Compilation", "quick_reference.html#qr_compilation", null ],
      [ "Tips &amp; Tricks", "quick_reference.html#qr_tips", null ],
      [ "Type Constraints", "quick_reference.html#qr_constraints", null ],
      [ "Common Issues", "quick_reference.html#qr_troubleshooting", null ],
      [ "Performance Tips", "quick_reference.html#qr_performance", null ],
      [ "Quick Examples", "quick_reference.html#qr_examples", null ],
      [ "JSON Output Format", "quick_reference.html#qr_json_output", null ]
    ] ],
    [ "Security Guide", "security.html", [
      [ "Overview", "security.html#security_overview", null ],
      [ "Built-in Security Features", "security.html#security_features", null ],
      [ "Callback Security", "security.html#callback_security", null ],
      [ "Deadlock Prevention", "security.html#deadlock_prevention", null ],
      [ "Locking and Deadlock Prevention", "security.html#locking_considerations", null ],
      [ "Resource Exhaustion Prevention", "security.html#resource_exhaustion", null ],
      [ "Exception Handling", "security.html#exception_handling", null ],
      [ "Thread Safety", "security.html#thread_safety", null ],
      [ "Shutdown Safety", "security.html#shutdown_safety", null ],
      [ "Monitoring and Diagnostics", "security.html#monitoring", null ],
      [ "Security Checklist", "security.html#security_checklist", null ],
      [ "Vulnerability Reporting", "security.html#vulnerability_reporting", null ],
      [ "Additional Resources", "security.html#security_resources", null ],
      [ "FAQ", "security.html#security_faq", null ]
    ] ],
    [ "Usage Guide", "usage_guide.html", [
      [ "Overview", "usage_guide.html#overview", null ],
      [ "Resource Pool", "usage_guide.html#resource_pool", [
        [ "Basic Usage", "usage_guide.html#rp_basic", null ],
        [ "Capacity Management", "usage_guide.html#rp_capacity", null ],
        [ "Custom Resource Factory", "usage_guide.html#rp_factory", null ],
        [ "Resource Pool Methods", "usage_guide.html#rp_methods", null ],
        [ "Scoped Resource Wrapper", "usage_guide.html#rp_scoped_resource", null ],
        [ "Multi-threaded Usage", "usage_guide.html#rp_multithreaded", null ]
      ] ],
      [ "Best Practices", "usage_guide.html#best_practices", [
        [ "Lifetime Management", "usage_guide.html#bp_lifetime", null ],
        [ "Resource Types", "usage_guide.html#bp_resource_types", null ],
        [ "Capacity Planning", "usage_guide.html#bp_capacity", null ],
        [ "Error Handling", "usage_guide.html#bp_error_handling", null ],
        [ "Monitoring", "usage_guide.html#bp_monitoring", null ]
      ] ],
      [ "Advanced Topics", "usage_guide.html#advanced", [
        [ "Custom Resource Types", "usage_guide.html#adv_custom_types", null ],
        [ "JSON Serialization", "usage_guide.html#adv_json_serialization", null ],
        [ "Resource Invalidation", "usage_guide.html#adv_resource_invalidation", null ]
      ] ],
      [ "Performance Considerations", "usage_guide.html#performance", null ]
    ] ],
    [ "Concepts", "concepts.html", "concepts" ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ],
        [ "Enumerations", "functions_enum.html", null ]
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
"_2opt_2azure-agent_2_work_220_2s_2include_2siddiqsoft_2private_2common_8hpp-example.html"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';