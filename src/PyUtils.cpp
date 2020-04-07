#include "PyUtils.h"

namespace pyu {

py::gil_scoped_acquire acquireGIL() {
  return py::gil_scoped_acquire();
}

py::gil_scoped_release releaseGIL() {
  return py::gil_scoped_release();
}

}  // namespace pyu