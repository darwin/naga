#include "JSObjectKVIterator.h"
#include "JSObject.h"
#include "Logging.h"
#include "Printing.h"
#include "Wrapping.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

JSObjectKVIterator::JSObjectKVIterator(SharedJSObjectPtr shared_object_ptr) : m_shared_object_ptr(shared_object_ptr) {
  TRACE("JSObjectKVIterator::JSObjectKVIterator {} shared_object_ptr={}", THIS, shared_object_ptr);
}

JSObjectKVIterator::~JSObjectKVIterator() {
  TRACE("JSObjectKVIterator::~JSObjectKVIterator {}", THIS);
}

void JSObjectKVIterator::Init() {
  TRACE("JSObjectKVIterator::Init {}", THIS);
  assert(!Initialized());
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_this = m_shared_object_ptr->ToV8(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_key_collection_mode = v8::KeyCollectionMode::kIncludePrototypes;
  auto v8_property_filter = static_cast<v8::PropertyFilter>(v8::PropertyFilter::ONLY_ENUMERABLE |  //
                                                            v8::PropertyFilter::SKIP_SYMBOLS);
  auto v8_index_filter = v8::IndexFilter::kSkipIndices;
  auto v8_conversion_mode = v8::KeyConversionMode::kNoNumbers;
  auto v8_maybe_property_names = v8_this->GetPropertyNames(v8_context,              //
                                                           v8_key_collection_mode,  //
                                                           v8_property_filter,      //
                                                           v8_index_filter,         //
                                                           v8_conversion_mode);
  auto v8_property_names = v8_maybe_property_names.ToLocalChecked();
  m_v8_property_names.Reset(v8_isolate, v8_property_names);
  m_count = v8_property_names->Length();
  assert(Initialized());
}

SharedJSObjectKVIteratorPtr JSObjectKVIterator::Iter() {
  TRACE("JSObjectKVIterator::Iter {}", THIS);
  return shared_from_this();
}

py::object JSObjectKVIterator::Next() {
  TRACE("JSObjectKVIterator::Next {} m_index={}", THIS, m_index);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);

  if (!Initialized()) {
    Init();
  }

  if (m_index >= m_count) {
    throw py::stop_iteration();
  }

  auto index = m_index;
  m_index++;

  auto v8_property_names = m_v8_property_names.Get(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_property_name = v8_property_names->Get(v8_context, index).ToLocalChecked();
  auto py_property_name = wrap(v8_isolate, v8_property_name);

  auto v8_this = m_shared_object_ptr->ToV8(v8_isolate);
  auto v8_maybe_property_value = v8_this->Get(v8_context, v8_property_name);

  auto py_result = py::tuple(2);
  py_result[0] = py_property_name;
  if (!v8_maybe_property_value.IsEmpty()) {
    auto v8_property_value = v8_maybe_property_value.ToLocalChecked();
    auto py_property_value = wrap(v8_isolate, v8_property_value);
    py_result[1] = py_property_value;
  }
  TRACE("=> {}", py_result);
  return py_result.cast<py::object>();
}
