#pragma once

#include "Base.h"

class CJSObjectFunctionImpl {
 public:
  CJSObjectBase* m_base;

  py::object Call(const py::list& py_args,
                  const py::dict& py_kwargs,
                  std::optional<v8::Local<v8::Object>> opt_v8_this = std::nullopt) const;
  py::object Apply(py::object py_self, const py::list& py_args, const py::dict& py_kwds);

  [[nodiscard]] std::string GetName() const;
  void SetName(const std::string& name);

  [[nodiscard]] int GetLineNumber() const;
  [[nodiscard]] int GetColumnNumber() const;
  [[nodiscard]] std::string GetResourceName() const;
  [[nodiscard]] std::string GetInferredName() const;
  [[nodiscard]] int GetLineOffset() const;
  [[nodiscard]] int GetColumnOffset() const;
};