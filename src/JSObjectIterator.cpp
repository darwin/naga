#include "JSObjectIterator.h"
#include "JSObject.h"
#include "Logging.h"
#include "Printing.h"
#include "Wrapping.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

JSObjectIterator::JSObjectIterator(SharedJSObjectPtr shared_object_ptr) : m_shared_object_ptr(shared_object_ptr) {
  TRACE("JSObjectIterator::JSObjectIterator {} shared_object_ptr={}", THIS, shared_object_ptr);
}

JSObjectIterator::~JSObjectIterator() {
  TRACE("JSObjectIterator::~JSObjectIterator {}", THIS);
}

void JSObjectIterator::Init() {
  TRACE("JSObjectIterator::Init {}", THIS);
  assert(!Initialized());
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_this = m_shared_object_ptr->ToV8(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  // TODO: fine-tune GetPropertyNames call
  auto v8_property_names = v8_this->GetPropertyNames(v8_context).ToLocalChecked();
  m_v8_property_names.Reset(v8_isolate, v8_property_names);
  m_count = v8_property_names->Length();
  assert(Initialized());
}

SharedJSObjectIteratorPtr JSObjectIterator::Iter() {
  TRACE("JSObjectIterator::Iter {}", THIS);
  return shared_from_this();
}

py::object JSObjectIterator::Next() {
  TRACE("JSObjectIterator::Next {} m_index={}", THIS, m_index);
  if (!Initialized()) {
    Init();
  }
  if (m_index >= m_count) {
    throw py::stop_iteration();
  }

  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_property_names = m_v8_property_names.Get(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_property_name = v8_property_names->Get(v8_context, m_index).ToLocalChecked();
  auto py_property_name = wrap(v8_isolate, v8_property_name);
  m_index++;
  TRACE("=> {}", py_property_name);
  return py_property_name;
}
