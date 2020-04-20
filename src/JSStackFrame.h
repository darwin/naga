#pragma once

#include "Base.h"

class CJSStackFrame {
  v8::IsolateRef m_v8_isolate;
  v8::Persistent<v8::StackFrame> m_v8_frame;

 public:
  CJSStackFrame(const v8::IsolateRef& v8_isolate, v8::Local<v8::StackFrame> v8_stack_frame);
  CJSStackFrame(const CJSStackFrame& stack_frame);

  [[nodiscard]] v8::Local<v8::StackFrame> Handle() const;

  [[nodiscard]] int GetLineNumber() const;
  [[nodiscard]] int GetColumn() const;
  [[nodiscard]] std::string GetScriptName() const;
  [[nodiscard]] std::string GetFunctionName() const;
  [[nodiscard]] bool IsEval() const;
  [[nodiscard]] bool IsConstructor() const;

  void Dump(std::ostream& os) const;
};
