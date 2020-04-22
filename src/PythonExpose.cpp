#include "PythonExpose.h"
#include "Platform.h"
#include "JSIsolate.h"
#include "JSEngine.h"
#include "JSScript.h"
#include "JSLocker.h"
#include "JSUnlocker.h"
#include "JSContext.h"
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
  TRACE("exposeJSToolkit py_module={}", py_module);
  py::module m = py_module.def_submodule("toolkit", "Javascript Toolkit");

  // clang-format off
  m.def("linenum", ForwardToThis<&CJSObjectAPI::LineNumber>{},
        py::arg("this"),
        "The line number of function in the script");
  m.def("colnum", ForwardToThis<&CJSObjectAPI::ColumnNumber>{},
        py::arg("this"),
        "The column number of function in the script");
  m.def("resname", ForwardToThis<&CJSObjectAPI::ResourceName>{},
        py::arg("this"),
        "The resource name of script");
  m.def("inferredname", ForwardToThis<&CJSObjectAPI::InferredName>{},
        py::arg("this"),
        "Name inferred from variable or property assignment of this function");
  m.def("lineoff", ForwardToThis<&CJSObjectAPI::LineOffset>{},
        py::arg("this"),
        "The line offset of function in the script");
  m.def("coloff", ForwardToThis<&CJSObjectAPI::ColumnOffset>{},
        py::arg("this"),
        "The column offset of function in the script");
  m.def("set_name", ForwardToThis<&CJSObjectAPI::SetName>{},
        py::arg("this"),
        py::arg("name"));
  m.def("get_name", ForwardToThis<&CJSObjectAPI::GetName>{},
        py::arg("this"));

  m.def("apply", ForwardToThis<&CJSObjectAPI::Apply>{},
        py::arg("this"),
        py::arg("self"),
        py::arg("args") = py::list(),
        py::arg("kwds") = py::dict(),
        "Performs a function call using the parameters.");
  m.def("invoke", ForwardToThis<&CJSObjectAPI::Invoke>{},
        py::arg("this"),
        py::arg("args") = py::list(),
        py::arg("kwds") = py::dict(),
        "Performs a binding method call using the parameters.");
  m.def("clone", ForwardToThis<&CJSObjectAPI::Clone>{},
        py::arg("this"),
        "Clone the object.");

  m.def("create", &CJSObjectAPI::CreateWithArgs,
        py::arg("constructor"),
        py::arg("arguments") = py::tuple(),
        py::arg("propertiesObject") = py::dict(),
        "Creates a new object with the specified prototype object and properties.");

  m.def("has_role_array", [](const CJSObjectPtr &obj) { return obj->HasRole(CJSObjectBase::Roles::Array); });
  m.def("has_role_function", [](const CJSObjectPtr &obj) { return obj->HasRole(CJSObjectBase::Roles::Function); });
  m.def("has_role_cljs", [](const CJSObjectPtr &obj) { return obj->HasRole(CJSObjectBase::Roles::CLJS); });
  // clang-format on
}

void exposeAux(py::module py_module) {
  TRACE("exposeAux py_module={}", py_module);
  py::module m = py_module.def_submodule("aux", "Aux tools");

  m.def("refcount_addr", &refCountAddr);
  m.def("trigger1", &trigger1);
  m.def("trigger2", &trigger2);
  m.def("trigger3", &trigger3);
  m.def("trigger4", &trigger4);
  m.def("trigger5", &trigger5);
  m.def("trace", &trace);
  m.def("v8_request_gc_for_testing", &v8RequestGarbageCollectionForTesting);
}

void exposeJSNull(py::module py_module) {
  TRACE("exposeJSNull py_module={}", py_module);
  // Javascript's null maps to Python's None
  py_module.add_object("JSNull", Py_JSNull);
}

