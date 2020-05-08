#include "JSObject.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

JSObject::JSObject(v8::Local<v8::Object> v8_obj) : JSObjectAPI(v8_obj) {
  TRACE("JSObject::JSObject {} v8_obj={}", THIS, v8_obj);
}

JSObject::~JSObject() {
  TRACE("JSObject::~JSObject {}", THIS);
}
