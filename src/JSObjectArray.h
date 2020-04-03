#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectArray : public CJSObject {
  py::object m_items;
  size_t m_size;

 public:
  class ArrayIterator
      : public boost::iterator_facade<ArrayIterator, py::object const, boost::forward_traversal_tag, py::object> {
    CJSObjectArray* m_array;
    size_t m_idx;

   public:
    ArrayIterator(CJSObjectArray* array, size_t idx) : m_array(array), m_idx(idx) {}

    void increment() { m_idx++; }

    bool equal(ArrayIterator const& other) const { return m_array == other.m_array && m_idx == other.m_idx; }

    reference dereference() const { return m_array->GetItem(py::long_(m_idx)); }
  };

  explicit CJSObjectArray(v8::Local<v8::Array> array) : CJSObject(array), m_size(array->Length()) {}

  explicit CJSObjectArray(py::object items) : m_items(std::move(items)), m_size(0) {}

  size_t Length();

  py::object GetItem(py::object key);
  py::object SetItem(py::object key, py::object value);
  py::object DelItem(py::object key);
  bool Contains(py::object item);

  ArrayIterator begin() { return {this, 0}; }
  ArrayIterator end() { return {this, Length()}; }

  void LazyConstructor();
};