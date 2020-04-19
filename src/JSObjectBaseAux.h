#pragma once

#include "Base.h"

// these are utility operators to enable bit-wise operations on CJSObjectBase::Roles

constexpr CJSObjectBase::Roles operator|(CJSObjectBase::Roles left, CJSObjectBase::Roles right) {
  return static_cast<CJSObjectBase::Roles>(static_cast<CJSObjectBase::RoleFlagsType>(left) |
                                           static_cast<CJSObjectBase::RoleFlagsType>(right));
}

constexpr CJSObjectBase::Roles operator&(CJSObjectBase::Roles left, CJSObjectBase::Roles right) {
  return static_cast<CJSObjectBase::Roles>(static_cast<CJSObjectBase::RoleFlagsType>(left) &
                                           static_cast<CJSObjectBase::RoleFlagsType>(right));
}

inline CJSObjectBase::Roles& operator|=(CJSObjectBase::Roles& receiver, CJSObjectBase::Roles right) {
  receiver = receiver | right;
  return receiver;
}