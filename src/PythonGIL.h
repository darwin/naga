#pragma once

#include "Base.h"

class CPythonGIL {
  PyGILState_STATE m_state;

 public:
  CPythonGIL();
  ~CPythonGIL();
};
