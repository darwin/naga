#ifndef NAGA_PYTHONDATETIME_H_
#define NAGA_PYTHONDATETIME_H_

#include "Base.h"

bool isExactTime(py::handle py_obj);
bool isExactDate(py::handle py_obj);
bool isExactDateTime(py::handle py_obj);

void getPythonDateTime(py::handle py_obj, tm& ts, int& ms);
void getPythonTime(py::handle py_obj, tm& ts, int& ms);

py::object pythonFromDateAndTime(int year, int month, int day, int hour, int minute, int second, int usecond);

#endif