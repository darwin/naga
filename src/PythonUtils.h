#pragma once

#include "Python.h"
#include "v8.h"

// these utils work with raw python objects

v8::Local<v8::String> pythonBytesObjectToString(PyObject *py_obj);