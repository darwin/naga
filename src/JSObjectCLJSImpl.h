#ifndef NAGA_JSOBJECTCLJSIMPL_H_
#define NAGA_JSOBJECTCLJSIMPL_H_

#include "Base.h"

py::ssize_t JSObjectCLJSLength(const JSObject& self);
py::str JSObjectCLJSRepr(const JSObject& self);
py::str JSObjectCLJSStr(const JSObject& self);
py::object JSObjectCLJSGetItem(const JSObject& self, const py::object& py_key);
py::object JSObjectCLJSGetAttr(const JSObject& self, const py::object& py_key);
py::object JSObjectCLJSGetItemSlice(const JSObject& self, const py::object& py_slice);
py::object JSObjectCLJSGetItemIndex(const JSObject& self, const py::object& py_index);
py::object JSObjectCLJSGetItemString(const JSObject& self, const py::object& py_str);

#endif