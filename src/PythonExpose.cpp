#include "PythonExpose.h"
#include "Platform.h"
#include "Isolate.h"
#include "Engine.h"
#include "Script.h"
#include "Locker.h"
#include "Unlocker.h"
#include "Context.h"
#include "JSNull.h"
#include "JSUndefined.h"
#include "JSObjectAPI.h"
#include "JSStackTrace.h"
#include "JSStackFrame.h"
#include "JSException.h"
#include "Aux.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kExposeLogger), __VA_ARGS__)

// ForwardToThis is a helper class which redirects function calls to instance calls (expects first arg to be the
// instance)
//
// in python: toolkit.something(instance, arg1, arg2, ...)
// calls (via pybind) our wrapper's operator(), and we in turn call
// instance->Something(arg1, arg2, ...)
// credit: https://stackoverflow.com/a/46533854/84283
template <auto CJSObjectAPI::*F>
struct ForwardToThis {};

// this is a specialization for const member functions
template <class RET, class... ARGS, auto (CJSObjectAPI::*F)(ARGS...) const->RET>
struct ForwardToThis<F> {
  auto operator()(const CJSObjectPtr& obj, ARGS&&... args) const { return ((*obj).*F)(std::forward<ARGS>(args)...); }
};

// this is a specialization for non-const member functions
template <class RET, class... ARGS, auto (CJSObjectAPI::*F)(ARGS...)->RET>
struct ForwardToThis<F> {
  auto operator()(const CJSObjectPtr& obj, ARGS&&... args) const { return ((*obj).*F)(std::forward<ARGS>(args)...); }
};

void exposeJSToolkit(py::module py_module) {
  TRACE("exposeJSObject py_module={}", py_module);
  py::module m = py_module.def_submodule("toolkit", "Javascript Toolkit");

  // clang-format off
  m.def("linenum", ForwardToThis<&CJSObjectAPI::PythonLineNumber>{},
        py::arg("this"),
        "The line number of function in the script");
  m.def("colnum", ForwardToThis<&CJSObjectAPI::PythonColumnNumber>{},
        py::arg("this"),
        "The column number of function in the script");
  m.def("resname", ForwardToThis<&CJSObjectAPI::PythonResourceName>{},
        py::arg("this"),
        "The resource name of script");
  m.def("inferredname", ForwardToThis<&CJSObjectAPI::PythonInferredName>{},
        py::arg("this"),
        "Name inferred from variable or property assignment of this function");
  m.def("lineoff", ForwardToThis<&CJSObjectAPI::PythonLineOffset>{},
        py::arg("this"),
        "The line offset of function in the script");
  m.def("coloff", ForwardToThis<&CJSObjectAPI::PythonColumnOffset>{},
        py::arg("this"),
        "The column offset of function in the script");
  m.def("set_name", ForwardToThis<&CJSObjectAPI::PythonSetName>{},
        py::arg("this"),
        py::arg("name"));
  m.def("get_name", ForwardToThis<&CJSObjectAPI::PythonGetName>{},
        py::arg("this"));

  m.def("apply", ForwardToThis<&CJSObjectAPI::PythonApply>{},
        py::arg("this"),
        py::arg("self"),
        py::arg("args") = py::list(),
        py::arg("kwds") = py::dict(),
        "Performs a function call using the parameters.");
  m.def("invoke", ForwardToThis<&CJSObjectAPI::PythonInvoke>{},
        py::arg("this"),
        py::arg("args") = py::list(),
        py::arg("kwds") = py::dict(),
        "Performs a binding method call using the parameters.");
  m.def("clone", ForwardToThis<&CJSObjectAPI::PythonClone>{},
        py::arg("this"),
        "Clone the object.");

  m.def("create", &CJSObjectAPI::PythonCreateWithArgs,
        py::arg("constructor"),
        py::arg("arguments") = py::tuple(),
        py::arg("propertiesObject") = py::dict(),
        "Creates a new object with the specified prototype object and properties.");

  m.def("hasJSArrayRole", [](const CJSObjectPtr &obj) { return obj->HasRole(CJSObjectBase::Roles::JSArray); });
  m.def("hasJSFunctionRole", [](const CJSObjectPtr &obj) { return obj->HasRole(CJSObjectBase::Roles::JSFunction); });
  m.def("hasCLJSObjectRole", [](const CJSObjectPtr &obj) { return obj->HasRole(CJSObjectBase::Roles::CLJSObject); });
  // clang-format on
}

