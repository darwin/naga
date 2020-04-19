#pragma once

#include "Base.h"

class CJSObjectGenericImpl {
 public:
  CJSObjectBase* m_base;

  [[nodiscard]] bool ObjectContains(const py::object& py_key) const;
  py::object ObjectGetAttr(py::object py_key) const;
  void ObjectSetAttr(py::object py_key, py::object py_obj) const;
  void ObjectDelAttr(py::object py_key) const;

 private:
  void CheckAttr(v8::Local<v8::String> v8_name) const;
};