#include "JSObjectArrayImpl.h"
#include "JSException.h"
#include "Wrapping.h"
#include "JSObject.h"
#include "Logging.h"
#include "V8XUtils.h"
#include "Printing.h"
#include "PybindExtensions.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSObjectArrayImplLogger), __VA_ARGS__)

py::ssize_t JSObjectArrayImpl::Length() const {
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_this_array = m_base.ToV8(v8_isolate).As<v8::Array>();
  auto result = v8_this_array->Length();
  TRACE("JSObjectArrayImpl::Length {} => {}", THIS, result);
  return result;
}

py::object JSObjectArrayImpl::GetItem(const py::object& py_key) const {
  TRACE("JSObjectArrayImpl::GetItem {} py_key={}", THIS, py_key);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  auto v8_this_array = m_base.ToV8(v8_isolate).As<v8::Array>();

  if (py::isinstance<py::slice>(py_key)) {
    auto py_slice = py::cast<py::slice>(py_key);
    py::ssize_t array_length = v8_this_array->Length();
    py::ssize_t slice_start;
    py::ssize_t slice_stop;
    py::ssize_t slice_step;
    py::ssize_t slice_length;

    if (!py_slice.compute(array_length, &slice_start, &slice_stop, &slice_step, &slice_length)) {
      assert(PyErr_Occurred());
      return py::none();
    }

    // TODO: optimize this
    py::list py_result;
    for (py::ssize_t i = slice_start; i < slice_stop; i += slice_step) {
      uint32_t index = i;
      auto v8_value = v8_this_array->Get(v8_context, index).ToLocalChecked();
      auto py_value = wrap(v8_isolate, v8_value, v8_this_array);
      py_result.append(py_value);
    }
    return std::move(py_result);
  } else if (py::isinstance<py::int_>(py_key)) {
    auto py_int = py::cast<py::int_>(py_key);
    uint32_t index = py_int;

    if (index >= Length()) {
      throw JSException("index of of range", PyExc_IndexError);
    }

    if (!v8_this_array->Has(v8_context, index).ToChecked()) {
      return py::none();
    }

    auto v8_value = v8_this_array->Get(v8_context, index).ToLocalChecked();
    return wrap(v8_isolate, v8_value, v8_this_array);
  }

  throw JSException("list indices must be integers", PyExc_TypeError);
}

py::object JSObjectArrayImpl::SetItem(const py::object& py_key, const py::object& py_value) const {
  TRACE("JSObjectArrayImpl::SetItem {} py_key={} py_value={}", THIS, py_key, py_value);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  auto v8_this_array = m_base.ToV8(v8_isolate).As<v8::Array>();

  if (py::isinstance<py::slice>(py_key)) {
    auto py_slice = py::cast<py::slice>(py_key);

    py::ssize_t array_length = v8_this_array->Length();
    py::ssize_t slice_start;
    py::ssize_t slice_stop;
    py::ssize_t slice_step;
    py::ssize_t slice_length;

    // TODO: is there a pybind alternative to this code?
    PyObject* py_fast_sequence = PySequence_Fast(py_value.ptr(), "can only assign an iterable");
    if (!py_fast_sequence) {
      assert(PyErr_Occurred());
      return py::none();
    }
    py::ssize_t items_count = PySequence_Fast_GET_SIZE(py_fast_sequence);
    PyObject** py_items = PySequence_Fast_ITEMS(py_fast_sequence);

    if (PySlice_Unpack(py_slice.ptr(), &slice_start, &slice_stop, &slice_step) != 0) {
      assert(PyErr_Occurred());
      return py::none();
    }

    // TODO: there must be a simpler way how to set a slice...

    // If the slice start is greater than the array length we append null elements
    // to the tail of the array to fill the gap
    if (slice_start > array_length) {
      for (py::ssize_t i = array_length; i < slice_start; i++) {
        uint32_t index = array_length + i;
        v8_this_array->Set(v8_context, index, v8::Null(v8_isolate)).Check();
      }

      array_length = v8_this_array->Length();
    }

    // If the slice stop is greater than the array length (which was potentially
    // modified by the previous check) we append null elements to the tail of the
    // array. This step guarantees that the length of the array will always be
    // greater or equal than stop
    if (slice_stop > array_length) {
      for (py::ssize_t i = array_length; i < slice_stop; i++) {
        uint32_t index = i;
        v8_this_array->Set(v8_context, index, v8::Null(v8_isolate)).Check();
      }

      array_length = v8_this_array->Length();
    }

    if (!py_slice.compute(array_length, &slice_start, &slice_stop, &slice_step, &slice_length)) {
      assert(PyErr_Occurred());
      return py::none();
    }

    if (items_count != slice_length) {
      if (items_count < slice_length) {
        py::ssize_t diff = slice_length - items_count;
        py::ssize_t i;
        for (i = slice_start + items_count; i < array_length - diff; i++) {
          uint32_t index = i + diff;
          auto v8_val = v8_this_array->Get(v8_context, index).ToLocalChecked();
          v8_this_array->Set(v8_context, i, v8_val).Check();
        }
        for (i = array_length - 1; i > array_length - diff - 1; i--) {
          uint32_t index = i;
          v8_this_array->Delete(v8_context, index).Check();
        }
      } else if (items_count > slice_length) {
        py::ssize_t diff = items_count - slice_length;
        for (py::ssize_t i = array_length + diff - 1; i > slice_stop - 1; i--) {
          uint32_t index = i - diff;
          auto v8_val = v8_this_array->Get(v8_context, index).ToLocalChecked();
          v8_this_array->Set(v8_context, i, v8_val).Check();
        }
      }
    }

    for (py::ssize_t i = 0; i < items_count; i++) {
      auto py_item = py::reinterpret_borrow<py::object>(py_items[i]);
      auto v8_item = wrap(py_item);
      uint32_t index = slice_start + i * slice_step;
      v8_this_array->Set(v8_context, index, v8_item).Check();
    }
  } else if (py::isinstance<py::int_>(py_key)) {
    auto py_int = py::cast<py::int_>(py_key);
    uint32_t index = py_int;
    auto v8_value = wrap(py_value);
    v8_this_array->Set(v8_context, index, v8_value).Check();
  }

  return py_value;
}

