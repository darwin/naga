#include "PythonGIL.h"

CPythonGIL::CPythonGIL() {
  m_state = ::PyGILState_Ensure();
}

CPythonGIL::~CPythonGIL() {
  ::PyGILState_Release(m_state);
}
