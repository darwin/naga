#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectArray;

typedef std::shared_ptr<CJSObjectArray> CJSObjectArrayPtr;

class CJSObjectArray : public CJSObject {
  pb::object m_items;
  size_t m_size;

 public:
  class ArrayIterator
      //: public boost::iterator_facade<ArrayIterator, pb::object const, boost::forward_traversal_tag, pb::object>
          {
    CJSObjectArray* m_array;
    size_t m_idx;

   public:
    ArrayIterator(CJSObjectArray* array, size_t idx) : m_array(array), m_idx(idx) {}

    void increment() { m_idx++; }
    bool equal(ArrayIterator const& other) const { return m_array == other.m_array && m_idx == other.m_idx; }
    pb::object dereference() const { return m_array->GetItem(pb::int_(m_idx)); }
  };

  explicit CJSObjectArray(v8::Local<v8::Array> v8_array) : CJSObject(v8_array), m_size(v8_array->Length()) {}
  explicit CJSObjectArray(pb::object items) : m_items(items), m_size(0) {}

  size_t Length();

  pb::object GetItem(pb::object py_key);
  pb::object SetItem(pb::object py_key, pb::object py_value);
  pb::object DelItem(pb::object py_key);
  bool Contains(pb::object py_key);

  ArrayIterator begin() { return {this, 0}; }
  ArrayIterator end() { return {this, Length()}; }

  void LazyConstructor();
  void LazyInit() override { LazyConstructor(); }

};