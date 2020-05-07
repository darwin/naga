#include "JSException.h"
#include "JSEternals.h"
#include "PythonModule.h"
#include "Logging.h"
#include "V8XUtils.h"
#include "PythonUtils.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSExceptionLogger), __VA_ARGS__)

static std::string extractExceptionMessage(v8x::LockedIsolatePtr&& v8_isolate, const v8::TryCatch& v8_try_catch) {
  TRACE("extractExceptionMessage v8_isolate={} v8_try_catch={}", P$(v8_isolate), v8_try_catch);
  assert(v8_isolate->InContext());
  auto v8_scope = v8x::withScope(v8_isolate);

  std::string prefix = v8x::toStdString(v8_isolate, v8_try_catch.Exception());
  auto v8_message = v8_try_catch.Message();
  if (v8_message.IsEmpty()) {
    return prefix;
  }

  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto resource_name = v8x::toStdString(v8_isolate, v8_message->GetScriptResourceName());
  auto line_num = v8_message->GetLineNumber(v8_context).ToChecked();
  auto start_col = v8_message->GetStartColumn();

  std::string suffix;
  auto v8_source_line = v8_message->GetSourceLine(v8_context);
  if (!v8_source_line.IsEmpty()) {
    suffix = fmt::format(" -> {}", v8x::toStdString(v8_isolate, v8_source_line.ToLocalChecked()));
  }

  return fmt::format("{} ( {} @ {} : {} ) {}", prefix, resource_name, line_num, start_col, suffix);
}

v8::Eternal<v8::Private> privateAPIForType(v8x::LockedIsolatePtr& v8_isolate) {
  return v8x::createEternalPrivateAPI(v8_isolate, "Naga#JSException##exc_type");
}

v8::Eternal<v8::Private> privateAPIForValue(v8x::LockedIsolatePtr& v8_isolate) {
  return v8x::createEternalPrivateAPI(v8_isolate, "Naga#JSException##exc_value");
}

static void translateJavascriptException(const CJSException& e) {
  TRACE("translateJavascriptException");
  auto py_gil = pyu::withGIL();

  if (e.GetType()) {
    PyErr_SetString(e.GetType(), e.what());
  } else {
    auto v8_isolate = v8x::getCurrentIsolate();
    auto v8_scope = v8x::withScope(v8_isolate);
    auto v8_context = v8x::getCurrentContext(v8_isolate);

    // This code has something to do with propagating original Python errors through JS land.
    // Imagine a Python function, calling a JS function which calls a Python function which throws:
    //
    // Python entry:
    //   pyfn1
    //    -- boundary1
    //     jsfn1
    //      -- boundary2
    //       pyfn2
    //         throw!
    //
    // 1. Python exception is thrown in pyfn2
    // 2. It will bubble up the stack and on boundary2 it must be wrapped into a JS error object and enter the JS land.
    //    This wrapping is done in CPythonObject::ThrowJSException.
    // 3. If JS code does not catch it or if it catches it and rethrows the wrapper...
    // 4. It will continue bubbling up the stack and on boundary1 it must be converted into Python error
    // 5. In this special case we don't want to do translation of JS Error object but rather we want to restore the
    //    original Python error as it was present on boundary2.
    //
    // Technically in #2 we attach Python error object(s) to JS error object.
    // We store them as v8::External into Private API fields.
    // And here in #4 we look into those private fields and extract the info (if avail).
    // Note that Python objects lifetime extensions are guaranteed because the JS error object is put to Hospital.
    //
    if (!e.Exception().IsEmpty() && e.Exception()->IsObject()) {
      auto v8_ex = e.Exception().As<v8::Object>();

      auto v8_ex_type_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kJSExceptionType, privateAPIForType);
      auto v8_ex_type_val = v8_ex->GetPrivate(v8_context, v8_ex_type_api);

      auto v8_ex_value_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kJSExceptionValue, privateAPIForValue);
      auto v8_ex_value_val = v8_ex->GetPrivate(v8_context, v8_ex_value_api);

      if (!v8_ex_type_val.IsEmpty() && !v8_ex_value_val.IsEmpty()) {
        auto v8_ex_type = v8_ex_type_val.ToLocalChecked();
        PyObject* raw_type = nullptr;
        if (v8_ex_type->IsExternal()) {
          auto type_val = v8_ex_type.As<v8::External>()->Value();
          raw_type = static_cast<PyObject*>(type_val);
        }
        auto v8_ex_value = v8_ex_value_val.ToLocalChecked();
        PyObject* raw_value = nullptr;
        if (v8_ex_value->IsExternal()) {
          auto value_val = v8_ex_value.As<v8::External>()->Value();
          raw_value = static_cast<PyObject*>(value_val);
        }

        if (raw_type && raw_value) {
          PyErr_SetObject(raw_type, raw_value);
          return;
        }
      }
    }

    // This is a workaround for the fact that we cannot inherit from pure Python classes.
    // Ideally we would like to inherit JSException from JSError and set JSException as the final error.
    // Our approach here is to instead wrap JSException in JSError (we pass it as argument when
    // creating JSError instance). JSError then forwards some functionality to the wrapped class.
    // See naga_wrapper.py.
    auto& py_module = getNagaNativeModule();
    assert((bool)py_module);
    auto py_error_class = py_module.attr("JSError");
    auto py_error_instance = py_error_class(e);
    //
    // https://docs.python.org/3.7/extending/extending.html?highlight=extending#intermezzo-errors-and-exceptions
    // Quote:
    //   The most general function is `PyErr_SetObject`, which takes two object arguments, the exception and
    //   its associated value.  You don't need to `Py_INCREF` the objects passed to any of these functions.
    //
    PyErr_SetObject(py_error_class.ptr(), py_error_instance.ptr());
  }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "performance-unnecessary-value-param"
