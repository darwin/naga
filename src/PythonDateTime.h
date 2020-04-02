#pragma once

#include "Base.h"

void initPythonDateTime();

bool isExactTime(py::object obj);
bool isExactDate(py::object obj);
bool isExactDateTime(py::object obj);

void getPythonDateTime(py::object obj, tm& ts, int& ms);
void getPythonTime(py::object obj, tm& ts, int& ms);

py::object pythonFromDateAndTime(int year, int month, int day, int hour, int minute, int second, int usecond);