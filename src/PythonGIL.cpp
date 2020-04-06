#include "PythonGIL.h"

CPythonGIL::CPythonGIL() {
  m_raw_state = PyGILState_Ensure();
}

CPythonGIL::~CPythonGIL() {
  PyGILState_Release(m_raw_state);
}
