#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectFunction : public CJSObject {
  // we need to have CJSObject copyable for pybind
  // credit: https://stackoverflow.com/a/22648552/84283
  v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>> m_self;

  pb::object Call(v8::Local<v8::Object> v8_self, const pb::list& py_args, const pb::dict& py_kwargs);

 public:
  CJSObjectFunction(v8::Local<v8::Object> self, v8::Local<v8::Function> func);
  ~CJSObjectFunction() override { m_self.Reset(); }

  [[nodiscard]] v8::Local<v8::Object> Self() const;

  static pb::object CallWithArgs(pb::args py_args, const pb::kwargs& py_kwargs);
  static pb::object CreateWithArgs(const CJSObjectFunctionPtr& proto,
                                   const pb::tuple& py_args,
                                   const pb::dict& py_kwds);

  pb::object ApplyJavascript(const CJSObjectPtr& self, const pb::list& py_args, const pb::dict& py_kwds);
  pb::object ApplyPython(pb::object py_self, const pb::list& py_args, const pb::dict& py_kwds);
  pb::object Invoke(const pb::list& py_args, const pb::dict& py_kwds);

  [[nodiscard]] std::string GetName() const;
  void SetName(const std::string& name);

  [[nodiscard]] int GetLineNumber() const;
  [[nodiscard]] int GetColumnNumber() const;
  [[nodiscard]] std::string GetResourceName() const;
  [[nodiscard]] std::string GetInferredName() const;
  [[nodiscard]] int GetLineOffset() const;
  [[nodiscard]] int GetColumnOffset() const;

  [[nodiscard]] pb::object GetOwner2() const;
};