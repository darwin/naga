#include "Printing.h"
#include "Context.h"
#include "Engine.h"
#include "Script.h"
#include "JSException.h"
#include "JSObject.h"
#include "JSStackTrace.h"
#include "JSStackFrame.h"

std::ostream& operator<<(std::ostream& os, const CJSStackTrace& obj) {
  obj.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSException& ex) {
  os << "JSError: " << ex.what();
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSObject& obj) {
  obj.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSObjectAPI& obj) {
  obj.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSObjectPtr& obj) {
  if (!obj) {
    os << "[N/A]";
  } else {
    os << *obj;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const CContext& obj) {
  obj.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CEngine& obj) {
  obj.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CScript& obj) {
  obj.Dump(os);
  return os;
}

std::ostream& operator<<(std::ostream& os, const CJSStackFrame& obj) {
  obj.Dump(os);
  return os;
}

namespace v8 {

std::ostream& printLocal(std::ostream& os, const Local<Value>& v) {
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

}  // namespace v8
