#include "PythonExpose.h"
#include "JSPlatform.h"
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
#include "PybindNagaClass.h"
#include "PybindNagaModule.h"
#include "JSObject.h"
#include "Logging.h"
#include "V8XUtils.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kPythonExposeLogger), __VA_ARGS__)

void exposeJSNull(py::module py_module) {
  TRACE("exposeJSNull py_module={}", py_module);
  py_module.add_object("JSNull", Py_JSNull);
}

void exposeJSUndefined(py::module py_module) {
  TRACE("exposeJSUndefined py_module={}", py_module);
  py_module.add_object("JSUndefined", Py_JSUndefined);
}

void exposeAux(py::module py_module) {
  TRACE("exposeAux py_module={}", py_module);
  auto doc = "Aux tools";
  py::naga_module(py_module.def_submodule("aux", doc))                            //
      .def("refcount_addr", &refCountAddr)                                        //
      .def("trigger1", &trigger1)                                                 //
      .def("trigger2", &trigger2)                                                 //
      .def("trigger3", &trigger3)                                                 //
      .def("trigger4", &trigger4)                                                 //
      .def("trigger5", &trigger5)                                                 //
      .def("trace", &trace)                                                       //
      .def("v8_request_gc_for_testing", &v8RequestGarbageCollectionForTesting)    //
      .def("test_encountering_foreign_isolate", &testEncounteringForeignIsolate)  //
      .def("test_encountering_foreign_context", &testEncounteringForeignContext)  //
      ;
}

void exposeToolkit(py::module py_module) {
  TRACE("exposeToolkit py_module={}", py_module);
  auto doc = "Javascript Toolkit";
  py::naga_module(py_module.def_submodule("toolkit", doc))                              //
      .def("line_number", ForwardTo<&CJSObjectAPI::LineNumber>{},                       //
           py::arg("this"),                                                             //
           "The line number of function in the script")                                 //
      .def("column_number", ForwardTo<&CJSObjectAPI::ColumnNumber>{},                   //
           py::arg("this"),                                                             //
           "The column number of function in the script")                               //
      .def("resource_name", ForwardTo<&CJSObjectAPI::ResourceName>{},                   //
           py::arg("this"),                                                             //
           "The resource name of script")                                               //
      .def("inferred_name", ForwardTo<&CJSObjectAPI::InferredName>{},                   //
           py::arg("this"),                                                             //
           "Name inferred from variable or property assignment of this function")       //
      .def("line_offset", ForwardTo<&CJSObjectAPI::LineOffset>{},                       //
           py::arg("this"),                                                             //
           "The line offset of function in the script")                                 //
      .def("column_offset", ForwardTo<&CJSObjectAPI::ColumnOffset>{},                   //
           py::arg("this"),                                                             //
           "The column offset of function in the script")                               //
      .def("set_name", ForwardTo<&CJSObjectAPI::SetName>{},                             //
           py::arg("this"),                                                             //
           py::arg("name"))                                                             //
      .def("get_name", ForwardTo<&CJSObjectAPI::GetName>{},                             //
           py::arg("this"))                                                             //
      .def("apply", ForwardTo<&CJSObjectAPI::Apply>{},                                  //
           py::arg("this"),                                                             //
           py::arg("self"),                                                             //
           py::arg("args") = py::list(),                                                //
           py::arg("kwds") = py::dict(),                                                //
           "Performs a function call using the parameters.")                            //
      .def("invoke", ForwardTo<&CJSObjectAPI::Invoke>{},                                //
           py::arg("this"),                                                             //
           py::arg("args") = py::list(),                                                //
           py::arg("kwds") = py::dict(),                                                //
           "Performs a binding method call using the parameters.")                      //
      .def("clone", ForwardTo<&CJSObjectAPI::Clone>{},                                  //
           py::arg("this"),                                                             //
           "Clone the object.")                                                         //
      .def("create", &CJSObjectAPI::Create,                                             //
           py::arg("constructor"),                                                      //
           py::arg("arguments") = py::tuple(),                                          //
           py::arg("propertiesObject") = py::dict(),                                    //
           "Creates a new object with the specified prototype object and properties.")  //
      .def("has_role_array", ForwardTo<&CJSObjectAPI::HasRoleArray>{})                  //
      .def("has_role_function", ForwardTo<&CJSObjectAPI::HasRoleFunction>{})            //
      .def("has_role_cljs", ForwardTo<&CJSObjectAPI::HasRoleCLJS>{})                    //
      ;
}

