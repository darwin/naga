#pragma once

#include "Base.h"

class CJSObjectBase {
 public:
  typedef std::uint8_t RoleFlagsType;
  enum class Roles : RoleFlagsType {
    Generic = 0,  // always on
    Function = 1 << 0,
    Array = 1 << 1,
    CLJS = 1 << 2
  };

 protected:
  Roles m_roles;
  // we need to have CJSObject movable for pybind
  v8::Global<v8::Object> m_v8_obj;

 public:
  explicit CJSObjectBase(v8::Local<v8::Object> v8_obj);
  ~CJSObjectBase();

  bool HasRole(Roles roles) const;

  [[nodiscard]] v8::Local<v8::Object> Object() const;

  void Dump(std::ostream& os) const;

  static py::object Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this);
  static py::object Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val);
  static py::object Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Object> v8_obj);
  static py::object Wrap(v8::IsolateRef v8_isolate, const CJSObjectPtr& obj);
};

#include "JSObjectBaseAux.h"