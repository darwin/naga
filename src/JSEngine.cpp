#include "JSEngine.h"
#include "JSScript.h"
#include "PythonUtils.h"
#include "Wrapping.h"
#include "Logging.h"
#include "V8XUtils.h"
#include "Printing.h"
#include "PybindExtensions.h"
#include "V8XProtectedIsolate.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSEngineLogger), __VA_ARGS__)

JSEngine::JSEngine() : m_v8_isolate(v8x::getCurrentIsolate()) {
  TRACE("JSEngine::JSEngine");
}

JSEngine::JSEngine(v8x::ProtectedIsolatePtr v8_isolate) : m_v8_isolate(std::move(v8_isolate)) {
  TRACE("JSEngine::JSEngine v8_isolate={}", m_v8_isolate);
}

void JSEngine::SetFlags(const std::string& flags) {
  TRACE("JSEngine::SetFlags flags={}", flags);
  v8::V8::SetFlagsFromString(flags.c_str(), flags.size());
}

void JSEngine::SetStackLimit(uintptr_t stack_limit_size) {
  TRACE("JSEngine::SetStackLimit stack_limit_size={}", stack_limit_size);
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

  auto v8_isolate = v8x::getCurrentIsolate();
  v8_isolate->SetStackLimit(stack_limit);
}

const char* JSEngine::GetVersion() {
  TRACE("JSEngine::GetVersion");
  auto result = v8::V8::GetVersion();
  TRACE("JSEngine::GetVersion => {}", result);
  return result;
}

void JSEngine::TerminateAllThreads() {
  TRACE("JSEngine::TerminateAllThreads");
  auto v8_isolate = v8x::getCurrentIsolate();
  v8_isolate->TerminateExecution();
}

bool JSEngine::IsDead() {
  TRACE("JSEngine::IsDead");
  auto v8_isolate = v8x::getCurrentIsolate();
  auto result = v8_isolate->IsDead();
  TRACE("JSEngine::IsDead => {}", result);
  return result;
}

py::object JSEngine::ExecuteScript(v8::Local<v8::Script> v8_script) const {
  TRACE("JSEngine::ExecuteScript v8_script={}", v8_script);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);

  v8::MaybeLocal<v8::Value> v8_maybe_result;
  {
    auto _ = pyu::withoutGIL();
    v8_maybe_result = v8_script->Run(v8_context);
  }

  if (v8_maybe_result.IsEmpty()) {
    return py::js_null();
  }
  return wrap(v8_isolate, v8_maybe_result.ToLocalChecked());
}

SharedJSScriptPtr JSEngine::Compile(const std::string& src, const std::string& name, int line, int col) const {
  TRACE("JSEngine::Compile name={} line={} col={} src={}", name, line, col, traceText(src));
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  return InternalCompile(v8_isolate, v8x::toString(v8_isolate, src), v8x::toString(v8_isolate, name), line, col);
}

SharedJSScriptPtr JSEngine::CompileW(const std::wstring& src, const std::wstring& name, int line, int col) const {
  TRACE("JSEngine::CompileW name={} line={} col={} src={}", P$(name), line, col, traceMore(P$(src)));
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);
  return InternalCompile(v8_isolate, v8x::toString(v8_isolate, src), v8x::toString(v8_isolate, name), line, col);
}

SharedJSScriptPtr JSEngine::InternalCompile(v8x::LockedIsolatePtr& v8_isolate,
                                            v8::Local<v8::String> v8_src,
                                            v8::Local<v8::Value> v8_name,
                                            int line,
                                            int col) const {
  TRACE("JSEngine::InternalCompile v8_name={} line={} col={} v8_src={}", v8_name, line, col, traceText(v8_src));
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  v8::MaybeLocal<v8::Script> v8_maybe_script;
  {
    auto _ = pyu::withoutGIL();
    auto v8_line = v8x::toPositiveInteger(v8_isolate, line);
    auto v8_col = v8x::toPositiveInteger(v8_isolate, col);
    auto v8_script_origin = v8x::createScriptOrigin(v8_name, v8_line, v8_col);
    v8_maybe_script = v8::Script::Compile(v8_context, v8_src, &v8_script_origin);
  }

  v8x::checkTryCatch(v8_isolate, v8_try_catch);
  return std::make_shared<JSScript>(v8_isolate, *this, v8_src, v8_maybe_script.ToLocalChecked());
}

void JSEngine::Dump(std::ostream& os) const {
  fmt::print(os, "CEngine {}", THIS);
}
