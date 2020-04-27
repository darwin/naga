#include "JSObject.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

CJSObject::CJSObject(v8::Local<v8::Object> v8_obj) : CJSObjectAPI(v8_obj) {
  TRACE("CJSObject::CJSObject {} v8_obj={}", THIS, v8_obj);
}

CJSObject::~CJSObject() {
  TRACE("CJSObject::~CJSObject {}", THIS);
}
