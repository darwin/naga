#ifndef NAGA_PYBINDUTILS_H_
#define NAGA_PYBINDUTILS_H_

#include "Base.h"
#include "JSNull.h"
#include "JSUndefined.h"

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

class exact_bytes : public bytes {
 public:
 PYBIND11_OBJECT(exact_bytes, bytes, PyBytes_CheckExact)
};

class exact_tuple : public tuple {
 public:
  PYBIND11_OBJECT_CVT(exact_tuple, tuple, PyTuple_CheckExact, PySequence_Tuple)
};

namespace detail {

inline bool PyJSNull_Check(PyObject* o) {
  return o == Py_JSNull;
}

inline bool PyJSUndefined_Check(PyObject* o) {
  return o == Py_JSUndefined;
}

}  // namespace detail

class js_null : public object {
 public:
  PYBIND11_OBJECT(explicit js_null, object, detail::PyJSNull_Check)
  js_null() : object(Py_JSNull, borrowed_t{}) {}
};

class js_undefined : public object {
 public:
  PYBIND11_OBJECT(explicit js_undefined, object, detail::PyJSUndefined_Check)
  js_undefined() : object(Py_JSUndefined, borrowed_t{}) {}
};

}  // namespace pybind11
#pragma clang diagnostic pop

#endif