void exposeJSObject(py::module py_module) {
  TRACE("exposeJSObject py_module={}", py_module);
  py::naga_class<CJSObject, CJSObjectPtr>(py_module, "JSObject")
      // Please be aware that this wrapper object implements generic lookup of properties on
      // underlying JS objects, so binding new names here on wrapper instance increases risks
      // of clashing with some existing JS names.
      // Note that internal Python slots in the form of "__name__" are relatively safe names because it is
      // very unlikely that Javascript names would clash.
      //
      // solution: If you want to expose additional functionality, do it as a plain helper function in toolkit module.
      //           The helper can take instance as the first argument and operate on it.
      .def_method("__getattr__", &CJSObjectAPI::GetAttr)    //
      .def_method("__setattr__", &CJSObjectAPI::SetAttr)    //
      .def_method("__delattr__", &CJSObjectAPI::DelAttr)    //
                                                            //
      .def_method("__hash__", &CJSObjectAPI::Hash)          //
      .def_method("__dir__", &CJSObjectAPI::Dir)            //
                                                            //
      .def_method("__getitem__", &CJSObjectAPI::GetItem)    //
      .def_method("__setitem__", &CJSObjectAPI::SetItem)    //
      .def_method("__delitem__", &CJSObjectAPI::DelItem)    //
      .def_method("__contains__", &CJSObjectAPI::Contains)  //
                                                            //
      .def_method("__len__", &CJSObjectAPI::Len)            //
                                                            //
      .def_method("__int__", &CJSObjectAPI::Int)            //
      .def_method("__float__", &CJSObjectAPI::Float)        //
      .def_method("__str__", &CJSObjectAPI::Str)            //
      .def_method("__repr__", &CJSObjectAPI::Repr)          //
      .def_method("__bool__", &CJSObjectAPI::Bool)          //
                                                            //
      .def_method("__eq__", &CJSObjectAPI::EQ)              //
      .def_method("__ne__", &CJSObjectAPI::NE)              //
      .def_method("__call__", &CJSObjectAPI::Call)          //
      // TODO: .def("__iter__", &CJSObjectArray::begin, &CJSObjectArray::end)
      //
      // Emulating dict object
      // TODO: I'm not sure about this, revisit
      // this should go away when we implement __iter__
      //      .def("keys", &CJSObjectAPI::Dir,
      //           "Get a list of the object attributes.")
      ;
}

void exposeJSPlatform(py::module py_module) {
  TRACE("exposeJSPlatform py_module={}", py_module);
  auto doc = "JSPlatform allows the V8 platform to be initialized";
  py::naga_class<CJSPlatform>(py_module, "JSPlatform", doc)                        //
      .def_property_rs(                                                            //
          "instance", StaticCall<&CJSPlatform::Instance>{},                        //
          "Access to platform singleton instance")                                 //
      .def_property_r("initialized", &CJSPlatform::Initialized,                    //
                      "Returns true if the init was already called on platform.")  //
      .def_method("init", &CJSPlatform::Init,                                      //
                  py::arg("argv") = std::string(),                                 //
                  "Initializes the platform")                                      //
      ;
}

