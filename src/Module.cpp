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

PYBIND11_MODULE(_STPyV8, m) {
  useLogging();

  SPDLOG_INFO("");
  SPDLOG_INFO("=====================================================================================================");
  SPDLOG_INFO("Initializing _STPyV8 module...");

  CPlatform::Expose(m);
  CIsolate::Expose(m);
  CJSStackFrame::Expose(m);
  CJSStackTrace::Expose(m);
  CJSException::Expose(m);
  CJSObject::Expose(m);
  CContext::Expose(m);
  CScript::Expose(m);
  CEngine::Expose(m);
  CLocker::Expose(m);
  CUnlocker::Expose(m);
#ifdef STPYV8_FEATURE_CLJS
  CJSObjectCLJS::Expose(m);
#endif
}
