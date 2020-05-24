#include "JSObjectArrayIterator.h"
#include "JSObject.h"
#include "Logging.h"
#include "Printing.h"
#include "Wrapping.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectArrayImplLogger), __VA_ARGS__)

JSObjectArrayIterator::JSObjectArrayIterator(SharedJSObjectPtr shared_object_ptr)
    : m_shared_object_ptr(shared_object_ptr) {
  TRACE("JSObjectArrayIterator::JSObjectArrayIterator {} shared_object_ptr={}", THIS, shared_object_ptr);
}

JSObjectArrayIterator::~JSObjectArrayIterator() {
  TRACE("JSObjectArrayIterator::~JSObjectArrayIterator {}", THIS);
}

SharedJSObjectArrayIteratorPtr JSObjectArrayIterator::Iter() {
  TRACE("JSObjectArrayIterator::Iter {}", THIS);
  return shared_from_this();
}

py::object JSObjectArrayIterator::Next() {
  TRACE("JSObjectArrayIterator::Next {} m_index={}", THIS, m_index);

  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  auto v8_this = m_shared_object_ptr->ToV8(v8_isolate);
  auto v8_this_array = v8_this.As<v8::Array>();
  auto length = v8_this_array->Length();

  if (m_index >= length) {
    throw py::stop_iteration();
  }

  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto index = m_index;
  m_index++;

  if (!v8_this_array->Has(v8_context, index).ToChecked()) {
    return py::none();
  }

  auto v8_val = v8_this_array->Get(v8_context, index).ToLocalChecked();
  auto py_result = wrap(v8_isolate, v8_val, v8_this_array);
  TRACE("=> {}", py_result);
  return py_result;
}
