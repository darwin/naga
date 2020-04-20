#pragma once

#include "Base.h"

class CJSObjectCLJSImpl {
 public:
  CJSObjectBase& m_base;

  size_t Length() const;
  py::object Repr() const;
  py::object Str() const;
  py::object GetItem(const py::object& py_key) const;
  py::object GetAttr(const py::object& py_key) const;

 private:
  py::object GetItemSlice(const py::object& py_slice) const;
  py::object GetItemIndex(const py::object& py_index) const;
  py::object GetItemString(const py::object& py_str) const;
};