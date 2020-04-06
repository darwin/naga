#include "JSObjectArray.h"
#include "PythonObject.h"
#include "JSObjectCLJS.h"
#include "JSException.h"

// TODO : remove this after we get rid of python macros below
#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

void CJSObjectArray::LazyInit() {
  if (!m_v8_obj.IsEmpty()) {
    return;
  }

  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  v8::Local<v8::Array> v8_array;

  if (m_py_items.is_none()) {
    v8_array = v8::Array::New(v8_isolate, m_size);
  } else if (PyLong_CheckExact(m_py_items.ptr())) {
    m_size = PyLong_AsLong(m_py_items.ptr());
    v8_array = v8::Array::New(v8_isolate, m_size);
  } else if (PyList_Check(m_py_items.ptr())) {
    m_size = PyList_GET_SIZE(m_py_items.ptr());
    v8_array = v8::Array::New(v8_isolate, m_size);

    for (Py_ssize_t i = 0; i < static_cast<Py_ssize_t>(m_size); i++) {
      auto raw_item = PyList_GET_ITEM(m_py_items.ptr(), i);
      auto py_item = py::object(py::reinterpret_borrow<py::object>(raw_item));
      auto item = CPythonObject::Wrap(py_item);
      auto v8_i = v8::Uint32::New(v8_isolate, i);
      v8_array->Set(v8_context, v8_i, item).Check();
    }
  } else if (PyTuple_Check(m_py_items.ptr())) {
    m_size = PyTuple_GET_SIZE(m_py_items.ptr());
    v8_array = v8::Array::New(v8_isolate, m_size);

    for (Py_ssize_t i = 0; i < static_cast<Py_ssize_t>(m_size); i++) {
      auto raw_item = PyTuple_GET_ITEM(m_py_items.ptr(), i);
      auto py_item = py::object(py::reinterpret_borrow<py::object>(raw_item));
      auto item = CPythonObject::Wrap(py_item);
      auto v8_i = v8::Uint32::New(v8_isolate, i);
      v8_array->Set(v8_context, v8_i, item).Check();
    }
  } else if (PyGen_Check(m_py_items.ptr())) {
    v8_array = v8::Array::New(v8_isolate);
    auto py_iter(py::reinterpret_steal<py::object>(PyObject_GetIter(m_py_items.ptr())));

    m_size = 0;
    PyObject* raw_item;
    while (nullptr != (raw_item = PyIter_Next(py_iter.ptr()))) {
      auto py_item = py::object(py::reinterpret_borrow<py::object>(raw_item));
      auto item = CPythonObject::Wrap(py_item);
      auto v8_i = v8::Uint32::New(v8_isolate, m_size++);
      v8_array->Set(v8_context, v8_i, item).Check();
    }
  }

  m_v8_obj.Reset(v8_isolate, v8_array);
}

size_t CJSObjectArray::Length() {
  LazyInit();

  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto result = Object().As<v8::Array>()->Length();
  return result;
}

py::object CJSObjectArray::GetItem(py::object py_key) {
  LazyInit();

  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  // TODO: rewrite this using pybind
  if (PySlice_Check(py_key.ptr())) {
    Py_ssize_t arrayLen = Object().As<v8::Array>()->Length();
    Py_ssize_t start;
    Py_ssize_t stop;
    Py_ssize_t step;
    Py_ssize_t sliceLen;

    if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      py::list slice;

      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        auto v8_value = Object()->Get(v8_context, idx).ToLocalChecked();

        if (v8_value.IsEmpty()) {
          CJSException::ThrowIf(v8_isolate, v8_try_catch);
        }

        slice.append(CJSObject::Wrap(v8_value, Object()));
      }

      return std::move(slice);
    }
  } else if (PyLong_Check(py_key.ptr())) {
    auto idx = PyLong_AsUnsignedLong(py_key.ptr());

    if (idx >= m_size) {
      throw CJSException("index of of range", PyExc_IndexError);
    }

    if (!Object()->Has(v8_context, idx).ToChecked()) {
      return py::none();
    }

    auto v8_value = Object()->Get(v8_context, idx).ToLocalChecked();

    if (v8_value.IsEmpty()) {
      CJSException::ThrowIf(v8_isolate, v8_try_catch);
    }

    return CJSObject::Wrap(v8_value, Object());
  }

  throw CJSException("list indices must be integers", PyExc_TypeError);
}