void exposeJSIsolate(py::module py_module) {
  TRACE("exposeJSIsolate py_module={}", py_module);
  auto doc = "JSIsolate is an isolated instance of the V8 engine.";
  py::naga_class<CJSIsolate, CJSIsolatePtr>(py_module, "JSIsolate", doc)  //
      .def_ctor(py::init<>())                                             //
                                                                          //
      .def_property_rs(
          "current", StaticCall<&CJSIsolate::GetCurrent>{},                                                   //
          "Returns the entered isolate for the current thread or NULL in case there is no current isolate.")  //
                                                                                                              //
      .def_property_r("locked", &CJSIsolate::IsLocked)                                                        //
                                                                                                              //
      .def_method("get_current_stack_trace", &CJSIsolate::GetCurrentStackTrace)                               //
                                                                                                              //
      .def_method("enter", &CJSIsolate::Enter,                                                                //
                  "Sets this isolate as the entered one for the current thread. "                             //
                  "Saves the previously entered one (if any), so that it can be "                             //
                  "restored when exiting.  Re-entering an isolate is allowed.")                               //
                                                                                                              //
      .def_method("leave", &CJSIsolate::Leave,                                                                //
                  "Exits this isolate by restoring the previously entered one in the current thread. "        //
                  "The isolate may still stay the same, if it was entered more than once.")                   //
                                                                                                              //
      .def_property_r("entered_or_microtask_context", &CJSIsolate::GetEnteredOrMicrotaskContext,              //
                      "Returns either the last context entered through V8's C++ API, "                        //
                      "or the context of the currently running microtask while processing microtasks."        //
                      "If a context is entered while executing a microtask, that context is returned.")       //
                                                                                                              //
      .def_property_r("current_context", &CJSIsolate::GetCurrentContext,                                      //
                      "Returns the context of the currently running JavaScript, "                             //
                      "or the context on the top of the stack if no JavaScript is running.")                  //
                                                                                                              //
      .def_property_r("in_context", &CJSIsolate::InContext,                                                   //
                      "Returns true if this isolate has a current context.")                                  //
      ;
}

void exposeJSException(py::module py_module) {
  TRACE("exposeJSError py_module={}", py_module);

  py::register_exception_translator(&translateException);

  py::naga_class<CJSException>(py_module, "JSException")                                                   //
      .def_method("__str__", &CJSException::Str)                                                           //
                                                                                                           //
      .def_property_r("name", &CJSException::GetName,                                                      //
                      "The exception name.")                                                               //
      .def_property_r("message", &CJSException::GetMessage,                                                //
                      "The exception message.")                                                            //
      .def_property_r("script_name", &CJSException::GetScriptName,                                         //
                      "The script name which throw the exception.")                                        //
      .def_property_r("line_number", &CJSException::GetLineNumber, "The line number of error statement.")  //
      .def_property_r("startpos", &CJSException::GetStartPosition,                                         //
                      "The start position of error statement in the script.")                              //
      .def_property_r("endpos", &CJSException::GetEndPosition,                                             //
                      "The end position of error statement in the script.")                                //
      .def_property_r("startcol", &CJSException::GetStartColumn,                                           //
                      "The start column of error statement in the script.")                                //
      .def_property_r("endcol", &CJSException::GetEndColumn,                                               //
                      "The end column of error statement in the script.")                                  //
      .def_property_r("source_line", &CJSException::GetSourceLine,                                         //
                      "The source line of error statement.")                                               //
      .def_property_r("stack_trace", &CJSException::GetStackTrace,                                         //
                      "The stack trace of error statement.")                                               //
      .def_method("print_stack_trace", &CJSException::PrintStackTrace,                                     //
                  py::arg("file") = py::none(),                                                            //
                  "Print the stack trace of error statement.")                                             //
      ;
}

