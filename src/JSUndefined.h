#pragma once

#include "Base.h"

// JS Undefined maps to Py_JSUndefined (our None-like object)

PyAPI_DATA(PyTypeObject) _PyJSUndefined_Type;

// don't forget to apply Py_INCREF() when returning this value!!!
PyAPI_DATA(PyObject) _Py_JSUndefinedStruct; /* Don't use this directly */
#define Py_JSUndefined (&_Py_JSUndefinedStruct)

#define Py_RETURN_JSUndefined return Py_INCREF(Py_JSUndefined), Py_JSUndefined

// -- pybind wrapper --------------------------------------------------------------------------------------------------

// this is similar to py::none

namespace pybind11 {

namespace detail {

inline bool PyJSUndefined_Check(PyObject* o) {
  return o == Py_JSUndefined;
}

}  // namespace detail

class js_undefined : public object {
 public:
  PYBIND11_OBJECT(explicit js_undefined, object, detail::PyJSUndefined_Check)
  js_undefined() : object(Py_JSUndefined, borrowed_t{}) {}
};

}  // namespace pybind11