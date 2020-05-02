#include "PythonObject.h"
#include "JSException.h"
#include "JSHospital.h"
#include "JSEternals.h"
#include "Logging.h"
#include "PythonUtils.h"
#include "PybindExtensions.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__)

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

static std::string extractExceptionMessage(py::handle py_value) {
  std::string msg;

  if (py::hasattr(py_value, "args")) {
    auto py_args = py_value.attr("args");
    if (py::isinstance<py::tuple>(py_args)) {
      auto py_args_tuple = py::cast<py::tuple>(py_args);
      auto it = py_args_tuple.begin();
      while (it != py_args_tuple.end()) {
        auto py_arg = *it;
        if (py::isinstance<py::str>(py_arg)) {
          msg += py::cast<py::str>(py_arg);
        }
        it++;
      }
    }
  } else if (py::hasattr(py_value, "message")) {
    auto py_msg = py_value.attr("message");
    if (py::isinstance<py::str>(py_msg)) {
      msg += py::cast<py::str>(py_msg);
    }
  } else if (py::isinstance<py::exact_bytes>(py_value)) {
    auto py_bytes = py::cast<py::exact_bytes>(py_value);
    msg += py_bytes;
  } else if (py::isinstance<py::exact_tuple>(py_value)) {
    auto py_tuple = py::cast<py::exact_tuple>(py_value);
    auto it = py_tuple.begin();
    while (it != py_tuple.end()) {
      auto py_item = *it;
      if (py::isinstance<py::exact_bytes>(py_item)) {
        auto py_bytes = py::cast<py::exact_bytes>(py_item);
        msg = py_bytes;
        break;
      }
      it++;
    }
  }

  return msg;
}

static auto convertExceptionToV8Error(v8::IsolatePtr v8_isolate, py::handle py_type, const std::string& msg) {
  auto raw_type = py_type.ptr();
  if (PyErr_GivenExceptionMatches(raw_type, PyExc_IndexError)) {
    return v8::Exception::RangeError(v8u::toString(v8_isolate, msg));
  } else if (PyErr_GivenExceptionMatches(raw_type, PyExc_AttributeError)) {
    return v8::Exception::ReferenceError(v8u::toString(v8_isolate, msg));
  } else if (PyErr_GivenExceptionMatches(raw_type, PyExc_SyntaxError)) {
    return v8::Exception::SyntaxError(v8u::toString(v8_isolate, msg));
  } else if (PyErr_GivenExceptionMatches(raw_type, PyExc_TypeError)) {
    return v8::Exception::TypeError(v8u::toString(v8_isolate, msg));
  } else {
    return v8::Exception::Error(v8u::toString(v8_isolate, msg));
  }
}

static void attachPythonInfoToV8Error(v8::IsolatePtr v8_isolate,
                                      v8::Local<v8::Object> v8_error_object,
                                      py::handle py_type,
                                      py::handle py_value) {
  // see general explanation in translateJavascriptException
  auto raw_type = py_type.ptr();
  auto raw_value = py_value.ptr();

  auto v8_context = v8_isolate->GetCurrentContext();

  auto v8_type_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kJSExceptionType, privateAPIForType);
  auto v8_exc_type_external = v8::External::New(v8_isolate, raw_type);
  v8_error_object->SetPrivate(v8_context, v8_type_api, v8_exc_type_external);

  auto v8_value_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kJSExceptionValue, privateAPIForValue);
  auto v8_exc_value_external = v8::External::New(v8_isolate, raw_value);
  v8_error_object->SetPrivate(v8_context, v8_value_api, v8_exc_value_external);

  // this must match Py_DECREFs below !!!
  Py_INCREF(raw_type);
  Py_INCREF(raw_value);

  hospitalizePatient(v8_error_object, [](v8::Local<v8::Object> v8_patient) {
    // note we cannot capture anything in this lambda, this would not match hospitalizePatient signature
    TRACE("doing cleanup of v8_error {}", v8_patient);
    auto v8_isolate = v8_patient->GetIsolate();
    auto v8_context = v8_isolate->GetCurrentContext();

    auto v8_type_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kJSExceptionType, privateAPIForType);
    auto v8_type_val = v8_patient->GetPrivate(v8_context, v8_type_api).ToLocalChecked();
    assert(v8_type_val->IsExternal());
    auto raw_type = static_cast<PyObject*>(v8_type_val.As<v8::External>()->Value());
    assert(raw_type);

    auto v8_value_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kJSExceptionValue, privateAPIForValue);
    auto v8_value_val = v8_patient->GetPrivate(v8_context, v8_value_api).ToLocalChecked();
    assert(v8_value_val->IsExternal());
    auto raw_value = static_cast<PyObject*>(v8_value_val.As<v8::External>()->Value());
    assert(raw_value);

    // this must match Py_INCREFs above !!!
    Py_DECREF(raw_type);
    Py_DECREF(raw_value);
  });
}

void CPythonObject::ThrowJSException(v8::IsolatePtr v8_isolate, const py::error_already_set& py_ex) {
  TRACE("CPythonObject::ThrowJSException");
  auto py_gil = pyu::withGIL();

  auto v8_scope = v8u::withScope(v8_isolate);

  py::object py_type(py_ex.type());
  py::object py_value(py_ex.value());
  py::object py_trace(py_ex.trace());

  // note that this call can create new Python objects into our py-wrappers
  // but it will make sure our ownership expectations do not change
  // (our wrappers still have to DECREF whatever is in them)
  PyErr_NormalizeException(&py_type.ptr(), &py_value.ptr(), &py_trace.ptr());

  auto msg = extractExceptionMessage(py_value);
  auto v8_error = convertExceptionToV8Error(v8_isolate, py_type, msg);
  if (v8_error->IsObject()) {
    auto v8_error_object = v8_error.As<v8::Object>();
    // see general explanation in translateJavascriptException
    attachPythonInfoToV8Error(v8_isolate, v8_error_object, py_type, py_value);
  }

  v8_isolate->ThrowException(v8_error);
}

#pragma clang diagnostic pop

v8::Local<v8::ObjectTemplate> CPythonObject::CreateJSWrapperTemplate(v8::IsolatePtr v8_isolate) {
  TRACE("CPythonObject::CreateJSWrapperTemplate");
  auto v8_wrapper_template = v8::ObjectTemplate::New(v8_isolate);
  auto v8_handler_config =
      v8::NamedPropertyHandlerConfiguration(NamedGetter, NamedSetter, NamedQuery, NamedDeleter, NamedEnumerator);

  v8_wrapper_template->SetHandler(v8_handler_config);
  v8_wrapper_template->SetIndexedPropertyHandler(IndexedGetter, IndexedSetter, IndexedQuery, IndexedDeleter,
                                                 IndexedEnumerator);
  v8_wrapper_template->SetCallAsFunctionHandler(CallWrapperAsFunction);
  return v8_wrapper_template;
}

v8::Local<v8::ObjectTemplate> CPythonObject::GetOrCreateCachedJSWrapperTemplate(v8::IsolatePtr v8_isolate) {
  TRACE("CPythonObject::GetOrCreateCachedJSWrapperTemplate");
  auto v8_scope = v8u::withEscapableScope(v8_isolate);
  auto v8_object_template =
      lookupEternal<v8::ObjectTemplate>(v8_isolate, CJSEternals::kJSWrapperTemplate, [](v8::IsolatePtr v8_isolate) {
        auto v8_wrapper_template = CreateJSWrapperTemplate(v8_isolate);
        return v8::Eternal<v8::ObjectTemplate>(v8_isolate, v8_wrapper_template);
      });
  return v8_scope.Escape(v8_object_template);
}