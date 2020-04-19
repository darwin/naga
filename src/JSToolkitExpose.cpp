#include "JSToolkitExpose.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

// CallThis is a helper class which redirects function calls to instance calls (expects first arg to be the instance)
//
// in python: JSObject.something(instance, arg1, arg2, ...)
// calls (via pybind) our wrapper's operator(), and we in turn call
// instance->Something(arg1, arg2, ...)
// credit: https://stackoverflow.com/a/46533854/84283
template <auto CJSObjectAPI::*F>
struct CallThis {};

// this is a specialization for const member functions
template <class RET, class... ARGS, auto (CJSObjectAPI::*F)(ARGS...) const->RET>
struct CallThis<F> {
  auto operator()(const CJSObjectPtr& obj, ARGS&&... args) const { return ((*obj).*F)(std::forward<ARGS>(args)...); }
};

// this is a specialization for non-const member functions
template <class RET, class... ARGS, auto (CJSObjectAPI::*F)(ARGS...)->RET>
struct CallThis<F> {
  auto operator()(const CJSObjectPtr& obj, ARGS&&... args) const { return ((*obj).*F)(std::forward<ARGS>(args)...); }
};

void exposeJSToolkit(py::module py_module) {
  TRACE("exposeJSObject py_module={}", py_module);
  py::module m = py_module.def_submodule("toolkit", "Javascript Toolkit");

  // clang-format off
  m.def("linenum", CallThis<&CJSObjectAPI::PythonLineNumber>{},
        py::arg("this"),
        "The line number of function in the script");
  m.def("colnum", CallThis<&CJSObjectAPI::PythonColumnNumber>{},
        py::arg("this"),
        "The column number of function in the script");
  m.def("resname", CallThis<&CJSObjectAPI::PythonResourceName>{},
        py::arg("this"),
        "The resource name of script");
  m.def("inferredname", CallThis<&CJSObjectAPI::PythonInferredName>{},
        py::arg("this"),
        "Name inferred from variable or property assignment of this function");
  m.def("lineoff", CallThis<&CJSObjectAPI::PythonLineOffset>{},
        py::arg("this"),
        "The line offset of function in the script");
  m.def("coloff", CallThis<&CJSObjectAPI::PythonColumnOffset>{},
        py::arg("this"),
        "The column offset of function in the script");
  m.def("set_name", CallThis<&CJSObjectAPI::PythonSetName>{},
        py::arg("this"),
        py::arg("name"));
  m.def("get_name", CallThis<&CJSObjectAPI::PythonGetName>{},
        py::arg("this"));

  m.def("apply", CallThis<&CJSObjectAPI::PythonApply>{},
        py::arg("this"),
        py::arg("self"),
        py::arg("args") = py::list(),
        py::arg("kwds") = py::dict(),
        "Performs a function call using the parameters.");
  m.def("invoke", CallThis<&CJSObjectAPI::PythonInvoke>{},
        py::arg("this"),
        py::arg("args") = py::list(),
        py::arg("kwds") = py::dict(),
        "Performs a binding method call using the parameters.");
  m.def("clone", CallThis<&CJSObjectAPI::PythonClone>{},
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