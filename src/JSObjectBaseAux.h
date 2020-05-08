#ifndef NAGA_JSOBJECTBASEAUX_H_
#define NAGA_JSOBJECTBASEAUX_H_

#include "Base.h"

// these are utility operators to enable bit-wise operations on JSObjectBase::Roles

constexpr JSObjectBase::Roles operator|(JSObjectBase::Roles left, JSObjectBase::Roles right) {
  return static_cast<JSObjectBase::Roles>(static_cast<JSObjectBase::RoleFlagsType>(left) |
                                          static_cast<JSObjectBase::RoleFlagsType>(right));
}

constexpr JSObjectBase::Roles operator&(JSObjectBase::Roles left, JSObjectBase::Roles right) {
  return static_cast<JSObjectBase::Roles>(static_cast<JSObjectBase::RoleFlagsType>(left) &
                                          static_cast<JSObjectBase::RoleFlagsType>(right));
}

inline JSObjectBase::Roles& operator|=(JSObjectBase::Roles& receiver, JSObjectBase::Roles right) {
  receiver = receiver | right;
  return receiver;
}

#endif