void exposeJSUndefined(py::module py_module) {
  TRACE("exposeJSUndefined py_module={}", py_module);
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
      .def("__getattr__", &CJSObjectAPI::GetAttr)
      .def("__setattr__", &CJSObjectAPI::SetAttr)
      .def("__delattr__", &CJSObjectAPI::DelAttr)

      .def("__hash__", &CJSObjectAPI::Hash)
      .def("__dir__", &CJSObjectAPI::Dir)

      .def("__getitem__", &CJSObjectAPI::GetItem)
      .def("__setitem__", &CJSObjectAPI::SetItem)
      .def("__delitem__", &CJSObjectAPI::DelItem)
      .def("__contains__", &CJSObjectAPI::Contains)

      .def("__len__", &CJSObjectAPI::Len)

      .def("__int__", &CJSObjectAPI::Int)
      .def("__float__", &CJSObjectAPI::Float)
      .def("__str__", &CJSObjectAPI::Str)
      .def("__repr__", &CJSObjectAPI::Repr)
      .def("__bool__", &CJSObjectAPI::Bool)

      .def("__eq__", &CJSObjectAPI::EQ)
      .def("__ne__", &CJSObjectAPI::NE)
      .def("__call__", &CJSObjectAPI::Call)
      // TODO: .def("__iter__", &CJSObjectArray::begin, &CJSObjectArray::end)

      // Emulating dict object
      // TODO: I'm not sure about this, revisit
      // this should go away when we implement __iter__
      //      .def("keys", &CJSObjectAPI::Dir,
      //           "Get a list of the object attributes.")

      ;
  // clang-format on
}

void exposeJSPlatform(py::module py_module) {
  TRACE("exposeJSPlatform py_module={}", py_module);
  // clang-format off
  py::class_<CPlatform>(py_module, "JSPlatform", "JSPlatform allows the V8 platform to be initialized")
      .def_property_readonly_static("instance", [](const py::object &) { return CPlatform::Instance(); },
                                    "Access to platform singleton instance")
      .def_property_readonly("initialized", &CPlatform::Initialized,
                             "Returns true if the init was already called on platform.")
      .def("init", &CPlatform::Init,
           py::arg("argv") = std::string(),
           "Initializes the platform");
  // clang-format on
}

void exposeJSIsolate(py::module py_module) {
  TRACE("exposeJSIsolate py_module={}", py_module);
  // clang-format off
  py::class_<CJSIsolate, CIsolatePtr>(py_module, "JSIsolate", "JSIsolate is an isolated instance of the V8 engine.")
      .def(py::init<>())

      .def_property_readonly_static(
          "current", [](const py::object&) { return CJSIsolate::GetCurrent(); },
          "Returns the entered isolate for the current thread or NULL in case there is no current isolate.")

      .def_property_readonly("locked", &CJSIsolate::IsLocked)

      .def("GetCurrentStackTrace", &CJSIsolate::GetCurrentStackTrace)

      .def("enter", &CJSIsolate::Enter,
           "Sets this isolate as the entered one for the current thread. "
           "Saves the previously entered one (if any), so that it can be "
           "restored when exiting.  Re-entering an isolate is allowed.")

      .def("leave", &CJSIsolate::Leave,
           "Exits this isolate by restoring the previously entered one in the current thread. "
           "The isolate may still stay the same, if it was entered more than once.");
  // clang-format on
}

void exposeJSException(py::module py_module) {
  TRACE("exposeJSError py_module={}", py_module);

  py::register_exception_translator(&translateException);

  // clang-format off
  py::class_<CJSException>(py_module, "JSException")
      .def("__str__", &CJSException::Str)

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

      .def("__str__", &CJSStackTrace::Str);

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
  py::class_<CJSEngine>(py_module, "JSEngine", "JSEngine is a backend Javascript engine.")
      .def(py::init<>(),
           "Create a new script engine instance.")
      .def_property_readonly_static(
          "version", [](const py::object &) { return CJSEngine::GetVersion(); },
          "Get the V8 engine version.")

      .def_property_readonly_static(
          "dead", [](const py::object &) { return CJSEngine::IsDead(); },
          "Check if V8 is dead and therefore unusable.")

      .def_static("setFlags", &CJSEngine::SetFlags,
                  "Sets V8 flags from a string.")

      .def_static("terminateAllThreads", &CJSEngine::TerminateAllThreads,
                  "Forcefully terminate the current thread of JavaScript execution.")

      .def_static(
          "dispose", []() { return v8::V8::Dispose(); },
          "Releases any resources used by v8 and stops any utility threads "
          "that may be running. Note that disposing v8 is permanent, "
          "it cannot be reinitialized.")

      .def_static(
          "lowMemory", []() { v8u::getCurrentIsolate()->LowMemoryNotification(); },
          "Optional notification that the system is running low on memory.")

      .def_static("setStackLimit", &CJSEngine::SetStackLimit,
                  py::arg("stack_limit_size") = 0,
                  "Uses the address of a local variable to determine the stack top now."
                  "Given a size, returns an address that is that far from the current top of stack.")

      .def("compile", &CJSEngine::Compile,
           py::arg("source"),
           py::arg("name") = std::string(),
           py::arg("line") = -1,
           py::arg("col") = -1)
      .def("compile", &CJSEngine::CompileW,
           py::arg("source"),
           py::arg("name") = std::wstring(),
           py::arg("line") = -1,
           py::arg("col") = -1);
  // clang-format on
}

