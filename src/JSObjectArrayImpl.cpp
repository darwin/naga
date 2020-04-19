#include "JSObjectArrayImpl.h"
#include "PythonObject.h"
#include "JSException.h"

size_t CJSObjectArrayImpl::ArrayLength() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto result = m_base.Object().As<v8::Array>()->Length();
  return result;
}

py::object CJSObjectArrayImpl::ArrayGetItem(py::object py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  // TODO: rewrite this using pybind
  if (PySlice_Check(py_key.ptr())) {
    Py_ssize_t arrayLen = m_base.Object().As<v8::Array>()->Length();
    Py_ssize_t start;
    Py_ssize_t stop;
    Py_ssize_t step;
    Py_ssize_t sliceLen;

    if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      py::list slice;

      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        auto v8_value = m_base.Object()->Get(v8_context, idx).ToLocalChecked();

        if (v8_value.IsEmpty()) {
          CJSException::ThrowIf(v8_isolate, v8_try_catch);
        }

        slice.append(CJSObject::Wrap(v8_isolate, v8_value, m_base.Object()));
      }

      return std::move(slice);
    }
  } else if (PyLong_Check(py_key.ptr())) {
    auto idx = PyLong_AsUnsignedLong(py_key.ptr());

    if (idx >= ArrayLength()) {
      throw CJSException("index of of range", PyExc_IndexError);
    }

    if (!m_base.Object()->Has(v8_context, idx).ToChecked()) {
      return py::none();
    }

    auto v8_value = m_base.Object()->Get(v8_context, idx).ToLocalChecked();

    if (v8_value.IsEmpty()) {
      CJSException::ThrowIf(v8_isolate, v8_try_catch);
    }

    return CJSObject::Wrap(v8_isolate, v8_value, m_base.Object());
  }

  throw CJSException("list indices must be integers", PyExc_TypeError);
}

py::object CJSObjectArrayImpl::ArraySetItem(py::object py_key, py::object py_value) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  if (PySlice_Check(py_key.ptr())) {
    PyObject* values = PySequence_Fast(py_value.ptr(), "can only assign an iterable");

    if (values) {
      Py_ssize_t itemSize = PySequence_Fast_GET_SIZE(py_value.ptr());
      PyObject** items = PySequence_Fast_ITEMS(py_value.ptr());

      Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(m_base.Object())->Length();
      Py_ssize_t start, stop, step, sliceLen;

      PySlice_Unpack(py_key.ptr(), &start, &stop, &step);

      /*
       * If the slice start is greater than the array length we append null elements
       * to the tail of the array to fill the gap
       */
      if (start > arrayLen) {
        for (Py_ssize_t idx = arrayLen; idx < start; idx++) {
          m_base.Object()->Set(v8_context, static_cast<uint32_t>(arrayLen + idx), v8::Null(v8_isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(m_base.Object())->Length();
      }

      /*
       * If the slice stop is greater than the array length (which was potentially
       * modified by the previous check) we append null elements to the tail of the
       * array. This step guarantees that the length of the array will always be
       * greater or equal than stop
       */
      if (stop > arrayLen) {
        for (Py_ssize_t idx = arrayLen; idx < stop; idx++) {
          m_base.Object()->Set(v8_context, static_cast<uint32_t>(idx), v8::Null(v8_isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(m_base.Object())->Length();
      }

      if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
        if (itemSize != sliceLen) {
          if (itemSize < sliceLen) {
            Py_ssize_t diff = sliceLen - itemSize;

            for (Py_ssize_t idx = start + itemSize; idx < arrayLen - diff; idx++) {
              auto js_obj = m_base.Object()->Get(v8_context, static_cast<uint32_t>(idx + diff)).ToLocalChecked();
              m_base.Object()->Set(v8_context, idx, js_obj).Check();
            }
            for (Py_ssize_t idx = arrayLen - 1; idx > arrayLen - diff - 1; idx--) {
              m_base.Object()->Delete(v8_context, static_cast<uint32_t>(idx)).Check();
            }
          } else if (itemSize > sliceLen) {
            Py_ssize_t diff = itemSize - sliceLen;

            for (Py_ssize_t idx = arrayLen + diff - 1; idx > stop - 1; idx--) {
              auto js_obj = m_base.Object()->Get(v8_context, static_cast<uint32_t>(idx - diff)).ToLocalChecked();
              m_base.Object()->Set(v8_context, idx, js_obj).Check();
            }
          }
        }

        for (Py_ssize_t idx = 0; idx < itemSize; idx++) {
          auto py_item(py::reinterpret_borrow<py::object>(items[idx]));
          auto item = CPythonObject::Wrap(py_item);
          m_base.Object()->Set(v8_context, static_cast<uint32_t>(start + idx * step), item).Check();
        }
      }
    }
  } else if (PyLong_Check(py_key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(py_key.ptr());

    if (!m_base.Object()
             ->Set(v8_context, v8::Integer::New(v8_isolate, idx), CPythonObject::Wrap(py_value))
             .ToChecked()) {
      CJSException::ThrowIf(v8_isolate, v8_try_catch);
    }
  }

  return py_value;
}

py::object CJSObjectArrayImpl::ArrayDelItem(py::object py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  if (PySlice_Check(py_key.ptr())) {
    Py_ssize_t arrayLen = m_base.Object().As<v8::Array>()->Length();
    Py_ssize_t start;
    Py_ssize_t stop;
    Py_ssize_t step;
    Py_ssize_t sliceLen;

    if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        m_base.Object()->Delete(v8_context, static_cast<uint32_t>(idx)).Check();
      }
    }

    return py::none();
  } else if (PyLong_Check(py_key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(py_key.ptr());

    py::object py_result;

    if (m_base.Object()->Has(v8_context, idx).ToChecked()) {
      auto v8_idx = v8::Integer::New(v8_isolate, idx);
      auto v8_obj = m_base.Object()->Get(v8_context, v8_idx).ToLocalChecked();
      py_result = CJSObject::Wrap(v8_isolate, v8_obj, m_base.Object());
    }

    if (!m_base.Object()->Delete(v8_context, idx).ToChecked()) {
      CJSException::ThrowIf(v8_isolate, v8_try_catch);
    }

    if (py_result) {
      return py_result;
    } else {
      return py::none();
    }
  }

  throw CJSException("list indices must be integers", PyExc_TypeError);
}

bool CJSObjectArrayImpl::ArrayContains(const py::object& py_key) const {
  auto v8_isolate = v8u::getCurrentIsolate();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::withScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::withTryCatch(v8_isolate);

  for (size_t i = 0; i < ArrayLength(); i++) {
    if (m_base.Object()->Has(v8_context, i).ToChecked()) {
      auto v8_i = v8::Integer::New(v8_isolate, i);
      auto v8_val = m_base.Object()->Get(v8_context, v8_i).ToLocalChecked();

      if (v8_try_catch.HasCaught()) {
        CJSException::ThrowIf(v8_isolate, v8_try_catch);
      }

      // TODO: could this be optimized without wrapping?
      if (py_key.is(CJSObject::Wrap(v8_isolate, v8_val, m_base.Object()))) {
        return true;
      }
    }
  }

  if (v8_try_catch.HasCaught()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return false;
}