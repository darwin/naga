#pragma once

#include "Base.h"

bool isExactTime(py::object py_obj);
bool isExactTime(pb::handle py_obj);
bool isExactDate(py::object py_obj);
bool isExactDate(pb::handle py_obj);
bool isExactDateTime(py::object py_obj);
bool isExactDateTime(pb::handle py_obj);

void getPythonDateTime(py::object py_obj, tm& ts, int& ms);
void getPythonDateTime(pb::handle py_obj, tm& ts, int& ms);
void getPythonTime(py::object py_obj, tm& ts, int& ms);
void getPythonTime(pb::handle py_obj, tm& ts, int& ms);

py::object pythonFromDateAndTime(int year, int month, int day, int hour, int minute, int second, int usecond);
pb::object pythonFromDateAndTime2(int year, int month, int day, int hour, int minute, int second, int usecond);