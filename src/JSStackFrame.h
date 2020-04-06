#pragma once

#include "Base.h"

class CJSStackFrame {
  v8::Isolate* m_v8_isolate;
  v8::Persistent<v8::StackFrame> m_v8_frame;

 public:
  CJSStackFrame(v8::Isolate* isolate, v8::Local<v8::StackFrame> frame)
      : m_v8_isolate(isolate), m_v8_frame(isolate, frame) {}

  CJSStackFrame(const CJSStackFrame& frame) : m_v8_isolate(frame.m_v8_isolate) {
    v8::HandleScope handle_scope(m_v8_isolate);

    m_v8_frame.Reset(m_v8_isolate, frame.Handle());
  }

  [[nodiscard]] v8::Local<v8::StackFrame> Handle() const {
    return v8::Local<v8::StackFrame>::New(m_v8_isolate, m_v8_frame);
  }

  [[nodiscard]] int GetLineNumber() const {
    v8::HandleScope handle_scope(m_v8_isolate);
    return Handle()->GetLineNumber();
  }
  [[nodiscard]] int GetColumn() const {
    v8::HandleScope handle_scope(m_v8_isolate);
    return Handle()->GetColumn();
  }
  [[nodiscard]] std::string GetScriptName() const;
  [[nodiscard]] std::string GetFunctionName() const;
  [[nodiscard]] bool IsEval() const {
    v8::HandleScope handle_scope(m_v8_isolate);
    return Handle()->IsEval();
  }
  [[nodiscard]] bool IsConstructor() const {
    v8::HandleScope handle_scope(m_v8_isolate);
    return Handle()->IsConstructor();
  }

  static void Expose(const pb::module& m);
};
