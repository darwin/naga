#pragma once

#include "Base.h"

// these are utility operators to enable bit-wise operations on CJSObject::Roles

constexpr CJSObject::Roles operator|(CJSObject::Roles left, CJSObject::Roles right) {
  return static_cast<CJSObject::Roles>(static_cast<CJSObject::RoleFlagsType>(left) |
                                       static_cast<CJSObject::RoleFlagsType>(right));
}

constexpr CJSObject::Roles operator&(CJSObject::Roles left, CJSObject::Roles right) {
  return static_cast<CJSObject::Roles>(static_cast<CJSObject::RoleFlagsType>(left) &
                                       static_cast<CJSObject::RoleFlagsType>(right));
}

inline CJSObject::Roles& operator|=(CJSObject::Roles& receiver, CJSObject::Roles right) {
  receiver = receiver | right;
  return receiver;
}