#pragma once

#include "Base.h"
#include "JSObject.h"

class CJavascriptNull : public CJSObject {
 public:
  bool nonzero() const { return false; }
  const std::string str() const { return "null"; }
};