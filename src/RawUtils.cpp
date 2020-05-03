#include "RawUtils.h"
#include "V8Utils.h"

const char* pythonTypeName(PyTypeObject* raw_type) {
  return raw_type->tp_name;
}