// TODO: raise question in pybind issues
// clang-tidy suggests "const std::exception_ptr& p" signature but register_exception_translator won't accept it
void translateException(std::exception_ptr p) {
  TRACE("translateException");
  try {
    if (p) {
      std::rethrow_exception(p);
    }
  } catch (const CJSException& e) {
    translateJavascriptException(e);
  }
}
#pragma clang diagnostic pop

CJSException::CJSException(v8x::ProtectedIsolatePtr v8_protected_isolate,
                           const v8::TryCatch& v8_try_catch,
                           PyObject* raw_type)
    : std::runtime_error(extractExceptionMessage(v8_protected_isolate.lock(), v8_try_catch)),
      m_v8_isolate(v8_protected_isolate),
      m_raw_type(raw_type) {
  TRACE("CJSException::CJSException {} v8_isolate={} v8_try_catch={} raw_type={}", THIS, m_v8_isolate, v8_try_catch,
        raw_type);
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);

  m_v8_exception.Reset(v8_isolate, v8_try_catch.Exception());
  m_v8_exception.AnnotateStrongRetainer("Naga JSException.m_v8_exception");

  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto stack_trace = v8_try_catch.StackTrace(v8_context);
  if (!stack_trace.IsEmpty()) {
    m_v8_stack.Reset(v8_isolate, stack_trace.ToLocalChecked());
    m_v8_exception.AnnotateStrongRetainer("Naga JSException.m_v8_stack");
  }

  m_v8_message.Reset(v8_isolate, v8_try_catch.Message());
  m_v8_exception.AnnotateStrongRetainer("Naga JSException.m_v8_message");
}

CJSException::CJSException(v8x::ProtectedIsolatePtr v8_isolate, const std::string& msg, PyObject* raw_type) noexcept
    : std::runtime_error(msg),
      m_v8_isolate(v8_isolate),
      m_raw_type(raw_type) {
  TRACE("CJSException::CJSException {} v8_isolate={} msg='{}' raw_type={}", THIS, m_v8_isolate, msg, raw_type);
}

CJSException::CJSException(const std::string& msg, PyObject* raw_type) noexcept
    : std::runtime_error(msg),
      m_v8_isolate(v8x::getCurrentIsolate()),
      m_raw_type(raw_type) {
  TRACE("CJSException::CJSException {} msg='{}' raw_type={}", THIS, msg, raw_type);
}

CJSException::CJSException(const CJSException& ex) noexcept
    : std::runtime_error(ex.what()),
      m_v8_isolate(ex.m_v8_isolate),
      m_raw_type(ex.m_raw_type) {
  TRACE("CJSException::CJSException {} ex={}", THIS, ex);
  auto v8_isolate = m_v8_isolate.lock();
  auto v8_scope = v8x::withScope(v8_isolate);

  m_v8_exception.Reset(v8_isolate, ex.Exception());
  m_v8_stack.Reset(v8_isolate, ex.Stack());
  m_v8_message.Reset(v8_isolate, ex.Message());
}

