#pragma once

#include "Base.h"
#include "JSObjectBase.h"
#include "JSObjectGenericImpl.h"
#include "JSObjectFunctionImpl.h"
#include "JSObjectArrayImpl.h"
#include "JSObjectCLJSImpl.h"

// this class maintains CJSObject API exposed to Python
class CJSObjectAPI : public CJSObjectBase {
 protected:
  CJSObjectGenericImpl m_generic_impl{*this};
  CJSObjectFunctionImpl m_function_impl{*this};
  CJSObjectArrayImpl m_array_impl{*this};
  CJSObjectCLJSImpl m_cljs_impl{*this};

 public:
  explicit CJSObjectAPI(v8::Local<v8::Object> v8_obj) : CJSObjectBase(v8_obj) {}

  static py::object CreateWithArgs(const CJSObjectPtr& proto, const py::tuple& py_args, const py::dict& py_kwds);

  py::object GetAttr(const py::object& py_key) const;
  void SetAttr(const py::object& py_key, const py::object& py_obj) const;
  void DelAttr(const py::object& py_key) const;
  [[nodiscard]] py::list Dir() const;

  py::object GetItem(const py::object& py_key) const;
  py::object SetItem(const py::object& py_key, const py::object& py_value) const;
  py::object DelItem(const py::object& py_key) const;
  bool Contains(const py::object& py_key) const;

  [[nodiscard]] int Hash() const;
  [[nodiscard]] CJSObjectPtr Clone() const;

  [[nodiscard]] size_t Len() const;
  [[nodiscard]] py::object Int() const;
  [[nodiscard]] py::object Float() const;
  [[nodiscard]] py::object Bool() const;
  [[nodiscard]] py::str Str() const;
  [[nodiscard]] py::object Repr() const;

  [[nodiscard]] bool EQ(const CJSObjectPtr& other) const;
  [[nodiscard]] bool NE(const CJSObjectPtr& other) const;

  py::object Call(const py::args& py_args, const py::kwargs& py_kwargs);
  py::object Apply(const py::object& py_self, const py::list& py_args, const py::dict& py_kwds);
  py::object Invoke(const py::list& py_args, const py::dict& py_kwds);

  [[nodiscard]] std::string GetName() const;
  void SetName(const std::string& name);

  [[nodiscard]] int LineNumber() const;
  [[nodiscard]] int ColumnNumber() const;
  [[nodiscard]] int LineOffset() const;
  [[nodiscard]] int ColumnOffset() const;
  [[nodiscard]] std::string ResourceName() const;
  [[nodiscard]] std::string InferredName() const;
};