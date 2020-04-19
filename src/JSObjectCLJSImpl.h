#pragma once

#include "Base.h"

class CJSObjectCLJSImpl {
 public:
  CJSObjectBase* m_base;

  // CLJSObject
  size_t CLJSLength() const;
  py::object CLJSRepr() const;
  py::object CLJSStr() const;
  py::object CLJSGetItem(const py::object& py_key) const;
  py::object CLJSGetAttr(const py::object& py_key) const;

 private:
  py::object CLJSGetItemSlice(const py::object& py_slice) const;
  py::object CLJSGetItemIndex(const py::object& py_index) const;
  py::object CLJSGetItemString(const py::object& py_str) const;
};