void exposeJSStackFrame(py::module py_module) {
  TRACE("exposeJSStackFrame py_module={}", py_module);
  py::naga_class<CJSStackFrame, CJSStackFramePtr>(py_module, "JSStackFrame")  //
      .def_property_r("line_number", &CJSStackFrame::GetLineNumber)           //
      .def_property_r("column_number", &CJSStackFrame::GetColumnNumber)       //
      .def_property_r("script_name", &CJSStackFrame::GetScriptName)           //
      .def_property_r("function_name", &CJSStackFrame::GetFunctionName)       //
      .def_property_r("is_eval", &CJSStackFrame::IsEval)                      //
      .def_property_r("is_constructor", &CJSStackFrame::IsConstructor)        //
      ;
}

void exposeJSStackTrace(py::module py_module) {
  TRACE("exposeJSStackTrace py_module={}", py_module);
  py::naga_class<CJSStackTrace, CJSStackTracePtr>(py_module, "JSStackTrace")  //
      .def_method("__len__", &CJSStackTrace::GetFrameCount)                   //
      .def_method("__getitem__", &CJSStackTrace::GetFrame)                    //
                                                                              //
      // TODO: .def("__iter__", py::range(&CJavascriptStackTrace::begin,
      // &CJavascriptStackTrace::end))
      //
      .def_method("__str__", &CJSStackTrace::Str);  //

  py::enum_<v8::StackTrace::StackTraceOptions>(py_module, "JSStackTraceOptions")  //
      .value("LineNumber", v8::StackTrace::kLineNumber)                           //
      .value("ColumnOffset", v8::StackTrace::kColumnOffset)                       //
      .value("ScriptName", v8::StackTrace::kScriptName)                           //
      .value("FunctionName", v8::StackTrace::kFunctionName)                       //
      .value("IsEval", v8::StackTrace::kIsEval)                                   //
      .value("IsConstructor", v8::StackTrace::kIsConstructor)                     //
      .value("Overview", v8::StackTrace::kOverview)                               //
      .value("Detailed", v8::StackTrace::kDetailed)                               //
      .export_values()                                                            //
      ;
}

void exposeJSEngine(py::module py_module) {
  TRACE("exposeJSEngine py_module={}", py_module);
  auto doc = "JSEngine is a backend Javascript engine.";
  py::naga_class<CJSEngine>(py_module, "JSEngine", doc)                                                  //
      .def_ctor(py::init<>(),                                                                            //
                "Create a new script engine instance.")                                                  //
      .def_property_rs("version", StaticCall<&CJSEngine::GetVersion>{},                                  //
                       "Get the V8 engine version.")                                                     //
                                                                                                         //
      .def_property_rs("dead", StaticCall<&CJSEngine::IsDead>{},                                         //
                       "Check if V8 is dead and therefore unusable.")                                    //
                                                                                                         //
      .def_method_s("set_flags", &CJSEngine::SetFlags,                                                   //
                    "Sets V8 flags from a string.")                                                      //
                                                                                                         //
      .def_method_s("terminate_all_threads", &CJSEngine::TerminateAllThreads,                            //
                    "Forcefully terminate the current thread of JavaScript execution.")                  //
                                                                                                         //
      .def_method_s("dispose", StaticCall<&v8::V8::Dispose>{},                                           //
                    "Releases any resources used by v8 and stops any utility threads "                   //
                    "that may be running. Note that disposing v8 is permanent, "                         //
                    "it cannot be reinitialized.")                                                       //
                                                                                                         //
      .def_method_s(                                                                                     //
          "low_memory", []() { v8x::getCurrentIsolate()->LowMemoryNotification(); },                     //
          "Optional notification that the system is running low on memory.")                             //
                                                                                                         //
      .def_method_s("set_stack_limit", &CJSEngine::SetStackLimit,                                        //
                    py::arg("stack_limit_size") = 0,                                                     //
                    "Uses the address of a local variable to determine the stack top now."               //
                    "Given a size, returns an address that is that far from the current top of stack.")  //
                                                                                                         //
      .def_method("compile", &CJSEngine::Compile,                                                        //
                  py::arg("source"),                                                                     //
                  py::arg("name") = std::string(),                                                       //
                  py::arg("line") = -1,                                                                  //
                  py::arg("col") = -1)                                                                   //
      .def_method("compile", &CJSEngine::CompileW,                                                       //
                  py::arg("source"),                                                                     //
                  py::arg("name") = std::wstring(),                                                      //
                  py::arg("line") = -1,                                                                  //
                  py::arg("col") = -1)                                                                   //
      ;
}

