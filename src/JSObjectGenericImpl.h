#pragma once

#include "Base.h"

class CJSObjectGenericImpl {
 public:
  CJSObjectBase& m_base;

  [[nodiscard]] bool Contains(const py::object& py_key) const;
  py::object GetAttr(py::object py_key) const;
  void SetAttr(py::object py_key, py::object py_obj) const;
  void DelAttr(py::object py_key) const;

 private:
  void CheckAttr(v8::Local<v8::String> v8_name) const;
};