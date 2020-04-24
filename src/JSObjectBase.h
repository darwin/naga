#pragma once

#include "Base.h"

class CJSObjectBase {
 public:
  using RoleFlagsType = std::uint8_t;

  enum class Roles : RoleFlagsType {
    Generic = 0,  // always on
    Function = 1 << 0,
    Array = 1 << 1,
    CLJS = 1 << 2
  };

 protected:
  Roles m_roles;
  v8::Global<v8::Object> m_v8_obj;

 public:
  explicit CJSObjectBase(v8::Local<v8::Object> v8_obj);
  ~CJSObjectBase();

  [[nodiscard]] v8::Local<v8::Object> ToV8(v8::IsolatePtr v8_isolate) const;

  bool HasRole(Roles roles) const;
  Roles GetRoles() const;

  void Dump(std::ostream& os) const;
};

#include "JSObjectBaseAux.h"