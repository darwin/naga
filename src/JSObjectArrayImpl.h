#ifndef NAGA_JSOBJECTARRAYIMPL_H_
#define NAGA_JSOBJECTARRAYIMPL_H_

#include "Base.h"

class JSObjectArrayImpl {
 public:
  JSObjectBase& m_base;

  py::ssize_t Length() const;
  py::object GetItem(const py::object& py_key) const;
  py::object SetItem(const py::object& py_key, const py::object& py_value) const;
  py::object DelItem(const py::object& py_key) const;
  bool Contains(const py::object& py_key) const;
};

#endif