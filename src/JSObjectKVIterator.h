#ifndef NAGA_JSObjectKVIterator_H_
#define NAGA_JSObjectKVIterator_H_

#include "Base.h"

class JSObjectKVIterator : public std::enable_shared_from_this<JSObjectKVIterator> {
  const SharedJSObjectPtr m_shared_object_ptr;
  v8::Global<v8::Array> m_v8_property_names;
  uint32_t m_index{0};
  uint32_t m_count{std::numeric_limits<decltype(m_count)>::max()};

 public:
  explicit JSObjectKVIterator(SharedJSObjectPtr shared_object_ptr);
  ~JSObjectKVIterator();

  bool Initialized() const { return m_count != std::numeric_limits<decltype(m_count)>::max(); }
  void Init();

  [[nodiscard]] SharedJSObjectKVIteratorPtr Iter();
  [[nodiscard]] py::object Next();
};

#endif