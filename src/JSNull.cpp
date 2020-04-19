#include "JSNull.h"

void exposeJSNull(py::module* py_module) {
  // Javascript's null maps to Python's None
  py_module->add_object("JSNull", Py_None);
}