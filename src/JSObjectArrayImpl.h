#pragma once

#include "Base.h"

class CJSObjectArrayImpl {
 public:
  CJSObjectBase& m_base;

  size_t Length() const;
  py::object GetItem(py::object py_key) const;
  py::object SetItem(py::object py_key, py::object py_value) const;
  py::object DelItem(py::object py_key) const;
  bool Contains(const py::object& py_key) const;
};