void exposeJSNull(py::module py_module) {
  // Javascript's null maps to Python's None
  py_module.add_object("JSNull", Py_JSNull);
}

void exposeJSUndefined(py::module py_module) {
  // Javascript's undefined maps to our JSUndefined
  py_module.add_object("JSUndefined", Py_JSUndefined);
}

void exposeJSObject(py::module py_module) {
  TRACE("exposeJSObject py_module={}", py_module);
  // clang-format off
  py::class_<CJSObject, CJSObjectPtr>(py_module, "JSObject")
      // Please be aware that this wrapper object implements generic lookup of properties on
      // underlying JS objects, so binding new names here on wrapper instance increases risks
      // of clashing with some existing JS names.
      // Note that internal Python slots in the form of "__name__" are relatively safe names because it is
      // very unlikely that Javascript names would clash.
      //
      // solution: If you want to expose additional functionality, do it as a plain helper function in toolkit module.
      //           The helper can take instance as the first argument and operate on it.
      .def("__getattr__", &CJSObjectAPI::PythonGetAttr)
      .def("__setattr__", &CJSObjectAPI::PythonSetAttr)
      .def("__delattr__", &CJSObjectAPI::PythonDelAttr)

      .def("__hash__", &CJSObjectAPI::PythonHash)
      .def("__dir__", &CJSObjectAPI::PythonDir)

      .def("__getitem__", &CJSObjectAPI::PythonGetItem)
      .def("__setitem__", &CJSObjectAPI::PythonSetItem)
      .def("__delitem__", &CJSObjectAPI::PythonDelItem)
      .def("__contains__", &CJSObjectAPI::PythonContains)

      .def("__len__", &CJSObjectAPI::PythonLen)

      .def("__int__", &CJSObjectAPI::PythonInt)
      .def("__float__", &CJSObjectAPI::PythonFloat)
      .def("__str__", &CJSObjectAPI::PythonStr)
      .def("__repr__", &CJSObjectAPI::PythonRepr)
      .def("__bool__", &CJSObjectAPI::PythonBool)

      .def("__eq__", &CJSObjectAPI::PythonEQ)
      .def("__ne__", &CJSObjectAPI::PythonNE)
      .def("__call__", &CJSObjectAPI::PythonCall)
      // TODO: .def("__iter__", &CJSObjectArray::begin, &CJSObjectArray::end)

      // Emulating dict object
      // TODO: I'm not sure about this, revisit
      // this should go away when we implement __iter__
      //      .def("keys", &CJSObjectAPI::PythonDir,
      //           "Get a list of the object attributes.")

      ;
  // clang-format on
}

void exposeJSPlatform(py::module py_module) {
  TRACE("exposeJSPlatform py_module={}", py_module);
  // clang-format off
  py::class_<CPlatform, CPlatformPtr>(py_module, "JSPlatform", "JSPlatform allows the V8 platform to be initialized")
      .def(py::init<std::string>(),
           py::arg("argv") = std::string())
      .def("init", &CPlatform::Init,
           "Initializes the platform");
  // clang-format on
}

void exposeJSIsolate(py::module py_module) {
  TRACE("exposeJSIsolate py_module={}", py_module);
  // clang-format off
  py::class_<CIsolate, CIsolatePtr>(py_module, "JSIsolate", "JSIsolate is an isolated instance of the V8 engine.")
      .def(py::init<>())

      .def_property_readonly_static(
          "current", [](const py::object&) { return CIsolate::GetCurrent(); },
          "Returns the entered isolate for the current thread or NULL in case there is no current isolate.")

      .def_property_readonly("locked", &CIsolate::IsLocked)

      .def("GetCurrentStackTrace", &CIsolate::GetCurrentStackTrace)

      .def("enter", &CIsolate::Enter,
           "Sets this isolate as the entered one for the current thread. "
           "Saves the previously entered one (if any), so that it can be "
           "restored when exiting.  Re-entering an isolate is allowed.")

      .def("leave", &CIsolate::Leave,
           "Exits this isolate by restoring the previously entered one in the current thread. "
           "The isolate may still stay the same, if it was entered more than once.");
  // clang-format on
}

