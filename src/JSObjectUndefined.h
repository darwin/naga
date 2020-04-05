#pragma once

#include "Base.h"
#include "JSObject.h"

class CJSObjectUndefined;

typedef std::shared_ptr<CJSObjectUndefined> CJSObjectUndefinedPtr;

class CJSObjectUndefined : public CJSObject {
 public:
  bool nonzero() const { return false; }
  const std::string str() const { return "undefined"; }
};