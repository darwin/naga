#ifndef NAGA_JSOBJECTBASE_H_
#define NAGA_JSOBJECTBASE_H_

#include "Base.h"

class JSObjectBase {
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
  explicit JSObjectBase(v8::Local<v8::Object> v8_obj);
  ~JSObjectBase();

  [[nodiscard]] v8::Local<v8::Object> ToV8(v8x::LockedIsolatePtr& v8_isolate) const;

  bool HasRole(Roles roles) const;
  Roles GetRoles() const;

  void Dump(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const JSObjectBase::Roles& v);

#include "JSObjectBaseAux.h"

#endif