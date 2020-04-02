#pragma once

#include <string>
#include <boost/python.hpp>
namespace py = boost::python;

#include "v8.h"

#define PyInt_Check PyLong_Check
#define PyInt_AsUnsignedLongMask PyLong_AsUnsignedLong
#define PySlice_Cast(obj) obj

v8::Local<v8::String> ToString(const std::string &str);
v8::Local<v8::String> ToString(const std::wstring &str);
v8::Local<v8::String> ToString(py::object str);

struct CPythonGIL {
  PyGILState_STATE m_state;

  CPythonGIL();
  ~CPythonGIL();
};
