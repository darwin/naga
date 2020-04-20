#include "JSUndefined.h"

// this code was taken from CPython / object.c
// it is a light-weight implementation of None-like object
// we use it to represent Javascript's `undefined` on Python side

/*
There is (and should be!) no way to create other objects of this type,
so there is exactly one (which is indestructible, by the way).
*/

static PyObject* jsundefined_repr(PyObject* op) {
  return PyUnicode_FromString("JSUndefined");
}

static void jsundefined_dealloc(PyObject* ignore) {
  /* This should never get called, but we also don't want to SEGV if
   * we accidentally decref JSUndefined out of existence.
   */
  Py_FatalError("deallocating JSUndefined");
}

static PyObject* jsundefined_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
  if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
    PyErr_SetString(PyExc_TypeError, "JSUndefinedType takes no arguments");
    return NULL;
  }
  Py_RETURN_JSUndefined;
}

static int jsundefined_bool(PyObject* v) {
  return 0;
}

static PyNumberMethods jsundefined_as_number = {
    0,                         /* nb_add */
    0,                         /* nb_subtract */
    0,                         /* nb_multiply */
    0,                         /* nb_remainder */
    0,                         /* nb_divmod */
    0,                         /* nb_power */
    0,                         /* nb_negative */
    0,                         /* nb_positive */
    0,                         /* nb_absolute */
    (inquiry)jsundefined_bool, /* nb_bool */
    0,                         /* nb_invert */
    0,                         /* nb_lshift */
    0,                         /* nb_rshift */
    0,                         /* nb_and */
    0,                         /* nb_xor */
    0,                         /* nb_or */
    0,                         /* nb_int */
    0,                         /* nb_reserved */
    0,                         /* nb_float */
    0,                         /* nb_inplace_add */
    0,                         /* nb_inplace_subtract */
    0,                         /* nb_inplace_multiply */
    0,                         /* nb_inplace_remainder */
    0,                         /* nb_inplace_power */
    0,                         /* nb_inplace_lshift */
    0,                         /* nb_inplace_rshift */
    0,                         /* nb_inplace_and */
    0,                         /* nb_inplace_xor */
    0,                         /* nb_inplace_or */
    0,                         /* nb_floor_divide */
    0,                         /* nb_true_divide */
    0,                         /* nb_inplace_floor_divide */
    0,                         /* nb_inplace_true_divide */
    0,                         /* nb_index */
};

PyTypeObject _PyJSUndefined_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0) "JSUndefinedType",
    0,
    0,
    jsundefined_dealloc,
    /*tp_dealloc*/          /*never called*/
    0,                      /*tp_print*/
    0,                      /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                      /*tp_reserved*/
    jsundefined_repr,       /*tp_repr*/
    &jsundefined_as_number, /*tp_as_number*/
    0,                      /*tp_as_sequence*/
    0,                      /*tp_as_mapping*/
    0,                      /*tp_hash */
    0,                      /*tp_call */
    0,                      /*tp_str */
    0,                      /*tp_getattro */
    0,                      /*tp_setattro */
    0,                      /*tp_as_buffer */
    Py_TPFLAGS_DEFAULT,     /*tp_flags */
    0,                      /*tp_doc */
    0,                      /*tp_traverse */
    0,                      /*tp_clear */
    0,                      /*tp_richcompare */
    0,                      /*tp_weaklistoffset */
    0,                      /*tp_iter */
    0,                      /*tp_iternext */
    0,                      /*tp_methods */
    0,                      /*tp_members */
    0,                      /*tp_getset */
    0,                      /*tp_base */
    0,                      /*tp_dict */
    0,                      /*tp_descr_get */
    0,                      /*tp_descr_set */
    0,                      /*tp_dictoffset */
    0,                      /*tp_init */
    0,                      /*tp_alloc */
    jsundefined_new,        /*tp_new */
};

PyObject _Py_JSUndefinedStruct = {_PyObject_EXTRA_INIT 1, &_PyJSUndefined_Type};