#pragma once

#include "Base.h"

// TODO: replace this with pybind GIL

class CPythonGIL {
  PyGILState_STATE m_raw_state;

 public:
  CPythonGIL();
  ~CPythonGIL();
};
