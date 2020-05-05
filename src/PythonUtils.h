#ifndef NAGA_PYTHONUTILS_H_
#define NAGA_PYTHONUTILS_H_

#include "Base.h"

namespace pyu {

py::gil_scoped_acquire withGIL();
py::gil_scoped_release withoutGIL();

inline py::object getStdOut() {
  return py::reinterpret_borrow<py::object>(PySys_GetObject("stdout"));
}

const char* pythonTypeName(PyTypeObject* raw_type);
bool printToFileOrStdOut(const char* s, py::object py_file = getStdOut());

}  // namespace pyu

#endif