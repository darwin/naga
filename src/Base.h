#pragma once

// this header should be included on all header files
// it is also going to be precompiled in _precompile.h

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <codecvt>
#include <any>

#include <Python.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include "v8.h"
#include "libplatform/libplatform.h"

#include <gsl/gsl-lite.hpp>
#include <magic_enum.hpp>

#include "ForwardDeclarations.h"
#include "Logging.h"
#include "Features.h"
#include "Utils.h"
#include "V8Utils.h"
#include "RawUtils.h"
#include "PythonUtils.h"
#include "Printing.h"
