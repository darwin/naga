#include "JSObjectArrayImpl.h"
#include "JSException.h"
#include "Wrapping.h"
#include "JSNull.h"
#include "JSObject.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectArrayImplLogger), __VA_ARGS__)

size_t CJSObjectArrayImpl::Length() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto result = m_base.ToV8(v8_isolate).As<v8::Array>()->Length();
  TRACE("CJSObjectArrayImpl::Length {} => {}", THIS, result);
  return result;
}

py::object CJSObjectArrayImpl::GetItem(const py::object& py_key) const {
  TRACE("CJSObjectArrayImpl::GetItem {} py_key={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  // TODO: rewrite this using pybind
  if (PySlice_Check(py_key.ptr())) {
    Py_ssize_t arrayLen = m_base.ToV8(v8_isolate).As<v8::Array>()->Length();
    Py_ssize_t start;
    Py_ssize_t stop;
    Py_ssize_t step;
    Py_ssize_t sliceLen;

    if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      py::list slice;

      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        auto v8_value = m_base.ToV8(v8_isolate)->Get(v8_context, idx).ToLocalChecked();
        slice.append(wrap(v8_isolate, v8_value, m_base.ToV8(v8_isolate)));
      }

      return std::move(slice);
    }
  } else if (PyLong_Check(py_key.ptr())) {
    auto idx = PyLong_AsUnsignedLong(py_key.ptr());

    if (idx >= Length()) {
      throw CJSException("index of of range", PyExc_IndexError);
    }

    if (!m_base.ToV8(v8_isolate)->Has(v8_context, idx).ToChecked()) {
      return py::js_null();
    }

    auto v8_value = m_base.ToV8(v8_isolate)->Get(v8_context, idx).ToLocalChecked();
    return wrap(v8_isolate, v8_value, m_base.ToV8(v8_isolate));
  }

  throw CJSException("list indices must be integers", PyExc_TypeError);
}

py::object CJSObjectArrayImpl::SetItem(const py::object& py_key, const py::object& py_value) const {
  TRACE("CJSObjectArrayImpl::SetItem {} py_key={} py_value={}", THIS, py_key, py_value);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  if (PySlice_Check(py_key.ptr())) {
    PyObject* values = PySequence_Fast(py_value.ptr(), "can only assign an iterable");

    if (values) {
      Py_ssize_t itemSize = PySequence_Fast_GET_SIZE(py_value.ptr());
      PyObject** items = PySequence_Fast_ITEMS(py_value.ptr());

      Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(m_base.ToV8(v8_isolate))->Length();
      Py_ssize_t start, stop, step, sliceLen;

      PySlice_Unpack(py_key.ptr(), &start, &stop, &step);

      /*
       * If the slice start is greater than the array length we append null elements
       * to the tail of the array to fill the gap
       */
      if (start > arrayLen) {
        for (Py_ssize_t idx = arrayLen; idx < start; idx++) {
          m_base.ToV8(v8_isolate)->Set(v8_context, static_cast<uint32_t>(arrayLen + idx), v8::Null(v8_isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(m_base.ToV8(v8_isolate))->Length();
      }

      /*
       * If the slice stop is greater than the array length (which was potentially
       * modified by the previous check) we append null elements to the tail of the
       * array. This step guarantees that the length of the array will always be
       * greater or equal than stop
       */
      if (stop > arrayLen) {
        for (Py_ssize_t idx = arrayLen; idx < stop; idx++) {
          m_base.ToV8(v8_isolate)->Set(v8_context, static_cast<uint32_t>(idx), v8::Null(v8_isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(m_base.ToV8(v8_isolate))->Length();
      }

      if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
        if (itemSize != sliceLen) {
          if (itemSize < sliceLen) {
            Py_ssize_t diff = sliceLen - itemSize;

            for (Py_ssize_t idx = start + itemSize; idx < arrayLen - diff; idx++) {
              auto js_obj =
                  m_base.ToV8(v8_isolate)->Get(v8_context, static_cast<uint32_t>(idx + diff)).ToLocalChecked();
              m_base.ToV8(v8_isolate)->Set(v8_context, idx, js_obj).Check();
            }
            for (Py_ssize_t idx = arrayLen - 1; idx > arrayLen - diff - 1; idx--) {
              m_base.ToV8(v8_isolate)->Delete(v8_context, static_cast<uint32_t>(idx)).Check();
            }
          } else if (itemSize > sliceLen) {
            Py_ssize_t diff = itemSize - sliceLen;

            for (Py_ssize_t idx = arrayLen + diff - 1; idx > stop - 1; idx--) {
              auto js_obj =
                  m_base.ToV8(v8_isolate)->Get(v8_context, static_cast<uint32_t>(idx - diff)).ToLocalChecked();
              m_base.ToV8(v8_isolate)->Set(v8_context, idx, js_obj).Check();
            }
          }
        }

        for (Py_ssize_t idx = 0; idx < itemSize; idx++) {
          auto py_item(py::reinterpret_borrow<py::object>(items[idx]));
          auto item = wrap(py_item);
          m_base.ToV8(v8_isolate)->Set(v8_context, static_cast<uint32_t>(start + idx * step), item).Check();
        }
      }
    }
  } else if (PyLong_Check(py_key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(py_key.ptr());

    m_base.ToV8(v8_isolate)->Set(v8_context, v8::Integer::New(v8_isolate, idx), wrap(py_value)).Check();
  }

  return py_value;
}

py::object CJSObjectArrayImpl::DelItem(const py::object& py_key) const {
  TRACE("CJSObjectArrayImpl::DelItem {} py_key={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  if (PySlice_Check(py_key.ptr())) {
    Py_ssize_t arrayLen = m_base.ToV8(v8_isolate).As<v8::Array>()->Length();
    Py_ssize_t start;
    Py_ssize_t stop;
    Py_ssize_t step;
    Py_ssize_t sliceLen;

    if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        m_base.ToV8(v8_isolate)->Delete(v8_context, static_cast<uint32_t>(idx)).Check();
      }
    }

    return py::none();
  } else if (PyLong_Check(py_key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(py_key.ptr());

    py::object py_result;  // = py::none/py::js_null by default

    if (m_base.ToV8(v8_isolate)->Has(v8_context, idx).ToChecked()) {
      auto v8_idx = v8::Integer::New(v8_isolate, idx);
      auto v8_obj = m_base.ToV8(v8_isolate)->Get(v8_context, v8_idx).ToLocalChecked();
      py_result = wrap(v8_isolate, v8_obj, m_base.ToV8(v8_isolate));
    }

    m_base.ToV8(v8_isolate)->Delete(v8_context, idx).Check();
    return py_result;
  }

  throw CJSException("list indices must be integers", PyExc_TypeError);
}

bool CJSObjectArrayImpl::Contains(const py::object& py_key) const {
  TRACE("CJSObjectArrayImpl::Contains {} py_key={}", THIS, py_key);
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withAutoTryCatch(v8_isolate);

  for (size_t i = 0; i < Length(); i++) {
    if (m_base.ToV8(v8_isolate)->Has(v8_context, i).ToChecked()) {
      auto v8_i = v8::Integer::New(v8_isolate, i);
      auto v8_val = m_base.ToV8(v8_isolate)->Get(v8_context, v8_i).ToLocalChecked();

      // TODO: could this be optimized without wrapping?
      if (py_key.is(wrap(v8_isolate, v8_val, m_base.ToV8(v8_isolate)))) {
        return true;
      }
    }
  }

  return false;
}