#ifndef NAGA_PYTHONUTILS_H_
#define NAGA_PYTHONUTILS_H_

#include "Base.h"

namespace pyu {

py::gil_scoped_acquire withGIL();
py::gil_scoped_release withoutGIL();

}  // namespace pyu

#endif