#include "PythonObject.h"
#include "JSException.h"
#include "JSHospital.h"
#include "JSEternals.h"
#include "Logging.h"
#include "PythonUtils.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonObjectLogger), __VA_ARGS__)

static std::string_view getFirstLine(std::string_view sv) {
  auto pos = sv.find_first_of('\n');
  return sv.substr(0, pos);
}

static std::string_view stripTypeInfo(std::string_view sv) {
  // we expect input like "IndexError: list index out of range"
  // we strip the first part with colon and space
  // => "list index out of range"
  auto pos = sv.find_first_of(':');
  if (pos != std::string::npos) {
    if (pos + 1 < sv.size() && sv[pos + 1] == ' ') {
      return sv.substr(pos + 2);
    }
  }
  return sv;
}

static auto convertPythonExceptionToV8Error(v8::IsolatePtr v8_isolate, const py::error_already_set& py_ex) {
  // It turns out it is not that easy to extract text error message from Python's "error value" object
  // because in general it could be anything, see:
  // https://stackoverflow.com/questions/1418015/how-to-get-python-exception-text
  // https://stackoverflow.com/questions/33239308/how-to-get-exception-message-in-python-properly
  //
  // A historical note:
  //   Original STPyV8 had some hairy heuristic code here and tried to be smart about extracting
  //   text message under different circumstances (it looked for "args" and "message" attributes on the value object
  //   or tried to extract first string value if it was a tuple).
  //   There was no explanation, so I decided to delete that code.
  //
  // One can look at Python's traceback module implementation to see how complex the task is.
  // Ideally we would like to have something like `traceback.format_exception_only` here.
  // One solution could be to import traceback and let it do the job here (as suggested in[1]). But that feels
  // like a really heavy-weight approach which could be fragile (what if it has unexpected side-effects?,
  // what if user's python import environment is broken?, what if something else goes wrong?).
  //
  // Alternatively we could re-implement the main code path of format_exception_only, which boils down to this code:
  //
  //    def _format_final_exc_line(etype, value):
  //        valuestr = _some_str(value)
  //        if value is None or not valuestr:
  //            line = "%s\n" % etype
  //        else:
  //            line = "%s: %s\n" % (etype, valuestr)
  //        return line
  //
  //    def _some_str(value):
  //        try:
  //            return str(value)
  //        except:
  //            return '<unprintable %s object>' % type(value).__name__
  //
  //
  // It turns out pybind implements something very similar to the code above. See their detail::error_string()
  // implementation. This error message is available in error_already_set.what() so I decided to use it and rely
  // on their hard work to keep this in sync with future Python.
  //
  // We just need to massage their error message:
  // 1. we take only the first line of the message
  // 2. we skip type info in the message, our translation provides V8-related types
  //    so for example             "IndexError: list index out of range"
  //    is stripped to                         "list index out of range"
  //    which is later prefixed tp "RangeError: list index out of range"
  //
  // [1] https://stackoverflow.com/a/6576177/84283

  auto msg = stripTypeInfo(getFirstLine(py_ex.what()));
  auto v8_msg = v8u::toString(v8_isolate, msg);
  if (py_ex.matches(PyExc_IndexError)) {
    return v8::Exception::RangeError(v8_msg);
  } else if (py_ex.matches(PyExc_AttributeError)) {
    return v8::Exception::ReferenceError(v8_msg);
  } else if (py_ex.matches(PyExc_SyntaxError)) {
    return v8::Exception::SyntaxError(v8_msg);
  } else if (py_ex.matches(PyExc_TypeError)) {
    return v8::Exception::TypeError(v8_msg);
  } else {
    return v8::Exception::Error(v8_msg);
  }
}

static void attachPythonInfoToV8Error(v8::IsolatePtr v8_isolate,
                                      v8::Local<v8::Object> v8_error_object,
                                      py::handle py_type,
                                      py::handle py_value) {
  // see general explanation in translateJavascriptException
  auto raw_type = py_type.ptr();
  auto raw_value = py_value.ptr();

  auto v8_type_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kJSExceptionType, privateAPIForType);
  auto v8_value_api = lookupEternal<v8::Private>(v8_isolate, CJSEternals::kJSExceptionValue, privateAPIForValue);

  auto v8_exc_type_external = v8::External::New(v8_isolate, raw_type);
  auto v8_exc_value_external = v8::External::New(v8_isolate, raw_value);

  auto v8_context = v8u::getCurrentContext(v8_isolate);
  v8_error_object->SetPrivate(v8_context, v8_type_api, v8_exc_type_external);
  v8_error_object->SetPrivate(v8_context, v8_value_api, v8_exc_value_external);

  // this must match Py_DECREFs below !!!
  Py_INCREF(raw_type);
  Py_INCREF(raw_value);

  hospitalizePatient(v8_error_object, [raw_type, raw_value](v8::Local<v8::Object> v8_patient) {
    TRACE("doing cleanup of v8_error {} raw_type={} raw_value={}", v8_patient, raw_type, raw_value);
    // this must match Py_INCREFs above !!!
    Py_DECREF(raw_type);
    Py_DECREF(raw_value);
  });
}

void CPythonObject::ThrowJSException(v8::IsolatePtr v8_isolate, const py::error_already_set& py_ex) {
  TRACE("CPythonObject::ThrowJSException");
  auto py_gil = pyu::withGIL();
  auto v8_scope = v8u::withScope(v8_isolate);

  // note we don't need to call PyErr_NormalizeException
  // py::error_already_set in its constructor called py::detail::error_string() which did the normalization
  // in other words py_ex.type(), py_ex.value() and py_ex.trace() are already normalized

  auto v8_error = convertPythonExceptionToV8Error(v8_isolate, py_ex);
  if (v8_error->IsObject()) {
    auto v8_error_object = v8_error.As<v8::Object>();
    // see general explanation in translateJavascriptException
    attachPythonInfoToV8Error(v8_isolate, v8_error_object, py_ex.type(), py_ex.value());
  }

  v8_isolate->ThrowException(v8_error);
}

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