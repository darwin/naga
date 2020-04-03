#pragma once

#include "Base.h"
#include "JSObject.h"

class CJavascriptFunction;

typedef std::shared_ptr<CJavascriptFunction> CJavascriptFunctionPtr;

class CJavascriptFunction : public CJSObject {
  v8::Persistent<v8::Object> m_self;

  py::object Call(v8::Local<v8::Object> self, py::list args, py::dict kwds);

 public:
  CJavascriptFunction(v8::Local<v8::Object> self, v8::Local<v8::Function> func)
      : CJSObject(func), m_self(v8::Isolate::GetCurrent(), self) {}

  ~CJavascriptFunction() override { m_self.Reset(); }

  v8::Local<v8::Object> Self() const { return v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), m_self); }

  static py::object CallWithArgs(py::tuple args, py::dict kwds);
  static py::object CreateWithArgs(CJavascriptFunctionPtr proto, py::tuple args, py::dict kwds);

  py::object ApplyJavascript(CJSObjectPtr self, py::list args, py::dict kwds);
  py::object ApplyPython(py::object self, py::list args, py::dict kwds);
  py::object Invoke(py::list args, py::dict kwds);

  const std::string GetName() const;
  void SetName(const std::string& name);

  int GetLineNumber() const;
  int GetColumnNumber() const;
  const std::string GetResourceName() const;
  const std::string GetInferredName() const;
  int GetLineOffset() const;
  int GetColumnOffset() const;

  py::object GetOwner() const;
};