void exposeJSScript(py::module py_module) {
  TRACE("exposeJSScript py_module={}", py_module);
  auto doc = "JSScript is a compiled JavaScript script.";
  py::naga_class<CJSScript, CJSScriptPtr>(py_module, "JSScript", doc)  //
      .def_property_r("source", &CJSScript::GetSource,                 //
                      "the source code")                               //
                                                                       //
      .def_method("run", &CJSScript::Run,                              //
                  "Execute the compiled code.")                        //
      ;
}

void exposeJSLocker(py::module py_module) {
  TRACE("exposeJSLocker py_module={}", py_module);
  py::naga_class<CJSLocker>(py_module, "JSLocker")                                    //
      .def_ctor(py::init<>())                                                         //
                                                                                      //
      .def_property_rs("active", StaticCall<&CJSLocker::IsActive>{},                  //
                       "whether Locker is being used by this V8 instance.")           //
      .def_property_rs("locked", StaticCall<&CJSLocker::IsLocked>{},                  //
                       "whether or not the locker is locked by the current thread.")  //
                                                                                      //
      .def_method("entered", &CJSLocker::IsEntered)                                   //
      .def_method("enter", &CJSLocker::Enter)                                         //
      .def_method("leave", &CJSLocker::Leave)                                         //
      ;
}

void exposeJSUnlocker(py::module py_module) {
  TRACE("exposeJSUnlocker py_module={}", py_module);
  py::naga_class<CJSUnlocker>(py_module, "JSUnlocker")  //
      .def_ctor(py::init<>())                           //
      .def_method("entered", &CJSUnlocker::IsEntered)   //
      .def_method("enter", &CJSUnlocker::Enter)         //
      .def_method("leave", &CJSUnlocker::Leave)         //
      ;
}

void exposeJSContext(py::module py_module) {
  TRACE("exposeJSContext py_module={}", py_module);
  py::naga_class<CJSContext, CJSContextPtr>(py_module, "JSContext", "JSContext is an execution context.")  //
      .def_ctor(py::init<py::object>())                                                                    //
                                                                                                           //
      .def_property("security_token", &CJSContext::GetSecurityToken, &CJSContext::SetSecurityToken)        //
                                                                                                           //
      .def_property_r("locals", &CJSContext::GetGlobal,                                                    //
                      "Local variables within context")                                                    //
                                                                                                           //
      .def_property_rs("current", StaticCall<&CJSContext::GetCurrent>{},                                   //
                       "The context that is on the top of the stack.")                                     //
                                                                                                           //
      .def_method_s("eval", &CJSContext::Evaluate,                                                         //
                    py::arg("source"),                                                                     //
                    py::arg("name") = std::string(),                                                       //
                    py::arg("line") = -1,                                                                  //
                    py::arg("col") = -1)                                                                   //
      .def_method_s("eval", &CJSContext::EvaluateW,                                                        //
                    py::arg("source"),                                                                     //
                    py::arg("name") = std::wstring(),                                                      //
                    py::arg("line") = -1,                                                                  //
                    py::arg("col") = -1)                                                                   //
                                                                                                           //
      .def_method("enter", &CJSContext::Enter,                                                             //
                  "Enter this context. "                                                                   //
                  "After entering a context, all code compiled and "                                       //
                  "run is compiled and run in this context.")                                              //
      .def_method("leave", &CJSContext::Leave,                                                             //
                  "Exit this context. "                                                                    //
                  "Exiting the current context restores the context "                                      //
                  "that was in place when entering the current context.")                                  //
      ;
}