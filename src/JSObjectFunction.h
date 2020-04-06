#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectFunction;

typedef std::shared_ptr<CJSObjectFunction> CJSObjectFunctionPtr;

class CJSObjectFunction : public CJSObject {
  // we need to have CJSObject copyable for pybind
  // credit: https://stackoverflow.com/a/22648552/84283
  v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>> m_self;

  // py::object Call(v8::Local<v8::Object> self, py::list args, py::dict kwds);
  pb::object Call2(v8::Local<v8::Object> v8_self, pb::list py_args, pb::dict py_kwargs);

 public:
  CJSObjectFunction(v8::Local<v8::Object> self, v8::Local<v8::Function> func)
      : CJSObject(func), m_self(v8::Isolate::GetCurrent(), self) {}

  ~CJSObjectFunction() override { m_self.Reset(); }

  v8::Local<v8::Object> Self() const { return v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), m_self); }

  // static py::object CallWithArgs(py::tuple args, py::dict kwds);
  static pb::object CallWithArgs2(pb::args py_args, pb::kwargs py_kwargs);
  // static py::object CreateWithArgs(CJSObjectFunctionPtr proto, py::tuple args, py::dict kwds);
  static pb::object CreateWithArgs2(CJSObjectFunctionPtr proto, pb::tuple py_args, pb::dict py_kwds);

  // py::object ApplyJavascript(CJSObjectPtr self, py::list args, py::dict kwds);
  pb::object ApplyJavascript2(CJSObjectPtr self, pb::list py_args, pb::dict py_kwds);
  // py::object ApplyPython(py::object self, py::list args, py::dict kwds);
  pb::object ApplyPython2(pb::object py_self, pb::list py_args, pb::dict py_kwds);
  // py::object Invoke(py::list args, py::dict kwds);
  pb::object Invoke2(pb::list py_args, pb::dict py_kwds);

  const std::string GetName() const;
  void SetName(const std::string& name);

  int GetLineNumber() const;
  int GetColumnNumber() const;
  const std::string GetResourceName() const;
  const std::string GetInferredName() const;
  int GetLineOffset() const;
  int GetColumnOffset() const;

  // py::object GetOwner() const;
  pb::object GetOwner2() const;
};