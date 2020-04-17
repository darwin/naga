#include "_precompile.h"

#include "PyUtils.h"

namespace pyu {

py::gil_scoped_acquire withGIL() {
  return py::gil_scoped_acquire();
}

py::gil_scoped_release withoutGIL() {
  return py::gil_scoped_release();
}

}  // namespace pyu