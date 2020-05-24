#ifndef NAGA_JSOBJECTFUNCTIONIMPL_H_
#define NAGA_JSOBJECTFUNCTIONIMPL_H_

#include "Base.h"

py::object JSObjectFunctionCall(const JSObject& self,
                                const py::list& py_args,
                                const py::dict& py_kwargs,
                                std::optional<v8::Local<v8::Object>> opt_v8_this = std::nullopt);
py::object JSObjectFunctionApply(const JSObject& self,
                                 const py::object& py_self,
                                 const py::list& py_args,
                                 const py::dict& py_kwds);

std::string JSObjectFunctionGetName(const JSObject& self);
void JSObjectFunctionSetName(const JSObject& self, const std::string& name);

int JSObjectFunctionGetLineNumber(const JSObject& self);
int JSObjectFunctionGetColumnNumber(const JSObject& self);
int JSObjectFunctionGetLineOffset(const JSObject& self);
int JSObjectFunctionGetColumnOffset(const JSObject& self);
std::string JSObjectFunctionGetResourceName(const JSObject& self);
std::string JSObjectFunctionGetInferredName(const JSObject& self);

#endif