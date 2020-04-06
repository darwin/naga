#include "JSObjectArray.h"
#include "PythonObject.h"
#include "PythonDateTime.h"
#include "JSObjectCLJS.h"
#include "Exception.h"

void CJSObjectArray::LazyConstructor() {
  if (!m_obj.IsEmpty()) {
    return;
  }

  auto v8_isolate = v8::Isolate::GetCurrent();
  auto v8_scope = v8u::getScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  v8::Local<v8::Array> v8_array;

  if (m_items.is_none()) {
    v8_array = v8::Array::New(v8_isolate, m_size);
  } else if (PyLong_CheckExact(m_items.ptr())) {
    m_size = PyLong_AsLong(m_items.ptr());
    v8_array = v8::Array::New(v8_isolate, m_size);
  } else if (PyList_Check(m_items.ptr())) {
    m_size = PyList_GET_SIZE(m_items.ptr());
    v8_array = v8::Array::New(v8_isolate, m_size);

    for (Py_ssize_t i = 0; i < (Py_ssize_t)m_size; i++) {
      auto raw_item = PyList_GET_ITEM(m_items.ptr(), i);
      auto py_item = pb::object(pb::reinterpret_borrow<pb::object>(raw_item));
      auto item = CPythonObject::Wrap(py_item);
      auto v8_i = v8::Uint32::New(v8_isolate, i);
      v8_array->Set(v8_context, v8_i, item).Check();
    }
  } else if (PyTuple_Check(m_items.ptr())) {
    m_size = PyTuple_GET_SIZE(m_items.ptr());
    v8_array = v8::Array::New(v8_isolate, m_size);

    for (Py_ssize_t i = 0; i < (Py_ssize_t)m_size; i++) {
      auto raw_item = PyTuple_GET_ITEM(m_items.ptr(), i);
      auto py_item = pb::object(pb::reinterpret_borrow<pb::object>(raw_item));
      auto item = CPythonObject::Wrap(py_item);
      auto v8_i = v8::Uint32::New(v8_isolate, i);
      v8_array->Set(v8_context, v8_i, item).Check();
    }
  } else if (PyGen_Check(m_items.ptr())) {
    v8_array = v8::Array::New(v8_isolate);
    pb::object py_iter(pb::reinterpret_steal<pb::object>(PyObject_GetIter(m_items.ptr())));

    m_size = 0;
    PyObject* raw_item;
    while (nullptr != (raw_item = PyIter_Next(py_iter.ptr()))) {
      auto py_item = pb::object(pb::reinterpret_borrow<pb::object>(raw_item));
      auto item = CPythonObject::Wrap(py_item);
      auto v8_i = v8::Uint32::New(v8_isolate, m_size++);
      v8_array->Set(v8_context, v8_i, item).Check();
    }
  }

  m_obj.Reset(v8_isolate, v8_array);
}

// void CJSObjectArray::LazyConstructor() {
//  if (!m_obj.IsEmpty())
//    return;
//
//  v8::Isolate* isolate = v8::Isolate::GetCurrent();
//  v8::HandleScope handle_scope(isolate);
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::Local<v8::Array> array;
//
//  if (m_items.is_none()) {
//    array = v8::Array::New(isolate, m_size);
//  } else if (PyLong_CheckExact(m_items.ptr())) {
//    m_size = PyLong_AsLong(m_items.ptr());
//    array = v8::Array::New(isolate, m_size);
//  } else if (PyList_Check(m_items.ptr())) {
//    m_size = PyList_GET_SIZE(m_items.ptr());
//    array = v8::Array::New(isolate, m_size);
//
//    for (Py_ssize_t i = 0; i < (Py_ssize_t)m_size; i++) {
//      auto py_obj = pb::object(pb::handle<>(pb::borrowed(PyList_GET_ITEM(m_items.ptr(), i))));
//      auto wrapped_obj = CPythonObject::Wrap(py_obj);
//      array->Set(context, v8::Uint32::New(isolate, i), wrapped_obj).Check();
//    }
//  } else if (PyTuple_Check(m_items.ptr())) {
//    m_size = PyTuple_GET_SIZE(m_items.ptr());
//    array = v8::Array::New(isolate, m_size);
//
//    for (Py_ssize_t i = 0; i < (Py_ssize_t)m_size; i++) {
//      auto py_obj = pb::object(pb::handle<>(pb::borrowed(PyTuple_GET_ITEM(m_items.ptr(), i))));
//      auto wrapped_obj = CPythonObject::Wrap(py_obj);
//      array->Set(context, v8::Uint32::New(isolate, i), wrapped_obj).Check();
//    }
//  } else if (PyGen_Check(m_items.ptr())) {
//    array = v8::Array::New(isolate);
//
//    pb::object iter(pb::handle<>(PyObject_GetIter(m_items.ptr())));
//
//    m_size = 0;
//    PyObject* item = NULL;
//
//    while (NULL != (item = PyIter_Next(iter.ptr()))) {
//      auto py_obj = pb::object(pb::handle<>(pb::borrowed(item)));
//      auto wrapped_obj = CPythonObject::Wrap(py_obj);
//      array->Set(context, v8::Uint32::New(isolate, m_size++), wrapped_obj).Check();
//    }
//  }
//
//  m_obj.Reset(isolate, array);
//}

