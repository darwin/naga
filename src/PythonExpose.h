#ifndef NAGA_PYTHONEXPOSE_H_
#define NAGA_PYTHONEXPOSE_H_

#include "Base.h"

void exposeJSNull(py::module py_module);
void exposeJSUndefined(py::module py_module);

void exposeAux(py::module py_module);
void exposeToolkit(py::module py_module);

void exposeJSObject(py::module py_module);
void exposeJSPlatform(py::module py_module);
void exposeJSIsolate(py::module py_module);
void exposeJSException(py::module py_module);
void exposeJSStackFrame(py::module py_module);
void exposeJSStackTrace(py::module py_module);
void exposeJSEngine(py::module py_module);
void exposeJSScript(py::module py_module);
void exposeJSContext(py::module py_module);

#endif