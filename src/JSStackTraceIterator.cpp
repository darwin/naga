#include "JSStackTraceIterator.h"
#include "JSStackTrace.h"
#include "Logging.h"
#include "Printing.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSStackTraceLogger), __VA_ARGS__)

JSStackTraceIterator::JSStackTraceIterator(SharedJSStackTracePtr stack_trace_ptr)
    : m_shared_stack_trace_ptr(stack_trace_ptr),
      m_index(0) {
  TRACE("JSStackTraceIterator::JSStackTraceIterator {} stack_trace_ptr={}", THIS, stack_trace_ptr);
}

JSStackTraceIterator::~JSStackTraceIterator() {
  TRACE("JSStackTraceIterator::~JSStackTraceIterator {}", THIS);
}

SharedJSStackTraceIteratorPtr JSStackTraceIterator::Iter() {
  TRACE("JSStackTraceIterator::Iter {}", THIS);
  return shared_from_this();
}

SharedJSStackFramePtr JSStackTraceIterator::Next() {
  TRACE("JSStackTraceIterator::Next {} m_index={}", THIS, m_index);
  if (m_index >= m_shared_stack_trace_ptr->GetFrameCount()) {
    TRACE("=> STOP ITERATION");
    throw py::stop_iteration();
  }
  auto frame = m_shared_stack_trace_ptr->GetFrame(m_index);
  TRACE("=> {}", frame);
  m_index++;
  return frame;
}