py::object JSObjectArrayImpl::DelItem(const py::object& py_key) const {
  TRACE("JSObjectArrayImpl::DelItem {} py_key={}", THIS, py_key);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  auto v8_this_array = m_base.ToV8(v8_isolate).As<v8::Array>();

  if (py::isinstance<py::slice>(py_key)) {
    auto py_slice = py::cast<py::slice>(py_key);
    py::ssize_t array_length = v8_this_array->Length();
    py::ssize_t slice_start;
    py::ssize_t slice_stop;
    py::ssize_t slice_step;
    py::ssize_t slice_length;

    if (!py_slice.compute(array_length, &slice_start, &slice_stop, &slice_step, &slice_length)) {
      assert(PyErr_Occurred());
      return py::none();
    }

    for (py::ssize_t i = slice_start; i < slice_stop; i += slice_step) {
      uint32_t index = i;
      v8_this_array->Delete(v8_context, index).Check();
    }

    return py::none();
  } else if (py::isinstance<py::int_>(py_key)) {
    auto py_int = py::cast<py::int_>(py_key);
    uint32_t index = py_int;
    if (!v8_this_array->Has(v8_context, index).ToChecked()) {
      return py::none();
    }
    auto v8_val = v8_this_array->Get(v8_context, index).ToLocalChecked();
    auto py_result = wrap(v8_isolate, v8_val, v8_this_array);
    v8_this_array->Delete(v8_context, index).Check();
    return py_result;
  }

  throw JSException("list indices must be integers", PyExc_TypeError);
}

bool JSObjectArrayImpl::Contains(const py::object& py_key) const {
  TRACE("JSObjectArrayImpl::Contains {} py_key={}", THIS, py_key);
  auto v8_isolate = v8x::getCurrentIsolate();
  auto v8_scope = v8x::withScope(v8_isolate);
  auto v8_context = v8x::getCurrentContext(v8_isolate);
  auto v8_try_catch = v8x::withAutoTryCatch(v8_isolate);
  auto v8_this_array = m_base.ToV8(v8_isolate).As<v8::Array>();

  for (py::ssize_t i = 0; i < Length(); i++) {
    auto v8_maybe_val = v8_this_array->Get(v8_context, i);
    if (v8_maybe_val.IsEmpty()) {
      continue;
    }
    auto v8_val = v8_maybe_val.ToLocalChecked();
    if (py_key.is(wrap(v8_isolate, v8_val, v8_this_array))) {
      return true;
    }
  }

  return false;
}