void exposeJSError(py::module py_module) {
  TRACE("CJSException::Expose py_module={}", py_module);

  py::register_exception_translator(&translateException);

  // clang-format off
  py::class_<CJSException>(py_module, "_JSError")
      .def("__str__", &CJSException::ToPythonStr)

      .def_property_readonly("name", &CJSException::GetName,
                             "The exception name.")
      .def_property_readonly("message", &CJSException::GetMessage,
                             "The exception message.")
      .def_property_readonly("scriptName", &CJSException::GetScriptName,
                             "The script name which throw the exception.")
      .def_property_readonly("lineNum", &CJSException::GetLineNumber, "The line number of error statement.")
      .def_property_readonly("startPos", &CJSException::GetStartPosition,
                             "The start position of error statement in the script.")
      .def_property_readonly("endPos", &CJSException::GetEndPosition,
                             "The end position of error statement in the script.")
      .def_property_readonly("startCol", &CJSException::GetStartColumn,
                             "The start column of error statement in the script.")
      .def_property_readonly("endCol", &CJSException::GetEndColumn,
                             "The end column of error statement in the script.")
      .def_property_readonly("sourceLine", &CJSException::GetSourceLine,
                             "The source line of error statement.")
      .def_property_readonly("stackTrace", &CJSException::GetStackTrace,
                             "The stack trace of error statement.")
      .def("print_tb", &CJSException::PrintCallStack,
           py::arg("file") = py::none(),
           "Print the stack trace of error statement.");
  // clang-format on
}

void exposeJSStackFrame(py::module py_module) {
  TRACE("exposeJSStackFrame py_module={}", py_module);
  // clang-format off
  py::class_<CJSStackFrame, CJSStackFramePtr>(py_module, "JSStackFrame")
      .def_property_readonly("lineNum", &CJSStackFrame::GetLineNumber)
      .def_property_readonly("column", &CJSStackFrame::GetColumn)
      .def_property_readonly("scriptName", &CJSStackFrame::GetScriptName)
      .def_property_readonly("funcName", &CJSStackFrame::GetFunctionName)
      .def_property_readonly("isEval", &CJSStackFrame::IsEval)
      .def_property_readonly("isConstructor", &CJSStackFrame::IsConstructor);
  // clang-format on
}

void exposeJSStackTrace(py::module py_module) {
  TRACE("exposeJSStackTrace py_module={}", py_module);
  // clang-format off
  py::class_<CJSStackTrace, CJSStackTracePtr>(py_module, "JSStackTrace")
      .def("__len__", &CJSStackTrace::GetFrameCount)
      .def("__getitem__", &CJSStackTrace::GetFrame)

          // TODO: .def("__iter__", py::range(&CJavascriptStackTrace::begin, &CJavascriptStackTrace::end))

      .def("__str__", &CJSStackTrace::ToPythonStr);

  py::enum_<v8::StackTrace::StackTraceOptions>(py_module, "JSStackTraceOptions")
      .value("LineNumber", v8::StackTrace::kLineNumber)
      .value("ColumnOffset", v8::StackTrace::kColumnOffset)
      .value("ScriptName", v8::StackTrace::kScriptName)
      .value("FunctionName", v8::StackTrace::kFunctionName)
      .value("IsEval", v8::StackTrace::kIsEval)
      .value("IsConstructor", v8::StackTrace::kIsConstructor)
      .value("Overview", v8::StackTrace::kOverview)
      .value("Detailed", v8::StackTrace::kDetailed)
      .export_values();
  // clang-format on
}

void exposeJSEngine(py::module py_module) {
  TRACE("exposeJSEngine py_module={}", py_module);
  // clang-format off
  py::class_<CEngine>(py_module, "JSEngine", "JSEngine is a backend Javascript engine.")
      .def(py::init<>(),
           "Create a new script engine instance.")
      .def_property_readonly_static(
          "version", [](const py::object &) { return CEngine::GetVersion(); },
          "Get the V8 engine version.")

      .def_property_readonly_static(
          "dead", [](const py::object &) { return CEngine::IsDead(); },
          "Check if V8 is dead and therefore unusable.")

      .def_static("setFlags", &CEngine::SetFlags,
                  "Sets V8 flags from a string.")

      .def_static("terminateAllThreads", &CEngine::TerminateAllThreads,
                  "Forcefully terminate the current thread of JavaScript execution.")

      .def_static(
          "dispose", []() { return v8::V8::Dispose(); },
          "Releases any resources used by v8 and stops any utility threads "
          "that may be running. Note that disposing v8 is permanent, "
          "it cannot be reinitialized.")

      .def_static(
          "lowMemory", []() { v8u::getCurrentIsolate()->LowMemoryNotification(); },
          "Optional notification that the system is running low on memory.")

      .def_static("setStackLimit", &CEngine::SetStackLimit,
                  py::arg("stack_limit_size") = 0,
                  "Uses the address of a local variable to determine the stack top now."
                  "Given a size, returns an address that is that far from the current top of stack.")

      .def("compile", &CEngine::Compile,
           py::arg("source"),
           py::arg("name") = std::string(),
           py::arg("line") = -1,
           py::arg("col") = -1)
      .def("compile", &CEngine::CompileW,
           py::arg("source"),
           py::arg("name") = std::wstring(),
           py::arg("line") = -1,
           py::arg("col") = -1);
  // clang-format on
}