CJSException::~CJSException() noexcept {
  TRACE("CJSException::~CJSException {}", THIS);
  if (!m_v8_exception.IsEmpty()) {
    m_v8_exception.Reset();
  }
  if (!m_v8_message.IsEmpty()) {
    m_v8_message.Reset();
  }
}

std::string CJSException::GetName() const {
  TRACE("CJSException::GetName {}", THIS);
  if (m_v8_exception.IsEmpty()) {
    return std::string();
  }

  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());

  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_exception = Exception()->ToObject(v8_context).ToLocalChecked();
  auto v8_name_key = v8x::toString(v8_isolate, "name");
  auto v8_name_val = v8_exception->Get(v8_context, v8_name_key).ToLocalChecked();
  auto v8_name_utf = v8x::toUTF(v8_isolate, v8_name_val.As<v8::String>());
  auto result = std::string(*v8_name_utf, v8_name_utf.length());
  TRACE("CJSException::GetName {} => {}", THIS, result);
  return result;
}

std::string CJSException::GetMessage() const {
  TRACE("CJSException::GetMessage {}", THIS);
  if (m_v8_exception.IsEmpty()) {
    return std::string();
  }

  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());

  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_exception = Exception()->ToObject(v8_context).ToLocalChecked();
  auto v8_name_key = v8x::toString(v8_isolate, "message");
  auto v8_name_val = v8_exception->Get(v8_context, v8_name_key).ToLocalChecked();
  auto v8_name_utf = v8x::toUTF(v8_isolate, v8_name_val.As<v8::String>());
  auto result = std::string(*v8_name_utf, v8_name_utf.length());
  TRACE("CJSException::GetMessage {} => {}", THIS, result);
  return result;
}

std::string CJSException::GetScriptName() const {
  TRACE("CJSException::GetScriptName {}", THIS);
  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());

  auto v8_scope = v8x::withScope(v8_isolate);

  if (m_v8_message.IsEmpty() || Message()->GetScriptResourceName().IsEmpty() ||
      Message()->GetScriptResourceName()->IsUndefined()) {
    return std::string();
  }

  v8::String::Utf8Value name(v8_isolate, Message()->GetScriptResourceName());

  auto result = std::string(*name, name.length());
  TRACE("CJSException::GetScriptName {} => {}", THIS, result);
  return result;
}

int CJSException::GetLineNumber() const {
  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetLineNumber(v8_context).ToChecked();
  TRACE("CJSException::GetLineNumber {} => {}", THIS, result);
  return result;
}

int CJSException::GetStartPosition() const {
  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetStartPosition();
  TRACE("CJSException::GetStartPosition {} => {}", THIS, result);
  return result;
}

int CJSException::GetEndPosition() const {
  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetEndPosition();
  TRACE("CJSException::GetEndPosition {} => {}", THIS, result);
  return result;
}

int CJSException::GetStartColumn() const {
  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetStartColumn();
  TRACE("CJSException::GetStartColumn {} => {}", THIS, result);
  return result;
}

int CJSException::GetEndColumn() const {
  TRACE("CJSException::GetEndColumn {}", THIS);
  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());
  auto v8_scope = v8x::withScope(v8_isolate);
  auto result = m_v8_message.IsEmpty() ? 1 : Message()->GetEndColumn();
  TRACE("CJSException::GetEndColumn {} => {}", THIS, result);
  return result;
}

std::string CJSException::GetSourceLine() const {
  TRACE("CJSException::GetSourceLine {}", THIS);
  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());

  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);

  if (m_v8_message.IsEmpty() || Message()->GetSourceLine(v8_context).IsEmpty()) {
    return std::string();
  }

  auto v8_line = Message()->GetSourceLine(v8_context).ToLocalChecked();
  v8::String::Utf8Value line(v8_isolate, v8_line);
  auto result = std::string(*line, line.length());
  TRACE("CJSException::GetSourceLine {} => {}", THIS, result);
  return result;
}

