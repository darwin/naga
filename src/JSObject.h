#pragma once

#include "Base.h"

// https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html#std-shared-ptr
class CJSObject {
 protected:
  // we need to have CJSObject copyable for pybind
  v8::Global<v8::Object> m_v8_obj;

  CJSObject() = default;

 public:
  explicit CJSObject(v8::Local<v8::Object> v8_obj);
  virtual ~CJSObject();

  virtual void LazyInit() {}

  [[nodiscard]] v8::Local<v8::Object> Object() const;

  py::object GetAttr(const std::string& name);
  void SetAttr(const std::string& name, py::object py_obj) const;
  void DelAttr(const std::string& name);

  [[nodiscard]] py::list GetAttrList() const;
  [[nodiscard]] int GetIdentityHash() const;
  [[nodiscard]] CJSObjectPtr Clone() const;

  [[nodiscard]] bool Contains(const std::string& name) const;

  [[nodiscard]] py::object ToPythonInt() const;
  [[nodiscard]] py::object ToPythonFloat() const;
  [[nodiscard]] py::object ToPythonBool() const;
  [[nodiscard]] py::object ToPythonStr() const;

  [[nodiscard]] bool Equals(const CJSObjectPtr& other) const;
  [[nodiscard]] bool Unequals(const CJSObjectPtr& other) const { return !Equals(other); }

  void Dump(std::ostream& os) const;

  static py::object Wrap(v8::IsolateRef v8_isolate, const CJSObjectPtr& obj);
  static py::object Wrap(v8::IsolateRef v8_isolate,
                         v8::Local<v8::Value> v8_val,
                         v8::Local<v8::Object> v8_self = v8::Local<v8::Object>());
  static py::object Wrap(v8::IsolateRef v8_isolate,
                         v8::Local<v8::Object> v8_obj,
                         v8::Local<v8::Object> v8_self = v8::Local<v8::Object>());

  static void Expose(const py::module& py_module);

 protected:
  void CheckAttr(v8::Local<v8::String> v8_name) const;
};