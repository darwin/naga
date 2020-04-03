#include "JSObjectArray.h"
#include "PythonObject.h"
#include "PythonDateTime.h"
#include "JSObjectCLJS.h"
#include "Exception.h"

void CJSObjectArray::LazyConstructor(void) {
  if (!m_obj.IsEmpty())
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Array> array;

  if (m_items.is_none()) {
    array = v8::Array::New(isolate, m_size);
  } else if (PyLong_CheckExact(m_items.ptr())) {
    m_size = PyLong_AsLong(m_items.ptr());
    array = v8::Array::New(isolate, m_size);
  } else if (PyList_Check(m_items.ptr())) {
    m_size = PyList_GET_SIZE(m_items.ptr());
    array = v8::Array::New(isolate, m_size);

    for (Py_ssize_t i = 0; i < (Py_ssize_t)m_size; i++) {
      auto py_obj = py::object(py::handle<>(py::borrowed(PyList_GET_ITEM(m_items.ptr(), i))));
      auto wrapped_obj = CPythonObject::Wrap(py_obj);
      array->Set(context, v8::Uint32::New(isolate, i), wrapped_obj).Check();
    }
  } else if (PyTuple_Check(m_items.ptr())) {
    m_size = PyTuple_GET_SIZE(m_items.ptr());
    array = v8::Array::New(isolate, m_size);

    for (Py_ssize_t i = 0; i < (Py_ssize_t)m_size; i++) {
      auto py_obj = py::object(py::handle<>(py::borrowed(PyTuple_GET_ITEM(m_items.ptr(), i))));
      auto wrapped_obj = CPythonObject::Wrap(py_obj);
      array->Set(context, v8::Uint32::New(isolate, i), wrapped_obj).Check();
    }
  } else if (PyGen_Check(m_items.ptr())) {
    array = v8::Array::New(isolate);

    py::object iter(py::handle<>(::PyObject_GetIter(m_items.ptr())));

    m_size = 0;
    PyObject* item = NULL;

    while (NULL != (item = ::PyIter_Next(iter.ptr()))) {
      auto py_obj = py::object(py::handle<>(py::borrowed(item)));
      auto wrapped_obj = CPythonObject::Wrap(py_obj);
      array->Set(context, v8::Uint32::New(isolate, m_size++), wrapped_obj).Check();
    }
  }

  m_obj.Reset(isolate, array);
}

size_t CJSObjectArray::Length(void) {
  LazyConstructor();

  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  return v8::Local<v8::Array>::Cast(Object())->Length();
}

py::object CJSObjectArray::GetItem(py::object key) {
  LazyConstructor();

  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  if (PySlice_Check(key.ptr())) {
    Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
    Py_ssize_t start, stop, step, sliceLen;

    if (0 == PySlice_GetIndicesEx(key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      py::list slice;

      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        v8::Local<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, (uint32_t)idx)).ToLocalChecked();

        if (value.IsEmpty())
          CJavascriptException::ThrowIf(isolate, try_catch);

        slice.append(CJSObject::Wrap(value, Object()));
      }

      return std::move(slice);
    }
  } else if (PyLong_Check(key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(key.ptr());

    if (!Object()->Has(context, idx).ToChecked())
      return py::object();

    v8::Local<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, idx)).ToLocalChecked();

    if (value.IsEmpty())
      CJavascriptException::ThrowIf(isolate, try_catch);

    return CJSObject::Wrap(value, Object());
  }

  throw CJavascriptException("list indices must be integers", ::PyExc_TypeError);
}

