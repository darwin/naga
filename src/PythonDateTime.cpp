#include "PythonDateTime.h"
#include <datetime.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

// TODO: revisit this and maybe use pybind chrono

static bool initPythonDateTime() noexcept;

[[maybe_unused]] static bool initialized = initPythonDateTime();

static bool initPythonDateTime() noexcept {
  try {
    PyDateTime_IMPORT;
    assert(PyDateTimeAPI);
    return PyDateTimeAPI;
  } catch (...) {
    std::cerr << "Fatal error: unable to initialize PyDateTimeAPI";
    exit(13);
  }
}

bool isExactTime(pb::handle py_obj) {
  assert(PyDateTimeAPI);
  auto raw_obj = py_obj.ptr();
  return PyTime_CheckExact(raw_obj);
}

bool isExactDate(pb::handle py_obj) {
  assert(PyDateTimeAPI);
  auto raw_obj = py_obj.ptr();
  return PyDate_CheckExact(raw_obj);
}

bool isExactDateTime(pb::handle py_obj) {
  assert(PyDateTimeAPI);
  auto raw_obj = py_obj.ptr();
  return PyDateTime_CheckExact(raw_obj);
}

void getPythonDateTime(pb::handle py_obj, tm& ts, int& ms) {
  assert(PyDateTimeAPI);
  auto raw_obj = py_obj.ptr();
  ts.tm_year = PyDateTime_GET_YEAR(raw_obj) - 1900;
  ts.tm_mon = PyDateTime_GET_MONTH(raw_obj) - 1;
  ts.tm_mday = PyDateTime_GET_DAY(raw_obj);
  ts.tm_hour = PyDateTime_DATE_GET_HOUR(raw_obj);
  ts.tm_min = PyDateTime_DATE_GET_MINUTE(raw_obj);
  ts.tm_sec = PyDateTime_DATE_GET_SECOND(raw_obj);
  ts.tm_isdst = -1;

  ms = PyDateTime_DATE_GET_MICROSECOND(raw_obj);
}

void getPythonTime(pb::handle py_obj, tm& ts, int& ms) {
  assert(PyDateTimeAPI);
  auto raw_obj = py_obj.ptr();
  ts.tm_hour = PyDateTime_TIME_GET_HOUR(raw_obj) - 1;
  ts.tm_min = PyDateTime_TIME_GET_MINUTE(raw_obj);
  ts.tm_sec = PyDateTime_TIME_GET_SECOND(raw_obj);

  ms = PyDateTime_TIME_GET_MICROSECOND(raw_obj);
}

pb::object pythonFromDateAndTime(int year, int month, int day, int hour, int minute, int second, int usecond) {
  assert(PyDateTimeAPI);
  auto raw_obj = PyDateTime_FromDateAndTime(year, month, day, hour, minute, second, usecond);
  return pb::reinterpret_steal<pb::object>(raw_obj);
}

#pragma clang diagnostic pop