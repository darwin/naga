#include "PythonUtils.h"

namespace pyu {

py::gil_scoped_acquire withGIL() {
  return py::gil_scoped_acquire();
}

py::gil_scoped_release withoutGIL() {
  return py::gil_scoped_release();
}

const char* pythonTypeName(PyTypeObject* raw_type) {
  return raw_type->tp_name;
}

bool printToFileOrStdOut(const char* s, py::object py_file) {
  auto py_gil = withGIL();
  auto raw_file = py_file.is_none() ? PySys_GetObject("stdout") : py_file.ptr();
  if (!raw_file) {
    return false;
  }
  if (PyFile_WriteString(s, raw_file) == -1) {
    return false;
  }
  return true;
}

}  // namespace pyu