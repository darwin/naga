#pragma once

#include "Base.h"

class CJSObject;

typedef std::shared_ptr<CJSObject> CJSObjectPtr;

// https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html#std-shared-ptr
class CJSObject {
 protected:
  // we need to have CJSObject copyable for pybind
  // credit: https://stackoverflow.com/a/22648552/84283
  v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>> m_obj;

  void CheckAttr(v8::Local<v8::String> name) const;

  CJSObject() = default;

 public:
  explicit CJSObject(v8::Local<v8::Object> obj) : m_obj(v8::Isolate::GetCurrent(), obj) {}
  virtual ~CJSObject() { m_obj.Reset(); }

  static void Expose(pb::module& m);

  v8::Local<v8::Object> Object() const { return v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), m_obj); }

  py::object GetAttr(const std::string& name);
  pb::object GetAttr2(const std::string& name);
  void SetAttr(const std::string& name, py::object value);
  void SetAttr2(const std::string& name, pb::object py_obj);
  void DelAttr(const std::string& name);
  void DelAttr2(const std::string& name);

  py::list GetAttrList();
  int GetIdentityHash();
  CJSObjectPtr Clone();

  bool Contains(const std::string& name);

  py::object ToPythonInt() const;
  py::object ToPythonFloat() const;
  py::object ToPythonBool() const;
  py::object ToPythonStr() const;

  bool Equals(CJSObjectPtr other) const;
  bool Unequals(CJSObjectPtr other) const { return !Equals(std::move(other)); }

  void Dump(std::ostream& os) const;

  static py::object Wrap(CJSObject* obj);
  static py::object Wrap(v8::Local<v8::Value> value, v8::Local<v8::Object> self = v8::Local<v8::Object>());
  static py::object Wrap(v8::Local<v8::Object> obj, v8::Local<v8::Object> self = v8::Local<v8::Object>());

  static pb::object Wrap2(CJSObjectPtr obj);
  static pb::object Wrap2(v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_self = v8::Local<v8::Object>());
  static pb::object Wrap2(v8::Local<v8::Object> v8_obj, v8::Local<v8::Object> v8_self = v8::Local<v8::Object>());
};