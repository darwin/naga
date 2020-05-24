#ifndef NAGA_WRAPPING_H_
#define NAGA_WRAPPING_H_

#include "Base.h"

// JS -> Python
py::object wrap(v8x::LockedIsolatePtr& v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this);
py::object wrap(v8x::LockedIsolatePtr& v8_isolate, v8::Local<v8::Value> v8_val);
py::object wrap(v8x::LockedIsolatePtr& v8_isolate, v8::Local<v8::Object> v8_obj);
py::object wrap(v8x::LockedIsolatePtr& v8_isolate, v8::Local<v8::Array> v8_array);
py::object wrap(v8x::LockedIsolatePtr& v8_isolate, v8::Local<v8::Primitive> v8_primitive);
py::object wrap(v8x::LockedIsolatePtr& v8_isolate, const SharedJSObjectPtr& obj);

// Python -> JS
v8::Local<v8::Value> wrap(const py::handle& py_handle);

#endif