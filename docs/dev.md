## Some notes on naming conventions

Since rewrite to pybind11 I decided to adopt more consistent naming conventions.

As you know. Naming things is hard and the situation is even more difficult with our codebase.

We have 4 conceptual zones:

1. Python land (PyObject* and friends)
2. pybind land (py::object)
3. C++ land (our stuff like CJSObject)
4. V8 land (v8::Local<v8::Object>)

A lot of our code deals with converting or wrapping objects on one side and exposing them on the other
while crossing the no-man's land in C++. So even simple local variable name "obj" would force us to name
various variants of "obj" variables differently. Rather than leaving this to programmer's creativity we
propose following scheme: 

1. variables holding Python land objects are prefixed with `raw_`, e.g. `raw_obj`
2. variables holding pybind land objects are prefixed with `py_`, e.g. `py_obj`
3. variables holding our C++ things don't have prefix, e.g. `obj`
4. variables holding V8 objects are prefixed with `v8_`, e.g. `v8_obj`

What about function/class names?

We don't have strict rules but prefixes like raw, py and v8 may appear as part of function names
where it makes sense. Some functions are placed in namespaces, for example our v8 utilities
reside in `v8u` namespace, so there is no further need to prefix them. Similar for static methods of classes.
For functions in global namespace it is preferable to have them prefixed.
