#pragma once

#include "Base.h"

#undef Py_INCREF
#define Py_INCREF(op) debugPyIncRef(op)

#undef Py_DECREF
#define Py_DECREF(op) debugPyDecRef(op)

void debugPyIncRef(PyObject* op);
void debugPyDecRef(PyObject* op);
void debugPyIncRef(PyTypeObject* op);
void debugPyDecRef(PyTypeObject* op);
void debugPyIncRef(PyCFunctionObject* op);
void debugPyDecRef(PyCFunctionObject* op);
void debugPyIncRef(PyModuleDef* op);
void debugPyDecRef(PyModuleDef* op);