void exposeJSScript(py::module py_module) {
  TRACE("exposeJSScript py_module={}", py_module);
  // clang-format off
  py::class_<CJSScript, CScriptPtr>(py_module, "JSScript", "JSScript is a compiled JavaScript script.")
      .def_property_readonly("source", &CJSScript::GetSource,
                             "the source code")

      .def("run", &CJSScript::Run,
           "Execute the compiled code.");
  // clang-format on
}

void exposeJSLocker(py::module py_module) {
  TRACE("exposeJSLocker py_module={}", py_module);
  // clang-format off
  py::class_<CJSLocker>(py_module, "JSLocker")
      .def(py::init<>())

      .def_property_readonly_static(
          "active", [](const py::object&) { return CJSLocker::IsActive(); },
          "whether Locker is being used by this V8 instance.")
      .def_property_readonly_static(
          "locked", [](const py::object&) { return CJSLocker::IsLocked(); },
          "whether or not the locker is locked by the current thread.")

      .def("entered", &CJSLocker::IsEntered)
      .def("enter", &CJSLocker::Enter)
      .def("leave", &CJSLocker::Leave);
  // clang-format on
}

void exposeJSUnlocker(py::module py_module) {
  TRACE("exposeJSUnlocker py_module={}", py_module);
  // clang-format off
  py::class_<CJSUnlocker>(py_module, "JSUnlocker")
      .def(py::init<>())
      .def("entered", &CJSUnlocker::IsEntered)
      .def("enter", &CJSUnlocker::Enter)
      .def("leave", &CJSUnlocker::Leave);
  // clang-format on
}

void exposeJSContext(py::module py_module) {
  TRACE("exposeJSContext py_module={}", py_module);
  // clang-format off
  py::class_<CJSContext, CContextPtr>(py_module, "JSContext", "JSContext is an execution context.")
      .def(py::init<py::object>())

      .def_property("securityToken", &CJSContext::GetSecurityToken, &CJSContext::SetSecurityToken)

      .def_property_readonly("locals", &CJSContext::GetGlobal,
                             "Local variables within context")

      .def_property_readonly_static("entered", [](const py::object &) { return CJSContext::GetEntered(); },
                                    "The last entered context.")
      .def_property_readonly_static("current", [](const py::object &) { return CJSContext::GetCurrent(); },
                                    "The context that is on the top of the stack.")
      .def_property_readonly_static("calling", [](const py::object &) { return CJSContext::GetCalling(); },
                                    "The context of the calling JavaScript code.")
      .def_property_readonly_static("inContext", [](const py::object &) { return CJSContext::InContext(); },
                                    "Returns true if V8 has a current context.")

      .def_static("eval", &CJSContext::Evaluate,
                  py::arg("source"),
                  py::arg("name") = std::string(),
                  py::arg("line") = -1,
                  py::arg("col") = -1)
      .def_static("eval", &CJSContext::EvaluateW,
                  py::arg("source"),
                  py::arg("name") = std::wstring(),
                  py::arg("line") = -1,
                  py::arg("col") = -1)

      .def("enter", &CJSContext::Enter,
           "Enter this context. "
           "After entering a context, all code compiled and "
           "run is compiled and run in this context.")
      .def("leave", &CJSContext::Leave,
           "Exit this context. "
           "Exiting the current context restores the context "
           "that was in place when entering the current context.")

      .def("__bool__", &CJSContext::IsEntered,
           "the context has been entered.");
  // clang-format on
}