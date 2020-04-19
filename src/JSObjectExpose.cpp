#include "JSObjectExpose.h"
#include "JSObjectAPI.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

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