#pragma once

#include "Base.h"

// https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html#std-shared-ptr
class CJSObject {
 protected:
  // we need to have CJSObject copyable for pybind
  // credit: https://stackoverflow.com/a/22648552/84283
  v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>> m_v8_obj;

  CJSObject() = default;

 public:
  explicit CJSObject(v8::Local<v8::Object> v8_obj) : m_v8_obj(v8::Isolate::GetCurrent(), v8_obj) {}
  virtual ~CJSObject() { m_v8_obj.Reset(); }

  virtual void LazyInit() {}

  [[nodiscard]] v8::Local<v8::Object> Object() const;

  pb::object GetAttr(const std::string& name);
  void SetAttr(const std::string& name, pb::object py_obj) const;
  void DelAttr(const std::string& name);

  [[nodiscard]] pb::list GetAttrList() const;
  [[nodiscard]] int GetIdentityHash() const;
  [[nodiscard]] CJSObjectPtr Clone() const;

  [[nodiscard]] bool Contains(const std::string& name) const;

  [[nodiscard]] pb::object ToPythonInt() const;
  [[nodiscard]] pb::object ToPythonFloat() const;
  [[nodiscard]] pb::object ToPythonBool() const;
  [[nodiscard]] pb::object ToPythonStr() const;

  [[nodiscard]] bool Equals(const CJSObjectPtr& other) const;
  [[nodiscard]] bool Unequals(const CJSObjectPtr& other) const { return !Equals(other); }

  void Dump(std::ostream& os) const;

  static pb::object Wrap(const CJSObjectPtr& obj);
  static pb::object Wrap(v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_self = v8::Local<v8::Object>());
  static pb::object Wrap(v8::Local<v8::Object> v8_obj, v8::Local<v8::Object> v8_self = v8::Local<v8::Object>());

  static void Expose(const pb::module& py_module);

 protected:
  void CheckAttr(v8::Local<v8::String> v8_name) const;
};