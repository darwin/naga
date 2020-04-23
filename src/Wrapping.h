#pragma once

#include "Base.h"

// JS -> Python
py::object wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this);
py::object wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val);
py::object wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Object> v8_obj);
py::object wrap(v8::IsolateRef v8_isolate, const CJSObjectPtr& obj);

// Python -> JS
v8::Local<v8::Value> wrap(const py::handle& py_handle);