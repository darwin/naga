#pragma once

#include "Base.h"

class CJSObjectArrayImpl {
 public:
  CJSObjectBase& m_base;

  size_t ArrayLength() const;
  py::object ArrayGetItem(py::object py_key) const;
  py::object ArraySetItem(py::object py_key, py::object py_value) const;
  py::object ArrayDelItem(py::object py_key) const;
  bool ArrayContains(const py::object& py_key) const;
};