py::object CJSObjectArray::SetItem(py::object key, py::object value) {
  LazyConstructor();

  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  if (PySlice_Check(key.ptr())) {
    PyObject* values = ::PySequence_Fast(value.ptr(), "can only assign an iterable");

    if (values) {
      Py_ssize_t itemSize = PySequence_Fast_GET_SIZE(value.ptr());
      PyObject** items = PySequence_Fast_ITEMS(value.ptr());

      Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
      Py_ssize_t start, stop, step, sliceLen;

      PySlice_Unpack(key.ptr(), &start, &stop, &step);

      /*
       * If the slice start is greater than the array length we append null elements
       * to the tail of the array to fill the gap
       */
      if (start > arrayLen) {
        for (Py_ssize_t idx = arrayLen; idx < start; idx++) {
          Object()->Set(context, (uint32_t)(arrayLen + idx), v8::Null(isolate)).Check();
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
          Object()->Set(context, (uint32_t)idx, v8::Null(isolate)).Check();
        }

        arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
      }

      if (0 == PySlice_GetIndicesEx(key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
        if (itemSize != sliceLen) {
          if (itemSize < sliceLen) {
            Py_ssize_t diff = sliceLen - itemSize;

            for (Py_ssize_t idx = start + itemSize; idx < arrayLen - diff; idx++) {
              auto js_obj = Object()->Get(context, (uint32_t)(idx + diff)).ToLocalChecked();
              Object()->Set(context, idx, js_obj).Check();
            }
            for (Py_ssize_t idx = arrayLen - 1; idx > arrayLen - diff - 1; idx--) {
              Object()->Delete(context, (uint32_t)idx).Check();
            }
          } else if (itemSize > sliceLen) {
            Py_ssize_t diff = itemSize - sliceLen;

            for (Py_ssize_t idx = arrayLen + diff - 1; idx > stop - 1; idx--) {
              auto js_obj = Object()->Get(context, (uint32_t)(idx - diff)).ToLocalChecked();
              Object()->Set(context, idx, js_obj).Check();
            }
          }
        }

        for (Py_ssize_t idx = 0; idx < itemSize; idx++) {
          auto py_obj = py::object(py::handle<>(py::borrowed(items[idx])));
          auto wrapped_obj = CPythonObject::Wrap(py_obj);
          Object()->Set(context, (uint32_t)(start + idx * step), wrapped_obj).Check();
        }
      }
    }
  } else if (PyLong_Check(key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(key.ptr());

    if (!Object()->Set(context, v8::Integer::New(isolate, idx), CPythonObject::Wrap(value)).ToChecked())
      CJavascriptException::ThrowIf(isolate, try_catch);
  }

  return value;
}

py::object CJSObjectArray::DelItem(py::object key) {
  LazyConstructor();

  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  if (PySlice_Check(key.ptr())) {
    Py_ssize_t arrayLen = v8::Local<v8::Array>::Cast(Object())->Length();
    Py_ssize_t start, stop, step, sliceLen;

    if (0 == PySlice_GetIndicesEx(key.ptr(), arrayLen, &start, &stop, &step, &sliceLen)) {
      for (Py_ssize_t idx = start; idx < stop; idx += step) {
        Object()->Delete(context, (uint32_t)idx).Check();
      }
    }

    return py::object();
  } else if (PyLong_Check(key.ptr())) {
    uint32_t idx = PyLong_AsUnsignedLong(key.ptr());

    py::object value;

    if (Object()->Has(context, idx).ToChecked())
      value = CJSObject::Wrap(Object()->Get(context, v8::Integer::New(isolate, idx)).ToLocalChecked(), Object());

    if (!Object()->Delete(context, idx).ToChecked())
      CJavascriptException::ThrowIf(isolate, try_catch);

    return value;
  }

  throw CJavascriptException("list indices must be integers", ::PyExc_TypeError);
}

bool CJSObjectArray::Contains(py::object item) {
  LazyConstructor();

  auto isolate = v8::Isolate::GetCurrent();
  v8u::checkContext(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::TryCatch try_catch(isolate);

  for (size_t i = 0; i < Length(); i++) {
    if (Object()->Has(context, i).ToChecked()) {
      v8::Local<v8::Value> value = Object()->Get(context, v8::Integer::New(isolate, i)).ToLocalChecked();

      if (try_catch.HasCaught())
        CJavascriptException::ThrowIf(isolate, try_catch);

      if (item == CJSObject::Wrap(value, Object())) {
        return true;
      }
    }
  }

  if (try_catch.HasCaught())
    CJavascriptException::ThrowIf(isolate, try_catch);

  return false;
}