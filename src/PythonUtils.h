#pragma once

#include "Base.h"
#include "pybind11/pytypes.h"

namespace pyu {

py::gil_scoped_acquire withGIL();
py::gil_scoped_release withoutGIL();

}  // namespace pyu

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

}  // namespace pybind11
#pragma clang diagnostic pop