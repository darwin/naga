#ifndef NAGA_BASE_H_
#define NAGA_BASE_H_

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

#include <magic_enum.hpp>

// our headers included here should not include Base.h back!
// this might work for you but could cause issues when compiling on other systems
// to distinguish them, we prefix them with underscore
#include "_ForwardDeclarations.h"
#include "_Features.h"
#include "_LoggingLevel.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#endif