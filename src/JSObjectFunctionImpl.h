#ifndef NAGA_JSOBJECTFUNCTIONIMPL_H_
#define NAGA_JSOBJECTFUNCTIONIMPL_H_

#include "Base.h"

class JSObjectFunctionImpl {
 public:
  JSObjectBase& m_base;

  py::object Call(const py::list& py_args,
                  const py::dict& py_kwargs,
                  std::optional<v8::Local<v8::Object>> opt_v8_this = std::nullopt) const;
  py::object Apply(const py::object& py_self, const py::list& py_args, const py::dict& py_kwds) const;

  [[nodiscard]] std::string GetName() const;
  void SetName(const std::string& name) const;

  [[nodiscard]] int GetLineNumber() const;
  [[nodiscard]] int GetColumnNumber() const;
  [[nodiscard]] int GetLineOffset() const;
  [[nodiscard]] int GetColumnOffset() const;
  [[nodiscard]] std::string GetResourceName() const;
  [[nodiscard]] std::string GetInferredName() const;
};

#endif