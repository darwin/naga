#include "JSEngine.h"
#include "JSScript.h"
#include "JSException.h"
#include "JSObject.h"
#include "PythonThreads.h"
#include "JSNull.h"
#include "Wrapping.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kEngineLogger), __VA_ARGS__)

CJSEngine::CJSEngine() : m_v8_isolate(v8u::getCurrentIsolate()) {
  TRACE("CEngine::CEngine");
}

CJSEngine::CJSEngine(v8::IsolateRef v8_isolate) : m_v8_isolate(std::move(v8_isolate)) {
  TRACE("CEngine::CEngine v8_isolate={}", P$(m_v8_isolate));
}

void CJSEngine::SetFlags(const std::string& flags) {
  TRACE("CEngine::SetFlags flags={}", flags);
  v8::V8::SetFlagsFromString(flags.c_str(), flags.size());
}

void CJSEngine::SetStackLimit(uintptr_t stack_limit_size) {
  TRACE("CEngine::SetStackLimit stack_limit_size={}", stack_limit_size);
  // This function uses a local stack variable to determine the isolate's
  // stack limit
  uint32_t here;
  uintptr_t stack_limit = reinterpret_cast<uintptr_t>(&here) - stack_limit_size;

  // If the size is very large and the stack is near the bottom of memory
  // then the calculation above may wrap around and give an address that is
  // above the (downwards-growing) stack. In such case we alert the user
  // that the new stack limit is not going to be set and return
  if (stack_limit > reinterpret_cast<uintptr_t>(&here)) {
    SPDLOG_ERROR("[ERROR] Attempted to set a stack limit greater than available memory");
    return;
  }

  auto v8_isolate = v8u::getCurrentIsolate();
  v8_isolate->SetStackLimit(stack_limit);
}

const char* CJSEngine::GetVersion() {
  TRACE("CEngine::GetVersion");
  auto result = v8::V8::GetVersion();
  TRACE("CEngine::GetVersion => {}", result);
  return result;
}

void CJSEngine::TerminateAllThreads() {
  TRACE("CEngine::TerminateAllThreads");
  auto v8_isolate = v8u::getCurrentIsolate();
  v8_isolate->TerminateExecution();
}

bool CJSEngine::IsDead() {
  TRACE("CEngine::IsDead");
  auto v8_isolate = v8u::getCurrentIsolate();
  auto result = v8_isolate->IsDead();
  TRACE("CEngine::IsDead => {}", result);
  return result;
}

py::object CJSEngine::ExecuteScript(v8::Local<v8::Script> v8_script) const {
  TRACE("CEngine::ExecuteScript v8_script={}", v8_script);
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  auto v8_result = withAllowedPythonThreads([&]() { return v8_script->Run(v8_context); });
  if (!v8_result.IsEmpty()) {
    return wrap(v8_isolate, v8_result.ToLocalChecked());
  }

  if (v8_try_catch.HasCaught()) {
    if (!v8_try_catch.CanContinue() && PyErr_Occurred()) {
      throw py::error_already_set();
    }
    CJSException::HandleTryCatch(m_v8_isolate, v8_try_catch);
  }
  return py::js_null();
}

CScriptPtr CJSEngine::Compile(const std::string& src, const std::string& name, int line, int col) const {
  TRACE("CEngine::Compile name={} line={} col={} src={}", name, line, col, traceText(src));
  auto v8_scope = v8u::withScope(m_v8_isolate);
  return InternalCompile(v8u::toString(src), v8u::toString(name), line, col);
}

CScriptPtr CJSEngine::CompileW(const std::wstring& src, const std::wstring& name, int line, int col) const {
  TRACE("CEngine::CompileW name={} line={} col={} src={}", P$(name), line, col, traceMore(P$(src)));
  auto v8_scope = v8u::withScope(m_v8_isolate);
  return InternalCompile(v8u::toString(src), v8u::toString(name), line, col);
}

CScriptPtr CJSEngine::InternalCompile(v8::Local<v8::String> v8_src,
                                      v8::Local<v8::Value> v8_name,
                                      int line,
                                      int col) const {
  TRACE("CEngine::InternalCompile v8_name={} line={} col={} v8_src={}", v8_name, line, col, traceText(v8_src));
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);
  auto v8_script = withAllowedPythonThreads([&]() {
    auto v8_line = v8u::toPositiveInteger(v8_isolate, line);
    auto v8_col = v8u::toPositiveInteger(v8_isolate, col);
    auto v8_script_origin = v8u::createScriptOrigin(v8_name, v8_line, v8_col);
    return v8::Script::Compile(v8_context, v8_src, &v8_script_origin);
  });

  if (v8_script.IsEmpty()) {
    CJSException::HandleTryCatch(m_v8_isolate, v8_try_catch);
  }

  return std::make_shared<CJSScript>(m_v8_isolate, *this, v8_src, v8_script.ToLocalChecked());
}

void CJSEngine::Dump(std::ostream& os) const {
  fmt::print(os, "CEngine {}", THIS);
}