std::string CJSException::GetStackTrace() const {
  TRACE("CJSException::GetStackTrace {}", THIS);
  auto v8_isolate = m_v8_isolate.lock();
  assert(v8_isolate->InContext());

  auto v8_scope = v8x::withScope(v8_isolate);

  if (m_v8_stack.IsEmpty()) {
    return std::string();
  }

  auto stack = v8x::toUTF(v8_isolate, Stack());
  auto result = std::string(*stack, stack.length());
  TRACE("CJSException::GetStackTrace {} => {}", THIS, result);
  return result;
}

struct SupportedError {
  const char* m_name;
  PyObject* m_raw_type;
};

static SupportedError g_supported_errors[] = {{"RangeError", PyExc_IndexError},
                                              {"ReferenceError", PyExc_ReferenceError},
                                              {"SyntaxError", PyExc_SyntaxError},
                                              {"TypeError", PyExc_TypeError}};

void CJSException::CheckTryCatch(v8x::LockedIsolatePtr& v8_isolate, v8x::TryCatchPtr v8_try_catch) {
  // TODO: revisit this CanContinue test, it is suspicious
  TRACE("CheckTryCatch v8_isolate={} v8_try_catch={}", P$(v8_isolate), *v8_try_catch);
  if (!v8_try_catch->HasCaught() || !v8_try_catch->CanContinue()) {
    return;
  }
  Throw(v8_isolate, v8_try_catch);
}

void CJSException::Throw(v8x::LockedIsolatePtr& v8_isolate, v8x::TryCatchPtr v8_try_catch) {
  TRACE("CJSException::Throw v8_isolate={} v8_try_catch={}", P$(v8_isolate), *v8_try_catch);
  auto v8_scope = v8x::withScope(v8_isolate);
  assert(v8_try_catch->HasCaught() && v8_try_catch->CanContinue());

  PyObject* raw_type = nullptr;
  auto v8_ex = v8_try_catch->Exception();

  if (v8_ex->IsObject()) {
    auto v8_context = v8x::getCurrentContext(v8_isolate);
    auto v8_exc_obj = v8_ex->ToObject(v8_context).ToLocalChecked();
    auto v8_name = v8::String::NewFromUtf8(v8_isolate, "name").ToLocalChecked();

    if (v8_exc_obj->Has(v8_context, v8_name).ToChecked()) {
      auto v8_name_val = v8_exc_obj->Get(v8_context, v8_name).ToLocalChecked();
      auto v8_uft = v8x::toUTF(v8_isolate, v8_name_val.As<v8::String>());

      for (auto& error : g_supported_errors) {
        if (strncasecmp(error.m_name, *v8_uft, v8_uft.length()) == 0) {
          raw_type = error.m_raw_type;
        }
      }
    }
  }

  auto ex = CJSException(v8_isolate, *v8_try_catch, raw_type);
  // we have consumed passed v8_try_catch at this point
  // it is good idea to reset the source so there is no chance that someone will rethrow the exception later
  // e.g. someone could call v8x::checkTryCatch on the same v8_try_catch after this point
  v8_try_catch->Reset();
  throw ex;
}

void CJSException::PrintStackTrace(py::object py_file) const {
  TRACE("CJSException::PrintStackTrace {} py_file={}", THIS, py_file);
  auto stackTraceText = GetStackTrace();
  TRACE("printing:\n{}", traceText(stackTraceText));
  pyu::printToFileOrStdOut(stackTraceText.c_str(), py_file);
}

py::object CJSException::Str() const {
  std::stringstream ss;
  ss << *this;
  auto result = py::cast(ss.str());
  TRACE("CJSException::Str {} => {}", THIS, traceText(result));
  return result;
}

v8::Local<v8::Value> CJSException::Exception() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto result = v8::Local<v8::Value>::New(v8_isolate, m_v8_exception);
  TRACE("CJSException::Exception {} => {}", THIS, result);
  return result;
}

v8::Local<v8::Value> CJSException::Stack() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto result = v8::Local<v8::Value>::New(v8_isolate, m_v8_stack);
  TRACE("CJSException::Stack {} => {}", THIS, traceText(result));
  return result;
}

v8::Local<v8::Message> CJSException::Message() const {
  auto v8_isolate = m_v8_isolate.lock();
  auto result = v8::Local<v8::Message>::New(v8_isolate, m_v8_message);
  TRACE("CJSException::Message {} => {}", THIS, result);
  return result;
}

PyObject* CJSException::GetType() const {
  TRACE("CJSException::GetType {} => {}", THIS, m_raw_type);
  return m_raw_type;
}
