#pragma once

#include "Base.h"

// JSNull maps exactly to Python's None
#define Py_JSNull Py_None

// -- pybind wrapper --------------------------------------------------------------------------------------------------

// this is similar to py::none

namespace pybind11 {

namespace detail {

inline bool PyJSNull_Check(PyObject* o) {
  return o == Py_JSNull;
}

}  // namespace detail

class js_null : public object {
 public:
  PYBIND11_OBJECT(explicit js_null, object, detail::PyJSNull_Check)
  js_null() : object(Py_JSNull, borrowed_t{}) {}
};

}  // namespace pybind11