void exposeJSScript(py::module py_module) {
  TRACE("exposeJSScript py_module={}", py_module);
  // clang-format off
  py::class_<CScript, CScriptPtr>(py_module, "JSScript", "JSScript is a compiled JavaScript script.")
      .def_property_readonly("source", &CScript::GetSource,
                             "the source code")

      .def("run", &CScript::Run,
           "Execute the compiled code.");
  // clang-format on
}

void exposeJSLocker(py::module py_module) {
  TRACE("exposeJSLocker py_module={}", py_module);
  // clang-format off
  py::class_<CLocker>(py_module, "JSLocker")
      .def(py::init<>())

      .def_property_readonly_static(
          "active", [](const py::object&) { return CLocker::IsActive(); },
          "whether Locker is being used by this V8 instance.")
      .def_property_readonly_static(
          "locked", [](const py::object&) { return CLocker::IsLocked(); },
          "whether or not the locker is locked by the current thread.")

      .def("entered", &CLocker::IsEntered)
      .def("enter", &CLocker::Enter)
      .def("leave", &CLocker::Leave);
  // clang-format on
}

void exposeJSUnlocker(py::module py_module) {
  TRACE("exposeJSUnlocker py_module={}", py_module);
  // clang-format off
  py::class_<CUnlocker>(py_module, "JSUnlocker")
      .def(py::init<>())
      .def("entered", &CUnlocker::IsEntered)
      .def("enter", &CUnlocker::Enter)
      .def("leave", &CUnlocker::Leave);
  // clang-format on
}

void exposeJSContext(py::module py_module) {
  TRACE("exposeJSContext py_module={}", py_module);
  // clang-format off
  py::class_<CContext, CContextPtr>(py_module, "JSContext", "JSContext is an execution context.")
      .def(py::init<py::object>())

      .def_property("securityToken", &CContext::GetSecurityToken, &CContext::SetSecurityToken)

      .def_property_readonly("locals", &CContext::GetGlobal,
                             "Local variables within context")

      .def_property_readonly_static("entered", [](const py::object &) { return CContext::GetEntered(); },
                                    "The last entered context.")
      .def_property_readonly_static("current", [](const py::object &) { return CContext::GetCurrent(); },
                                    "The context that is on the top of the stack.")
      .def_property_readonly_static("calling", [](const py::object &) { return CContext::GetCalling(); },
                                    "The context of the calling JavaScript code.")
      .def_property_readonly_static("inContext", [](const py::object &) { return CContext::InContext(); },
                                    "Returns true if V8 has a current context.")

      .def_static("eval", &CContext::Evaluate,
                  py::arg("source"),
                  py::arg("name") = std::string(),
                  py::arg("line") = -1,
                  py::arg("col") = -1)
      .def_static("eval", &CContext::EvaluateW,
                  py::arg("source"),
                  py::arg("name") = std::wstring(),
                  py::arg("line") = -1,
                  py::arg("col") = -1)

      .def("enter", &CContext::Enter,
           "Enter this context. "
           "After entering a context, all code compiled and "
           "run is compiled and run in this context.")
      .def("leave", &CContext::Leave,
           "Exit this context. "
           "Exiting the current context restores the context "
           "that was in place when entering the current context.")

      .def("__bool__", &CContext::IsEntered,
           "the context has been entered.");
  // clang-format on
}

void exposeAux(py::module py_module) {
  py_module.def("refcount_addr", &refCountAddr);
  py_module.def("trigger1", &trigger1);
  py_module.def("trigger2", &trigger2);
  py_module.def("trigger3", &trigger3);
  py_module.def("trigger4", &trigger4);
  py_module.def("trigger5", &trigger5);
  py_module.def("trace", &trace);
  py_module.def("v8_request_gc_for_testing", &v8RequestGarbageCollectionForTesting);
}
