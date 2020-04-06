#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectArray : public CJSObject {
  pb::object m_py_items;
  size_t m_size;

 public:
  class ArrayIterator {
    CJSObjectArray* m_array_ptr;
    size_t m_idx;

   public:
    ArrayIterator(CJSObjectArray* array, size_t idx) : m_array_ptr(array), m_idx(idx) {}

    void increment() { m_idx++; }
    [[nodiscard]] bool equal(ArrayIterator const& other) const {
      return m_array_ptr == other.m_array_ptr && m_idx == other.m_idx;
    }
    [[nodiscard]] pb::object dereference() const { return m_array_ptr->GetItem(pb::int_(m_idx)); }
  };

  explicit CJSObjectArray(v8::Local<v8::Array> v8_array) : CJSObject(v8_array), m_size(v8_array->Length()) {}
  explicit CJSObjectArray(pb::object items) : m_py_items(std::move(items)), m_size(0) {}

  size_t Length();

  pb::object GetItem(pb::object py_key);
  pb::object SetItem(pb::object py_key, pb::object py_value);
  pb::object DelItem(pb::object py_key);
  bool Contains(const pb::object& py_key);

  ArrayIterator begin() { return {this, 0}; }
  ArrayIterator end() { return {this, Length()}; }

  void LazyInit() override;
};