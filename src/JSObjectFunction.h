#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectFunction : public CJSObject {
  py::object Call(const py::list& py_args,
                  const py::dict& py_kwargs,
                  std::optional<v8::Local<v8::Object>> opt_v8_this = std::nullopt);

 public:
  CJSObjectFunction(v8::Local<v8::Function> v8_fn);
  ~CJSObjectFunction() override;

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