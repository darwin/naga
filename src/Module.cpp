#include "Base.h"
#include "Context.h"
#include "Engine.h"
#include "Exception.h"
#include "Locker.h"
#include "JSObject.h"

BOOST_PYTHON_MODULE(_STPyV8) {
  // TODO: remove me
  //  this is just temporary for sharing module during boost -> pybind transition
  auto pm = py::scope().ptr();
  assert(pm);
  auto m = pb::reinterpret_borrow<pb::module>(pm);

  CJavascriptException::Expose();
  CJSObject::Expose();
  CContext::Expose();
  CEngine::Expose();
  CLocker::Expose(m);
  CUnlocker::Expose(m);
}