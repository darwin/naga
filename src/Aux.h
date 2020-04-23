#pragma once

#include "Base.h"

py::str refCountAddr(const py::object& py_obj);
void trigger1();
void trigger2();
void trigger3();
void trigger4();
void trigger5();
void trace(const py::str& s);
void v8RequestGarbageCollectionForTesting();
