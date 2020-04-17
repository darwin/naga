#include "_precompile.h"

#include "Module.h"
#include "Platform.h"
#include "Isolate.h"
#include "Context.h"
#include "Engine.h"
#include "Script.h"
#include "JSStackFrame.h"
#include "JSStackTrace.h"
#include "JSException.h"
#include "JSObject.h"
#include "JSObjectCLJS.h"
#include "Locker.h"
#include "Unlocker.h"
#include "Aux.h"

PYBIND11_MODULE(_STPyV8, py_module) {
  useLogging();

  SPDLOG_INFO("");
  SPDLOG_INFO("=====================================================================================================");
  SPDLOG_INFO("Initializing _STPyV8 module...");

  exposeAux(&py_module);

  CPlatform::Expose(py_module);
  CIsolate::Expose(py_module);
  CJSStackFrame::Expose(py_module);
  CJSStackTrace::Expose(py_module);
  CJSException::Expose(py_module);
  CJSObject::Expose(py_module);
  CContext::Expose(py_module);
  CScript::Expose(py_module);
  CEngine::Expose(py_module);
  CLocker::Expose(py_module);
  CUnlocker::Expose(py_module);
#ifdef STPYV8_FEATURE_CLJS
  CJSObjectCLJS::Expose(py_module);
#endif
}
