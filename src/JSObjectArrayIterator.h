#ifndef NAGA_JSOBJECTARRAYITERATOR_H_
#define NAGA_JSOBJECTARRAYITERATOR_H_

#include "Base.h"

class JSObjectArrayIterator : public std::enable_shared_from_this<JSObjectArrayIterator> {
  const SharedJSObjectPtr m_shared_object_ptr;
  uint32_t m_index{0};

 public:
  explicit JSObjectArrayIterator(SharedJSObjectPtr shared_object_ptr);
  ~JSObjectArrayIterator();

  [[nodiscard]] SharedJSObjectArrayIteratorPtr Iter();
  [[nodiscard]] py::object Next();
};

#endif