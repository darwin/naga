#ifndef NAGA_PYBINDUTILS_H_
#define NAGA_PYBINDUTILS_H_

#include "Base.h"
#include "pybind11/pytypes.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "google-explicit-constructor"

namespace pybind11 {

class exact_int : public int_ {
 public:
  PYBIND11_OBJECT_CVT(exact_int, int_, PyLong_CheckExact, PyNumber_Long)
};

class exact_float : public float_ {
 public:
  PYBIND11_OBJECT_CVT(exact_float, float_, PyFloat_CheckExact, PyNumber_Float)
};

inline bool isExactString(PyObject* o) {
  return PyUnicode_CheckExact(o) || PyBytes_CheckExact(o);
}

class exact_str : public str {
 public:
  PYBIND11_OBJECT_CVT(exact_str, str, isExactString, PyObject_Str)
};

}  // namespace pybind11
#pragma clang diagnostic pop

#endif