#include "JSContext.h"
#include "JSEngine.h"
#include "JSObject.h"
#include "JSScript.h"
#include "JSIsolate.h"
#include "JSException.h"
#include "Wrapping.h"
#include "Logging.h"
#include "Printing.h"
#include "V8Utils.h"
#include "V8ProtectedIsolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSContextLogger), __VA_ARGS__)

const int kSelfEmbedderDataIndex = 0;

static bool areIsolatesConsistent(const v8::Global<v8::Context>& v8_context, const CJSIsolatePtr& isolate) {
  auto v8_isolate = isolate->ToV8();
  auto v8_scope = v8u::withScope(v8_isolate);
  return v8_context.Get(v8_isolate)->GetIsolate() == v8_isolate;
}

CJSContextPtr CJSContext::FromV8(v8::Local<v8::Context> v8_context) {
  // FromV8 may be called only on contexts created by our constructor
  if (v8_context->GetNumberOfEmbedderDataFields() <= kSelfEmbedderDataIndex) {
    throw CJSException(v8::ProtectedIsolatePtr(v8_context->GetIsolate()),
                       fmt::format("Cannot work with foreign V8 context {}", v8_context));
  }
  auto v8_data = v8_context->GetEmbedderData(kSelfEmbedderDataIndex);
  assert(v8_data->IsExternal());
  auto v8_external = v8_data.As<v8::External>();
  auto context_ptr = static_cast<CJSContext*>(v8_external->Value());
  assert(context_ptr);
  TRACE("CJSContext::FromV8 v8_context={} => {}", v8_context, (void*)context_ptr);
  return context_ptr->shared_from_this();
}

v8::Local<v8::Context> CJSContext::ToV8() const {
  assert(areIsolatesConsistent(m_v8_context, m_isolate));
  auto v8_isolate = m_isolate->ToV8();
  auto v8_result = m_v8_context.Get(v8_isolate);
  TRACE("CJSContext::ToV8 {} => {}", THIS, v8_result);
  return v8_result;
}

CJSContext::CJSContext(const py::object& py_global) : m_entered_level(0) {
  TRACE("CJSContext::CJSContext {} py_global={}", THIS, py_global);
  auto v8_isolate = v8u::getCurrentIsolate();
  m_isolate = CJSIsolate::FromV8(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8::Context::New(v8_isolate);
  auto v8_this = v8::External::New(v8_isolate, this);
  v8_context->SetEmbedderData(kSelfEmbedderDataIndex, v8_this);
  m_v8_context.Reset(v8_isolate, v8_context);
  m_v8_context.AnnotateStrongRetainer("Naga JSContext");

  if (!py_global.is_none()) {
    auto v8_context_scope = v8u::withContext(v8_context);
    auto v8_proto_key = v8::String::NewFromUtf8(v8_isolate, "__proto__").ToLocalChecked();
    auto v8_global = wrap(py_global);
    v8_context->Global()->Set(v8_context, v8_proto_key, v8_global).Check();
  }
}

CJSContext::~CJSContext() {
  TRACE("CJSContext::~CJSContext {}", THIS);
  m_v8_context.Reset();
}

py::object CJSContext::GetGlobal() const {
  assert(areIsolatesConsistent(m_v8_context, m_isolate));
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto py_result = wrap(v8_isolate, ToV8()->Global());
  TRACE("CJSContext::GetGlobal {} => {}", THIS, py_result);
  return py_result;
}

py::str CJSContext::GetSecurityToken() const {
  TRACE("CJSContext::GetSecurityToken {}", THIS);
  assert(areIsolatesConsistent(m_v8_context, m_isolate));
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_token = ToV8()->GetSecurityToken();

  if (v8_token.IsEmpty()) {
    return py::str();
  }

  auto v8_token_str = v8_token->ToString(m_v8_context.Get(v8_isolate)).ToLocalChecked();
  auto v8_token_utf = v8u::toUTF(v8_isolate, v8_token_str);
  auto py_result = py::str(*v8_token_utf, v8_token_utf.length());
  TRACE("CJSContext::GetSecurityToken {} => {}", THIS, py_result);
  return py_result;
}

void CJSContext::SetSecurityToken(const py::str& py_token) const {
  TRACE("CJSContext::SetSecurityToken {} py_token={}", THIS, py_token);
  assert(areIsolatesConsistent(m_v8_context, m_isolate));
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::withScope(v8_isolate);

  if (py_token.is_none()) {
    ToV8()->UseDefaultSecurityToken();
  } else {
    std::string str = py::cast<py::str>(py_token);
    auto v8_token_str = v8u::toString(v8_isolate, str);
    ToV8()->SetSecurityToken(v8_token_str);
  }
}

py::object CJSContext::GetCurrent() {
  TRACE("CJSContext::GetCurrent");
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8u::getCurrentContext(v8_isolate);

  if (v8_context.IsEmpty()) {
    return py::none();
  }
  auto py_result = py::cast(FromV8(v8_context));
  TRACE("CJSContext::GetCurrent => {}", py_result);
  return py_result;
}

py::object CJSContext::Evaluate(const std::string& src, const std::string& name, int line, int col) {
  TRACE("CJSContext::Evaluate name={} line={} col={} src={}", name, line, col, traceText(src));
  auto v8_isolate = v8u::getCurrentIsolate();
  CJSEngine engine(v8_isolate);
  CJSScriptPtr script = engine.Compile(src, name, line, col);
  auto py_result = script->Run();
  TRACE("CJSContext::Evaluate => {}", py_result);
  return py_result;
}

py::object CJSContext::EvaluateW(const std::wstring& src, const std::wstring& name, int line, int col) {
  TRACE("CJSContext::EvaluateW name={} line={} col={} src={}", P$(name), line, col, traceText(P$(src)));
  auto v8_isolate = v8u::getCurrentIsolate();
  CJSEngine engine(v8_isolate);
  CJSScriptPtr script = engine.CompileW(src, name, line, col);
  return script->Run();
}

void CJSContext::Enter() {
  TRACE("CJSContext::Enter {}", THIS);
  assert(areIsolatesConsistent(m_v8_context, m_isolate));
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::withScope(v8_isolate);

  if (m_entered_level == 0) {
    m_v8_shared_isolate_locker = v8_isolate.shareLocker();
  }
  m_entered_level++;

  ToV8()->Enter();
}

void CJSContext::Leave() {
  TRACE("CJSContext::Leave {}", THIS);
  assert(areIsolatesConsistent(m_v8_context, m_isolate));
  auto v8_isolate = m_isolate->ToV8();
  auto v8_scope = v8u::withScope(v8_isolate);

  ToV8()->Exit();

  assert(m_entered_level > 0);
  m_entered_level--;
  if (m_entered_level == 0) {
    m_v8_shared_isolate_locker.reset();
  }
}

void CJSContext::Dump(std::ostream& os) const {
  fmt::print(os, "CContext {} m_v8_context={}", THIS, ToV8());
}