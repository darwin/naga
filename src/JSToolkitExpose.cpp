#include "JSToolkitExpose.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

// ForwardToThis is a helper class which redirects function calls to instance calls (expects first arg to be the instance)
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