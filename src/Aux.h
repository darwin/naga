#ifndef NAGA_AUX_H_
#define NAGA_AUX_H_

#include "Base.h"

py::str refCountAddr(const py::object& py_obj);
void trigger1();
void trigger2();
void trigger3();
void trigger4();
void trigger5();
void trace(const py::str& s);
void v8RequestGarbageCollectionForTesting();
SharedJSIsolatePtr testEncounteringForeignIsolate();
SharedJSContextPtr testEncounteringForeignContext();

#endif