#pragma once

#include "Base.h"

namespace pyu {

py::gil_scoped_acquire acquireGIL();
py::gil_scoped_release releaseGIL();

}  // namespace pyu