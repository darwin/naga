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

}  // namespace pyu