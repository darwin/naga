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
  CJSObjectGenericImpl m_generic_impl{this};
  CJSObjectFunctionImpl m_function_impl{this};
  CJSObjectArrayImpl m_array_impl{this};
  CJSObjectCLJSImpl m_cljs_impl{this};

 public:
  explicit CJSObjectAPI(v8::Local<v8::Object> v8_obj) : CJSObjectBase(v8_obj) {}

  static void Expose(const py::module& py_module);

  // Exposed Python API
  py::object PythonGetAttr(py::object py_key) const;
  void PythonSetAttr(py::object py_key, py::object py_obj) const;
  void PythonDelAttr(py::object py_key) const;

  py::object PythonGetItem(py::object py_key) const;
  py::object PythonSetItem(py::object py_key, py::object py_value) const;
  py::object PythonDelItem(py::object py_key) const;

  [[nodiscard]] py::list PythonGetAttrList() const;
  [[nodiscard]] int PythonIdentityHash() const;
  [[nodiscard]] CJSObjectPtr PythonClone() const;

  bool PythonContains(const py::object& py_key) const;

  [[nodiscard]] size_t PythonLength() const;
  [[nodiscard]] py::object PythonInt() const;
  [[nodiscard]] py::object PythonFloat() const;
  [[nodiscard]] py::object PythonBool() const;
  [[nodiscard]] py::object PythonStr() const;
  [[nodiscard]] py::object PythonRepr() const;

  [[nodiscard]] bool PythonEquals(const CJSObjectPtr& other) const;
  [[nodiscard]] bool PythonNotEquals(const CJSObjectPtr& other) const;

  // TODO: naming
  static py::object PythonCreateWithArgs(const CJSObjectPtr& proto, const py::tuple& py_args, const py::dict& py_kwds);
  py::object PythonCallWithArgs(const py::args& py_args, const py::kwargs& py_kwargs);
  py::object PythonApply(py::object py_self, const py::list& py_args, const py::dict& py_kwds);
  py::object PythonInvoke(const py::list& py_args, const py::dict& py_kwds);

  [[nodiscard]] std::string GetName() const;
  void SetName(const std::string& name);

  [[nodiscard]] int GetLineNumber() const;
  [[nodiscard]] int GetColumnNumber() const;
  [[nodiscard]] std::string GetResourceName() const;
  [[nodiscard]] std::string GetInferredName() const;
  [[nodiscard]] int GetLineOffset() const;
  [[nodiscard]] int GetColumnOffset() const;
};