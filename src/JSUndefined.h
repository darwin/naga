#ifndef NAGA_JSUNDEFINED_H_
#define NAGA_JSUNDEFINED_H_

#include "Base.h"

// JS Undefined maps to Py_JSUndefined (our None-like object)

PyAPI_DATA(PyTypeObject) _PyJSUndefined_Type;

// don't forget to apply Py_INCREF() when returning this value!!!
PyAPI_DATA(PyObject) _Py_JSUndefinedStruct; /* Don't use this directly */
#define Py_JSUndefined (&_Py_JSUndefinedStruct)

#define Py_RETURN_JSUndefined return Py_INCREF(Py_JSUndefined), Py_JSUndefined

#endif