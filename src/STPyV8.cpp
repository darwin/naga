#include "Base.h"
#include "Context.h"
#include "Engine.h"
#include "Exception.h"
#include "Locker.h"
#include "Wrapper.h"

BOOST_PYTHON_MODULE(_STPyV8) {
  CJavascriptException::Expose();
  CWrapper::Expose();
  CContext::Expose();
  CEngine::Expose();
  CLocker::Expose();
}

#define INIT_MODULE PyInit__STPyV8
extern "C" PyObject* INIT_MODULE();
