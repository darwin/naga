#include "Printing.h"
#include "Context.h"
#include "Engine.h"
#include "Script.h"
#include "JSException.h"
#include "JSObject.h"
#include "JSStackTrace.h"
#include "JSStackFrame.h"

std::string printCoerced(const std::wstring& v) {
  try {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(v);
  } catch (...) {
    return "{std::wstring conversion error}";
  }
}

std::string printCoerced(const v8::IsolateRef& v) {
  return fmt::format("v8::IsolateRef {}", static_cast<void*>(v.get()));
}

std::ostream& operator<<(std::ostream& os, const CJSStackTrace& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSException& v) {
  os << "JSError: " << v.what();
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSObject& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSObjectAPI& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CContext& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CEngine& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CScript& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSStackFrame& v) {
  v.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSObject::Roles& v) {
  std::vector<const char*> flags;
  flags.reserve(8);
  if ((v & CJSObject::Roles::Function) == CJSObject::Roles::Function) {
    flags.push_back("Function");
  }
  if ((v & CJSObject::Roles::Array) == CJSObject::Roles::Array) {
    flags.push_back("Array");
  }
  return os << fmt::format("{}", fmt::join(flags, ","));
}

std::ostream& operator<<(std::ostream& os, const PyObject* v) {
  if (!v) {
    return os << "PyObject* 0x0";
  }
  return os << fmt::format("PyObject* {} [#{}]", static_cast<const void*>(v), Py_REFCNT(v));
}

namespace v8 {

std::ostream& printLocalValue(std::ostream& os, const Local<Value>& v) {
  if (v.IsEmpty()) {
    return os << "{EMPTY}";
  }

  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_context = v8_isolate->GetEnteredOrMicrotaskContext();
  if (v8_context.IsEmpty()) {
    return os << "{NO CONTEXT}";
  }

  auto v8_str = v->ToDetailString(v8_context);
  if (v8_str.IsEmpty()) {
    return os << "{N/A}";
  } else {
    return os << *v8u::toUTF(v8_isolate, v8_str.ToLocalChecked());
  }
}

std::ostream& operator<<(std::ostream& os, const TryCatch& v) {
  return os << fmt::format("v8::TryCatch Message='{}'", v.Message());
}

std::ostream& operator<<(std::ostream& os, const Local<Private>& v) {
  return os << fmt::format("v8::Private {}", static_cast<void*>(*v));
}

std::ostream& operator<<(std::ostream& os, const Local<Context>& v) {
  if (v.IsEmpty()) {
    return os << fmt::format("v8::Context {} [EMPTY]", static_cast<void*>(*v));
  } else {
    return os << fmt::format("v8::Context {} global={}", static_cast<void*>(*v), v->Global());
  }
}

std::ostream& operator<<(std::ostream& os, const Local<Script>& v) {
  return os << fmt::format("v8::Script {}", static_cast<void*>(*v));
}

std::ostream& operator<<(std::ostream& os, const Local<ObjectTemplate>& v) {
  return os << fmt::format("v8::ObjectTemplate {}", static_cast<void*>(*v));
}

std::ostream& operator<<(std::ostream& os, const Local<Message>& v) {
  return os << fmt::format("v8::Message {} '{}'", static_cast<void*>(*v), v->Get());
}

std::ostream& operator<<(std::ostream& os, const Local<StackFrame>& v) {
  return os << fmt::format("v8::StackFrame {} ScriptId={} Script={}", static_cast<void*>(*v), v->GetScriptId(),
                           v->GetScriptNameOrSourceURL());
}

std::ostream& operator<<(std::ostream& os, const Local<StackTrace>& v) {
  return os << fmt::format("v8::StackTrace {} FrameCount={}", static_cast<void*>(*v), v->GetFrameCount());
}

std::ostream& operator<<(std::ostream& os, const IsolateRef& v) {
  return os << fmt::format("v8::IsolateRef {}", static_cast<void*>(v.get()));
}

}  // namespace v8

namespace pybind11 {

std::ostream& operator<<(std::ostream& os, const error_already_set& v) {
  return os << fmt::format("py::error_already_set[type={} what={}]", v.type(), v.what());
}

}  // namespace pybind11