// size_t CJSObjectArray::Length() {
//  LazyConstructor();
//
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
//
//  return v8::Local<v8::Array>::Cast(Object())->Length();
//}

size_t CJSObjectArray::Length() {
  LazyConstructor();

  auto v8_isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(v8_isolate);
  auto v8_scope = v8u::getScope(v8_isolate);
  auto result = Object().As<v8::Array>()->Length();
  return result;
}

pb::object CJSObjectArray::GetItem(pb::object py_key) {
  LazyConstructor();

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
      pb::list slice;

      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        auto v8_value = Object()->Get(v8_context, idx).ToLocalChecked();

        if (v8_value.IsEmpty()) {
          CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
        }

        slice.append(CJSObject::Wrap(v8_value, Object()));
      }

      return std::move(slice);
    }
  } else if (PyLong_Check(py_key.ptr())) {
    auto idx = PyLong_AsUnsignedLong(py_key.ptr());

    if (idx >= m_size) {
      throw CJavascriptException("index of of range", PyExc_IndexError);
    }

    if (!Object()->Has(v8_context, idx).ToChecked()) {
      return pb::none();
    }

    auto v8_value = Object()->Get(v8_context, idx).ToLocalChecked();

    if (v8_value.IsEmpty()) {
      CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
    }

    return CJSObject::Wrap(v8_value, Object());
  }

  throw CJavascriptException("list indices must be integers", PyExc_TypeError);
}

