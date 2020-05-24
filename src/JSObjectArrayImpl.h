#ifndef NAGA_JSOBJECTARRAYIMPL_H_
#define NAGA_JSOBJECTARRAYIMPL_H_

#include "Base.h"

py::ssize_t JSObjectArrayLength(const JSObject& self);
py::object JSObjectArrayGetItem(const JSObject& self, const py::object& py_key);
py::object JSObjectArraySetItem(const JSObject& self, const py::object& py_key, const py::object& py_value);
py::object JSObjectArrayDelItem(const JSObject& self, const py::object& py_key);
bool JSObjectArrayContains(const JSObject& self, const py::object& py_key);

#endif