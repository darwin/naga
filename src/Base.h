#pragma once

// this header should be included on all header files
// it is also going to be precompiled in _precompile.h

#include <string>
#include <map>
#include <vector>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <fstream>
#include <sstream>
#include <iostream>
#include <utility>
#include <iterator>
#include <optional>

#include "Python.h"
#include <boost/python.hpp>
#include <boost/iterator/iterator_facade.hpp>
namespace py = boost::python;

#include <boost/iterator/iterator_facade.hpp>

#include "v8.h"

#include "Config.h"
#include "Utils.h"