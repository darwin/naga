#ifndef NAGA_JSOBJECTGENERICIMPL_H_
#define NAGA_JSOBJECTGENERICIMPL_H_

#include "Base.h"

py::str JSObjectGenericStr(const JSObject& self);
py::str JSObjectGenericRepr(const JSObject& self);
bool JSObjectGenericContains(const JSObject& self, const py::object& py_key);
py::object JSObjectGenericGetAttr(const JSObject& self, const py::object& py_key);
void JSObjectGenericSetAttr(const JSObject& self, const py::object& py_key, const py::object& py_obj);
void JSObjectGenericDelAttr(const JSObject& self, const py::object& py_key);

#endif