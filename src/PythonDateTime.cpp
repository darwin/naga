#include "PythonDateTime.h"
#include <datetime.h>

static bool initPythonDateTime();

[[maybe_unused]] static bool initialized = initPythonDateTime();

static bool initPythonDateTime() {
  PyDateTime_IMPORT;
  assert(PyDateTimeAPI);
  return PyDateTimeAPI;
}

bool isExactTime(py::object obj) {
  assert(PyDateTimeAPI);
  auto o = obj.ptr();
  return PyTime_CheckExact(o);
}

bool isExactDate(py::object obj) {
  assert(PyDateTimeAPI);
  auto o = obj.ptr();
  return PyDate_CheckExact(o);
}

bool isExactDateTime(py::object obj) {
  assert(PyDateTimeAPI);
  auto o = obj.ptr();
  return PyDateTime_CheckExact(o);
}

void getPythonDateTime(py::object obj, tm& ts, int& ms) {
  assert(PyDateTimeAPI);
  auto o = obj.ptr();
  ts.tm_year = PyDateTime_GET_YEAR(o) - 1900;
  ts.tm_mon = PyDateTime_GET_MONTH(o) - 1;
  ts.tm_mday = PyDateTime_GET_DAY(o);
  ts.tm_hour = PyDateTime_DATE_GET_HOUR(o);
  ts.tm_min = PyDateTime_DATE_GET_MINUTE(o);
  ts.tm_sec = PyDateTime_DATE_GET_SECOND(o);
  ts.tm_isdst = -1;

  ms = PyDateTime_DATE_GET_MICROSECOND(o);
}

void getPythonTime(py::object obj, tm& ts, int& ms) {
  assert(PyDateTimeAPI);
  auto o = obj.ptr();
  ts.tm_hour = PyDateTime_TIME_GET_HOUR(o) - 1;
  ts.tm_min = PyDateTime_TIME_GET_MINUTE(o);
  ts.tm_sec = PyDateTime_TIME_GET_SECOND(o);

  ms = PyDateTime_TIME_GET_MICROSECOND(o);
}

py::object pythonFromDateAndTime(int year, int month, int day, int hour, int minute, int second, int usecond) {
  assert(PyDateTimeAPI);
  auto o = PyDateTime_FromDateAndTime(year, month, day, hour, minute, second, usecond);
  return py::object(py::handle<>(o));
}
