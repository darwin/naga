#ifndef NAGA_RAWUTILS_H_
#define NAGA_RAWUTILS_H_

#include "Base.h"

// these utils work with raw python objects

v8::Local<v8::String> pythonBytesObjectToString(PyObject* raw_obj);
const char* pythonTypeName(PyTypeObject* raw_type);

#endif