#include "_precompile.h"

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

std::ostream& operator<<(std::ostream& os, const CJSObjectPtr& obj) {
  if (!obj) {
    os << "[N/A]";
  } else {
    os << *obj;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, v8::Local<v8::Value> v8_val) {
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_context = v8_isolate->GetEnteredOrMicrotaskContext();
  if (v8_context.IsEmpty()) {
    os << "[NO CONTEXT]";
  } else if (v8_val.IsEmpty()) {
    os << "[EMPTY VAL]";
  } else {
    auto v8_str = v8_val->ToDetailString(v8_context);
    if (!v8_str.IsEmpty()) {
      auto v8_utf = v8u::toUTF(v8_isolate, v8_str.ToLocalChecked());
      os << *v8_utf;
    } else {
      os << "[N/A]";
    }
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
