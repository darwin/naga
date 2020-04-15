#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectFunction : public CJSObject {
  // we need to have CJSObject copyable for pybind
  v8::Global<v8::Object> m_self;

  py::object Call(v8::Local<v8::Object> v8_self, const py::list& py_args, const py::dict& py_kwargs);

 public:
  CJSObjectFunction(v8::Local<v8::Object> self, v8::Local<v8::Function> func);
  ~CJSObjectFunction() override;

  [[nodiscard]] v8::Local<v8::Object> Self() const;

  static py::object CallWithArgs(py::args py_args, const py::kwargs& py_kwargs);
  static py::object CreateWithArgs(const CJSObjectFunctionPtr& proto,
                                   const py::tuple& py_args,
                                   const py::dict& py_kwds);

  py::object ApplyJavascript(const CJSObjectPtr& self, const py::list& py_args, const py::dict& py_kwds);
  py::object ApplyPython(py::object py_self, const py::list& py_args, const py::dict& py_kwds);
  py::object Invoke(const py::list& py_args, const py::dict& py_kwds);

  [[nodiscard]] std::string GetName() const;
  void SetName(const std::string& name);

  [[nodiscard]] int GetLineNumber() const;
  [[nodiscard]] int GetColumnNumber() const;
  [[nodiscard]] std::string GetResourceName() const;
  [[nodiscard]] std::string GetInferredName() const;
  [[nodiscard]] int GetLineOffset() const;
  [[nodiscard]] int GetColumnOffset() const;

  [[nodiscard]] py::object GetOwner() const;
};