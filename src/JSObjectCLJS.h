#pragma once

#include "Base.h"
#include "JSObject.h"

void exposeCLJSTypes();
bool isCLJSType(v8::Local<v8::Object> obj);

const uint32_t kWrapperHintNone = 0;
const uint32_t kWrapperHintCCLJSIIterableIterator = 1;

void setWrapperHint(v8::Local<v8::Object> obj, uint32_t hint);
uint32_t getWrapperHint(v8::Local<v8::Object> obj);

// this is a generic wrapper for all CLJS types
// see https://docs.python.org/3/reference/datamodel.html?highlight=__iter__#emulating-container-types
// see https://www.boost.org/doc/libs/1_63_0/libs/python/doc/html/reference/topics/indexing_support.html
class CJSObjectCLJS : public CJSObject {
 public:
  explicit CJSObjectCLJS(v8::Local<v8::Object> o) : CJSObject(o) {}

  size_t Length();
  py::object Repr();
  py::object Str();
  py::object GetItem(const py::object& py_key);
  py::object GetAttr(const py::object& py_key);
  py::object Iter();
  //  bool Contains(py::object item);

 private:
  py::object GetItemSlice(const py::object& py_slice);
  py::object GetItemIndex(const py::object& py_index);
  py::object GetItemString(const py::object& py_string);
};

class CCLJSIIterableIterator : public CJSObject {
 public:
  explicit CCLJSIIterableIterator(v8::Local<v8::Object> o) : CJSObject(o) {}

  py::object Next();
  py::object Iter(const py::object& py_iter);
};