// pb::object CJSObjectArray::GetItem(pb::object key) {
//  LazyConstructor();
//
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  if (PySlice_Check(key.ptr())) {
//    Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
//    Py_ssize_t start, stop, step, sliceLen;
//
//    if (0 == PySlice_GetIndicesEx(key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
//      pb::list slice;
//
//      for (Py_ssize_t idx = start; idx < stop; idx += step) {
//        v8::Local<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate,
//        (uint32_t)idx)).ToLocalChecked();
//
//        if (value.IsEmpty())
//          CJavascriptException::ThrowIf(isolate, try_catch);
//
//        slice.append(CJSObject::Wrap(value, Object()));
//      }
//
//      return std::move(slice);
//    }
//  } else if (PyLong_Check(key.ptr())) {
//    uint32_t idx = PyLong_AsUnsignedLong(key.ptr());
//
//    if (!Object()->Has(context, idx).ToChecked())
//      return pb::object();
//
//    v8::Local<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, idx)).ToLocalChecked();
//
//    if (value.IsEmpty())
//      CJavascriptException::ThrowIf(isolate, try_catch);
//
//    return CJSObject::Wrap(value, Object());
//  }
//
//  throw CJavascriptException("list indices must be integers", PyExc_TypeError);
//}

pb::object CJSObjectArray::SetItem(pb::object py_key, pb::object py_value) {
  LazyConstructor();

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
          Object()->Set(v8_context, (uint32_t)(arrayLen + idx), v8::Null(v8_isolate)).Check();
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
          Object()->Set(v8_context, (uint32_t)idx, v8::Null(v8_isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
      }

      if (0 == PySlice_GetIndicesEx(py_key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
        if (itemSize != sliceLen) {
          if (itemSize < sliceLen) {
            Py_ssize_t diff = sliceLen - itemSize;

            for (Py_ssize_t idx = start + itemSize; idx < arrayLen - diff; idx++) {
              auto js_obj = Object()->Get(v8_context, (uint32_t)(idx + diff)).ToLocalChecked();
              Object()->Set(v8_context, idx, js_obj).Check();
            }
            for (Py_ssize_t idx = arrayLen - 1; idx > arrayLen - diff - 1; idx--) {
              Object()->Delete(v8_context, (uint32_t)idx).Check();
            }
          } else if (itemSize > sliceLen) {
            Py_ssize_t diff = itemSize - sliceLen;

            for (Py_ssize_t idx = arrayLen + diff - 1; idx > stop - 1; idx--) {
              auto js_obj = Object()->Get(v8_context, (uint32_t)(idx - diff)).ToLocalChecked();
              Object()->Set(v8_context, idx, js_obj).Check();
            }
          }
        }

        for (Py_ssize_t idx = 0; idx < itemSize; idx++) {
          pb::object py_item(pb::reinterpret_borrow<pb::object>(items[idx]));
          auto item = CPythonObject::Wrap(py_item);
          Object()->Set(v8_context, (uint32_t)(start + idx * step), item).Check();
        }
      }
    }
  } else if (PyLong_Check(py_key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(py_key.ptr());

    if (!Object()->Set(v8_context, v8::Integer::New(v8_isolate, idx), CPythonObject::Wrap(py_value)).ToChecked()) {
      CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
    }
  }

  return py_value;
}

// pb::object CJSObjectArray::SetItem(pb::object key, pb::object value) {
//  LazyConstructor();
//
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  if (PySlice_Check(key.ptr())) {
//    PyObject* values = PySequence_Fast(value.ptr(), "can only assign an iterable");
//
//    if (values) {
//      Py_ssize_t itemSize = PySequence_Fast_GET_SIZE(value.ptr());
//      PyObject** items = PySequence_Fast_ITEMS(value.ptr());
//
//      Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
//      Py_ssize_t start, stop, step, sliceLen;
//
//      PySlice_Unpack(key.ptr(), &start, &stop, &step);
//
//      /*
//       * If the slice start is greater than the array length we append null elements
//       * to the tail of the array to fill the gap
//       */
//      if (start > arrayLen) {
//        for (Py_ssize_t idx = arrayLen; idx < start; idx++) {
//          Object()->Set(context, (uint32_t)(arrayLen + idx), v8::Null(isolate)).Check();
//        }
//
//        arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
//      }
//
//      /*
//       * If the slice stop is greater than the array length (which was potentially
//       * modified by the previous check) we append null elements to the tail of the
//       * array. This step guarantees that the length of the array will always be
//       * greater or equal than stop
//       */
//      if (stop > arrayLen) {
//        for (Py_ssize_t idx = arrayLen; idx < stop; idx++) {
//          Object()->Set(context, (uint32_t)idx, v8::Null(isolate)).Check();
//        }
//
//        arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
//      }
//
//      if (0 == PySlice_GetIndicesEx(key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
//        if (itemSize != sliceLen) {
//          if (itemSize < sliceLen) {
//            Py_ssize_t diff = sliceLen - itemSize;
//
//            for (Py_ssize_t idx = start + itemSize; idx < arrayLen - diff; idx++) {
//              auto js_obj = Object()->Get(context, (uint32_t)(idx + diff)).ToLocalChecked();
//              Object()->Set(context, idx, js_obj).Check();
//            }
//            for (Py_ssize_t idx = arrayLen - 1; idx > arrayLen - diff - 1; idx--) {
//              Object()->Delete(context, (uint32_t)idx).Check();
//            }
//          } else if (itemSize > sliceLen) {
//            Py_ssize_t diff = itemSize - sliceLen;
//
//            for (Py_ssize_t idx = arrayLen + diff - 1; idx > stop - 1; idx--) {
//              auto js_obj = Object()->Get(context, (uint32_t)(idx - diff)).ToLocalChecked();
//              Object()->Set(context, idx, js_obj).Check();
//            }
//          }
//        }
//
//        for (Py_ssize_t idx = 0; idx < itemSize; idx++) {
//          auto py_obj = pb::object(pb::handle<>(pb::borrowed(items[idx])));
//          auto wrapped_obj = CPythonObject::Wrap(py_obj);
//          Object()->Set(context, (uint32_t)(start + idx * step), wrapped_obj).Check();
//        }
//      }
//    }
//  } else if (PyLong_Check(key.ptr())) {
//    uint32_t idx = PyLong_AsUnsignedLong(key.ptr());
//
//    if (!Object()->Set(context, v8::Integer::New(isolate, idx), CPythonObject::Wrap(value)).ToChecked())
//      CJavascriptException::ThrowIf(isolate, try_catch);
//  }
//
//  return value;
//}

pb::object CJSObjectArray::DelItem(pb::object py_key) {
  LazyConstructor();

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
        Object()->Delete(v8_context, (uint32_t)idx).Check();
      }
    }

    return pb::none();
  } else if (PyLong_Check(py_key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(py_key.ptr());

    pb::object py_result;

    if (Object()->Has(v8_context, idx).ToChecked()) {
      auto v8_idx = v8::Integer::New(v8_isolate, idx);
      auto v8_obj = Object()->Get(v8_context, v8_idx).ToLocalChecked();
      py_result = CJSObject::Wrap(v8_obj, Object());
    }

    if (!Object()->Delete(v8_context, idx).ToChecked()) {
      CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
    }

    if (py_result) {
      return py_result;
    } else {
      return pb::none();
    }
  }

  throw CJavascriptException("list indices must be integers", PyExc_TypeError);
}

// pb::object CJSObjectArray::DelItem(pb::object key) {
//  LazyConstructor();
//
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  if (PySlice_Check(key.ptr())) {
//    Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
//    Py_ssize_t start, stop, step, sliceLen;
//
//    if (0 == PySlice_GetIndicesEx(key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
//      for (Py_ssize_t idx = start; idx < stop; idx += step) {
//        Object()->Delete(context, (uint32_t)idx).Check();
//      }
//    }
//
//    return pb::object();
//  } else if (PyLong_Check(key.ptr())) {
//    uint32_t idx = PyLong_AsUnsignedLong(key.ptr());
//
//    pb::object value;
//
//    if (Object()->Has(context, idx).ToChecked())
//      value = CJSObject::Wrap(Object()->Get(context, v8::Integer::New(isolate, idx)).ToLocalChecked(), Object());
//
//    if (!Object()->Delete(context, idx).ToChecked())
//      CJavascriptException::ThrowIf(isolate, try_catch);
//
//    return value;
//  }
//
//  throw CJavascriptException("list indices must be integers", PyExc_TypeError);
//}

bool CJSObjectArray::Contains(pb::object py_key) {
  LazyConstructor();

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
        CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
      }

      // TODO: could this be optimized without wrapping?
      if (py_key.is(CJSObject::Wrap(v8_val, Object()))) {
        return true;
      }
    }
  }

  if (v8_try_catch.HasCaught()) {
    CJavascriptException::ThrowIf(v8_isolate, v8_try_catch);
  }

  return false;
}

// bool CJSObjectArray::Contains(pb::object item) {
//  LazyConstructor();
//
//  auto isolate = v8::Isolate::GetCurrent();
//  v8u::checkContext(isolate);
//  v8::HandleScope handle_scope(isolate);
//
//  v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//  v8::TryCatch try_catch(isolate);
//
//  for (size_t i = 0; i < Length(); i++) {
//    if (Object()->Has(context, i).ToChecked()) {
//      v8::Local<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, i)).ToLocalChecked();
//
//      if (try_catch.HasCaught())
//        CJavascriptException::ThrowIf(isolate, try_catch);
//
//      if (item == CJSObject::Wrap(value, Object())) {
//        return true;
//      }
//    }
//  }
//
//  if (try_catch.HasCaught())
//    CJavascriptException::ThrowIf(isolate, try_catch);
//
//  return false;
//}