#include "Base.h"
#include "Platform.h"
#include "Isolate.h"
#include "Context.h"
#include "Engine.h"
#include "Script.h"
#include "JSStackFrame.h"
#include "JSStackTrace.h"
#include "JSException.h"
#include "JSObject.h"
#include "Locker.h"
#include "Unlocker.h"

PYBIND11_MODULE(_STPyV8, m) {
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
}
