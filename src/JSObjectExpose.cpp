#include "JSObjectExpose.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectLogger), __VA_ARGS__)

void exposeJSObject(py::module py_module) {
  TRACE("exposeJSObject py_module={}", py_module);
  // clang-format off
  py::class_<CJSObject, CJSObjectPtr>(py_module, "JSObject")
      // --- start of protected section ---
      //   please be aware that this wrapper object implements generic lookup of properties on
      //   underlying JS objects, so binding new names here on wrapper instance increases risks
      //   of clashing with some existing JS names
      //   note that __name__ are relatively safe because it is unlikely that javascript names would clash
      //
      //   solution: prefer def_static with passing instance as the first "self" argument
      //             see below for examples
      .def("__getattr__", &CJSObjectAPI::PythonGetAttr)
      .def("__setattr__", &CJSObjectAPI::PythonSetAttr)
      .def("__delattr__", &CJSObjectAPI::PythonDelAttr)

      .def("__hash__", &CJSObjectAPI::PythonIdentityHash)
      .def("__dir__", &CJSObjectAPI::PythonGetAttrList)

      .def("__getitem__", &CJSObjectAPI::PythonGetItem)
      .def("__setitem__", &CJSObjectAPI::PythonSetItem)
      .def("__delitem__", &CJSObjectAPI::PythonDelItem)
      .def("__contains__", &CJSObjectAPI::PythonContains)

      .def("__len__", &CJSObjectAPI::PythonLength)

      .def("__int__", &CJSObjectAPI::PythonInt)
      .def("__float__", &CJSObjectAPI::PythonFloat)
      .def("__str__", &CJSObjectAPI::PythonStr)
      .def("__repr__", &CJSObjectAPI::PythonRepr)
      .def("__bool__", &CJSObjectAPI::PythonBool)

      .def("__eq__", &CJSObjectAPI::PythonEquals)
      .def("__ne__", &CJSObjectAPI::PythonNotEquals)
      .def("__call__", &CJSObjectAPI::PythonCallWithArgs)
      // TODO: .def("__iter__", &CJSObjectArray::begin, &CJSObjectArray::end)

      // Emulating dict object
      // TODO: I'm not sure about this, revisit
      // this should go away when we implement __iter__
      //      .def("keys", &CJSObjectAPI::PythonGetAttrList,
      //           "Get a list of the object attributes.")

      // --- end of protected section ---
      ;
  // clang-format on
}