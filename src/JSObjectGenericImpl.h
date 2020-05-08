#ifndef NAGA_JSOBJECTGENERICIMPL_H_
#define NAGA_JSOBJECTGENERICIMPL_H_

#include "Base.h"

class JSObjectGenericImpl {
 public:
  JSObjectBase& m_base;

  py::str Str() const;
  py::str Repr() const;

  [[nodiscard]] bool Contains(const py::object& py_key) const;
  py::object GetAttr(const py::object& py_key) const;
  void SetAttr(const py::object& py_key, const py::object& py_obj) const;
  void DelAttr(const py::object& py_key) const;
};

#endif