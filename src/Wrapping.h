#pragma once

#include "Base.h"

py::object wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this);
py::object wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val);
py::object wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Object> v8_obj);
py::object wrap(v8::IsolateRef v8_isolate, const CJSObjectPtr& obj);