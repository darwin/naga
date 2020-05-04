#include "PythonObject.h"
#include "JSEternals.h"
#include "Logging.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__)

v8::Local<v8::ObjectTemplate> CPythonObject::CreateJSWrapperTemplate(v8::IsolatePtr v8_isolate) {
  TRACE("CPythonObject::CreateJSWrapperTemplate");
  auto v8_template = v8::ObjectTemplate::New(v8_isolate);
  auto v8_handler_config =
      v8::NamedPropertyHandlerConfiguration(NamedGetter, NamedSetter, NamedQuery, NamedDeleter, NamedEnumerator);

  v8_template->SetHandler(v8_handler_config);
  v8_template->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter, IndexedEnumerator);
  v8_template->SetCallAsFunctionHandler(CallWrapperAsFunction);
  return v8_template;
}

v8::Local<v8::ObjectTemplate> CPythonObject::GetOrCreateCachedJSWrapperTemplate(v8::IsolatePtr v8_isolate) {
  TRACE("CPythonObject::GetOrCreateCachedJSWrapperTemplate");
  assert(v8u::hasScope(v8_isolate));
  auto v8_object_template =
      lookupEternal<v8::ObjectTemplate>(v8_isolate, CJSEternals::kJSWrapperTemplate, [](v8::IsolatePtr v8_isolate) {
        auto v8_wrapper_template = CreateJSWrapperTemplate(v8_isolate);
        return v8::Eternal<v8::ObjectTemplate>(v8_isolate, v8_wrapper_template);
      });
  return v8_object_template;
}