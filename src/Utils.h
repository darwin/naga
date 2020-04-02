#pragma once

#include <string>
#include <boost/python.hpp>
namespace py = boost::python;

#include "v8.h"

#include <strings.h>
#define strnicmp strncasecmp

#if defined(__GNUC__)
#define UNUSED_VAR(x) x __attribute__((unused))
#elif defined(__APPLE__)
#define UNUSED_VAR(x) x __unused
#else
#define UNUSED_VAR(x) x
#endif

#define PyInt_Check PyLong_Check
#define PyInt_AsUnsignedLongMask PyLong_AsUnsignedLong
#define PySlice_Cast(obj) obj
#ifdef Py_DEBUG
#define PyErr_OCCURRED() PyErr_Occurred()
#else
#define PyErr_OCCURRED() (PyThreadState_GET()->curexc_type)
#endif


v8::Local<v8::String> ToString(const std::string &str);
v8::Local<v8::String> ToString(const std::wstring &str);
v8::Local<v8::String> ToString(py::object str);

struct CPythonGIL {
  PyGILState_STATE m_state;

  CPythonGIL();
  ~CPythonGIL();
};
