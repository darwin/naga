#ifndef NAGA_JSSTACKTRACEITERATOR_H_
#define NAGA_JSSTACKTRACEITERATOR_H_

#include "Base.h"

class JSStackTraceIterator : public std::enable_shared_from_this<JSStackTraceIterator> {
  const SharedJSStackTracePtr m_shared_stack_trace_ptr;
  int m_index;

 public:
  explicit JSStackTraceIterator(SharedJSStackTracePtr stack_trace_ptr);
  ~JSStackTraceIterator();

  [[nodiscard]] SharedJSStackTraceIteratorPtr Iter();
  [[nodiscard]] SharedJSStackFramePtr Next();
};

#endif