#pragma once

#import "Base.h"

v8::Local<v8::String> ToString(const std::string &str);
v8::Local<v8::String> ToString(const std::wstring &str);
v8::Local<v8::String> ToString(py::object str);

struct CPythonGIL {
  PyGILState_STATE m_state;

  CPythonGIL();
  ~CPythonGIL();
};
