#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectNull : public CJSObject {
 public:
  [[nodiscard]] bool nonzero() const { return false; }
  [[nodiscard]] std::string str() const { return "null"; }
};