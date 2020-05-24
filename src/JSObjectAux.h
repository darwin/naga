#ifndef NAGA_JSOBJECTAUX_H_
#define NAGA_JSOBJECTAUX_H_

#include "Base.h"

// these are utility operators to enable bit-wise operations on JSObject::Roles

constexpr JSObject::Roles operator|(JSObject::Roles left, JSObject::Roles right) {
  return static_cast<JSObject::Roles>(static_cast<JSObject::RoleFlagsType>(left) |
                                      static_cast<JSObject::RoleFlagsType>(right));
}

constexpr JSObject::Roles operator&(JSObject::Roles left, JSObject::Roles right) {
  return static_cast<JSObject::Roles>(static_cast<JSObject::RoleFlagsType>(left) &
                                      static_cast<JSObject::RoleFlagsType>(right));
}

inline JSObject::Roles& operator|=(JSObject::Roles& receiver, JSObject::Roles right) {
  receiver = receiver | right;
  return receiver;
}

#endif