py::object CJSObjectArray::SetItem(py::object py_key, py::object py_value) {
  LazyInit();

  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  if (PySlice_Check(py_key.ptr())) {
    PyObject* values = PySequence_Fast(py_value.ptr(), "can only assign an iterable");

    if (values) {
      Py_ssize_t itemSize = PySequence_Fast_GET_SIZE(py_value.ptr());
      PyObject** items = PySequence_Fast_ITEMS(py_value.ptr());

      Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
      Py_ssize_t start, stop, step, sliceLen;

      PySlice_Unpack(py_key.ptr(), &start, &stop, &step);

      /*
       * If the slice start is greater than the array length we append null elements
       * to the tail of the array to fill the gap
       */
      if (start > arrayLen) {
        for (Py_ssize_t idx = arrayLen; idx < start; idx++) {
          Object()->Set(v8_context, static_cast<uint32_t>(arrayLen + idx), v8::Null(v8_isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
      }

      /*
       * If the slice stop is greater than the array length (which was potentially
       * modified by the previous check) we append null elements to the tail of the
       * array. This step guarantees that the length of the array will always be
       * greater or equal than stop
       */
      if (stop > arrayLen) {
        for (Py_ssize_t idx = arrayLen; idx < stop; idx++) {
          Object()->Set(v8_context, static_cast<uint32_t>(idx), v8::Null(v8_isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
      }

      if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
        if (itemSize != sliceLen) {
          if (itemSize < sliceLen) {
            Py_ssize_t diff = sliceLen - itemSize;

            for (Py_ssize_t idx = start + itemSize; idx < arrayLen - diff; idx++) {
              auto js_obj = Object()->Get(v8_context, static_cast<uint32_t>(idx + diff)).ToLocalChecked();
              Object()->Set(v8_context, idx, js_obj).Check();
            }
            for (Py_ssize_t idx = arrayLen - 1; idx > arrayLen - diff - 1; idx--) {
              Object()->Delete(v8_context, static_cast<uint32_t>(idx)).Check();
            }
          } else if (itemSize > sliceLen) {
            Py_ssize_t diff = itemSize - sliceLen;

            for (Py_ssize_t idx = arrayLen + diff - 1; idx > stop - 1; idx--) {
              auto js_obj = Object()->Get(v8_context, static_cast<uint32_t>(idx - diff)).ToLocalChecked();
              Object()->Set(v8_context, idx, js_obj).Check();
            }
          }
        }

        for (Py_ssize_t idx = 0; idx < itemSize; idx++) {
          auto py_item(py::reinterpret_borrow<py::object>(items[idx]));
          auto item = CPythonObject::Wrap(py_item);
          Object()->Set(v8_context, static_cast<uint32_t>(start + idx * step), item).Check();
        }
      }
    }
  } else if (PyLong_Check(py_key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(py_key.ptr());

    if (!Object()->Set(v8_context, v8::Integer::New(v8_isolate, idx), CPythonObject::Wrap(py_value)).ToChecked()) {
      CJSException::ThrowIf(v8_isolate, v8_try_catch);
    }
  }

  return py_value;
}

py::object CJSObjectArray::DelItem(py::object py_key) {
  LazyInit();

  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  if (PySlice_Check(py_key.ptr())) {
    Py_ssize_t arrayLen = Object().As<v8::Array>()->Length();
    Py_ssize_t start;
    Py_ssize_t stop;
    Py_ssize_t step;
    Py_ssize_t sliceLen;

    if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        Object()->Delete(v8_context, static_cast<uint32_t>(idx)).Check();
      }
    }

    return py::none();
  } else if (PyLong_Check(py_key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(py_key.ptr());

    py::object py_result;

    if (Object()->Has(v8_context, idx).ToChecked()) {
      auto v8_idx = v8::Integer::New(v8_isolate, idx);
      auto v8_obj = Object()->Get(v8_context, v8_idx).ToLocalChecked();
      py_result = CJSObject::Wrap(v8_obj, Object());
    }

    if (!Object()->Delete(v8_context, idx).ToChecked()) {
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

bool CJSObjectArray::Contains(const py::object& py_key) {
  LazyInit();

  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();
  auto v8_try_catch = v8u::openTryCatch(v8_isolate);

  for (size_t i = 0; i < Length(); i++) {
    if (Object()->Has(v8_context, i).ToChecked()) {
      auto v8_i = v8::Integer::New(v8_isolate, i);
      auto v8_val = Object()->Get(v8_context, v8_i).ToLocalChecked();

      if (v8_try_catch.HasCaught()) {
        CJSException::ThrowIf(v8_isolate, v8_try_catch);
      }

      // TODO: could this be optimized without wrapping?
      if (py_key.is(CJSObject::Wrap(v8_val, Object()))) {
        return true;
      }
    }
  }

  if (v8_try_catch.HasCaught()) {
    CJSException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return false;
}

#pragma clang diagnostic pop