#ifndef NAGA_JSSTACKFRAME_H_
#define NAGA_JSSTACKFRAME_H_

#include "Base.h"

class CJSStackFrame {
  v8::IsolatePtr m_v8_isolate;
  v8::Global<v8::StackFrame> m_v8_frame;

 public:
  CJSStackFrame(v8::IsolatePtr v8_isolate, v8::Local<v8::StackFrame> v8_stack_frame);
  CJSStackFrame(const CJSStackFrame& stack_frame);

  [[nodiscard]] v8::Local<v8::StackFrame> Handle() const;

  [[nodiscard]] int GetLineNumber() const;
  [[nodiscard]] int GetColumnNumber() const;
  [[nodiscard]] std::string GetScriptName() const;
  [[nodiscard]] std::string GetFunctionName() const;
  [[nodiscard]] bool IsEval() const;
  [[nodiscard]] bool IsConstructor() const;

  void Dump(std::ostream& os) const;
};

#endif