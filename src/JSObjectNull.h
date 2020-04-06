#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectNull : public CJSObject{
 public:
  bool nonzero() const { return false; }
  std::string str() const { return "null"; }
};