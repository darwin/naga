#pragma once

#include "Base.h"

class CJSObjectGenericImpl {
 public:
  CJSObjectBase& m_base;

  [[nodiscard]] bool Contains(const py::object& py_key) const;
  py::object GetAttr(const py::object& py_key) const;
  void SetAttr(const py::object& py_key, const py::object& py_obj) const;
  void DelAttr(const py::object& py_key) const;

 private:
  void CheckAttr(v8::Local<v8::String> v8_name) const;
};