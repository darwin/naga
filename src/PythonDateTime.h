#pragma once

#include "Base.h"

bool isExactTime(pb::handle py_obj);
bool isExactDate(pb::handle py_obj);
bool isExactDateTime(pb::handle py_obj);

void getPythonDateTime(pb::handle py_obj, tm& ts, int& ms);
void getPythonTime(pb::handle py_obj, tm& ts, int& ms);

pb::object pythonFromDateAndTime(int year, int month, int day, int hour, int minute, int second, int usecond);