#include "Debug.h"

// clang-format off
#define ORIG_Py_INCREF(op) (                    \
    _Py_INC_REFTOTAL  _Py_REF_DEBUG_COMMA       \
    ((PyObject *)(op))->ob_refcnt++)

#define ORIG_Py_DECREF(op)                              \
    do {                                                \
        PyObject *_py_decref_tmp = (PyObject *)(op);    \
        if (_Py_DEC_REFTOTAL  _Py_REF_DEBUG_COMMA       \
        --(_py_decref_tmp)->ob_refcnt != 0)             \
            _Py_CHECK_REFCNT(_py_decref_tmp)            \
        else                                            \
            _Py_Dealloc(_py_decref_tmp);                \
    } while (0)
// clang-format on

void debugPyIncRef(PyObject* op) {
//  auto n = _PyType_Name(Py_TYPE(op));
//  if (n) {
//    std::cerr << " I:" << op << " " << n << " +1\n";
//  } else {
//    std::cerr << op << " ???" << " +1\n";
//  }
  ORIG_Py_INCREF(op);
}
void debugPyDecRef(PyObject* op) {
//  auto n = _PyType_Name(Py_TYPE(op));
//  if (n) {
//    std::cerr << " I:" << op << " " << n << " -1\n";
//  } else {
//    std::cerr << op << " ???" << " -1\n";
//  }
  ORIG_Py_DECREF(op);
}

void debugPyIncRef(PyTypeObject* op) {
//  auto n = _PyType_Name(op);
//  if (n) {
//    std::cerr << " T:" << op << " " << n << " +1\n";
//  } else {
//    std::cerr << op << " ???" << " +1\n";
//  }
  ORIG_Py_INCREF(op);
}
void debugPyDecRef(PyTypeObject* op) {
//  auto n = _PyType_Name(op);
//  if (n) {
//    std::cerr << " T:" << op << " " << n << " -1\n";
//  } else {
//    std::cerr << op << " ???" << " -1\n";
//  }
  ORIG_Py_DECREF(op);
}

void debugPyIncRef(PyCFunctionObject* op) {
  ORIG_Py_INCREF(op);
}

void debugPyDecRef(PyCFunctionObject* op) {
  ORIG_Py_DECREF(op);
}

void debugPyIncRef(PyModuleDef* op) {
  ORIG_Py_INCREF(op);
}

void debugPyDecRef(PyModuleDef* op) {
  ORIG_Py_DECREF(op);
}
