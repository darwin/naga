#pragma once

#include "Base.h"

void exposeJSToolkit(py::module py_module);
void exposeJSNull(py::module py_module);
void exposeJSUndefined(py::module py_module);
void exposeJSObject(py::module py_module);
void exposeJSPlatform(py::module py_module);
void exposeJSIsolate(py::module py_module);
void exposeJSError(py::module py_module);
void exposeJSStackFrame(py::module py_module);
void exposeJSStackTrace(py::module py_module);
void exposeJSEngine(py::module py_module);
void exposeJSScript(py::module py_module);
void exposeJSLocker(py::module py_module);
void exposeJSUnlocker(py::module py_module);
void exposeJSContext(py::module py_module);
void exposeAux(py::module py_module);