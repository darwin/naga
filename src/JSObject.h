#pragma once

#include "Base.h"
#include "JSObjectAPI.h"

// We want to avoid having a C++ class hierarchy providing various wrappers to Python side [1]
//
// Ideally we want to have one single C++ class representing a common/unified wrapper for all V8 objects.
// The wrapper is not polymorphic, but it can decide how to behave at runtime depending on its role.
// This ultimately depends on capabilities of the underlying V8 object (the wrapped object).
// For example the wrapper can act as a generic JS object wrapper or alternatively it can act as a JS function wrapper
// if the underlying V8 object happens to be a function, see CJSObjectBase::Roles for available roles.
// Please note that roles are not necessarily mutually exclusive. The wrapper can take on multiple roles.
//
// On the other hand we want to structure our code in a sane way and don't want to dump all functionality
// into a single class. So for readability reasons we break JSObject into multiple C++ classes.
// We want to keep role-specific functionality in separate implementation classes and compose them later.
// But keep in mind that exposed wrapper is only the final composite CJSObject.
//
// Technically we wanted to avoid virtual inheritance of CJSObjectBase, so we adopted this idea [2]
//
// [1] https://github.com/area1/stpyv8/issues/8#issuecomment-606702978
// [2] https://stackoverflow.com/a/44941059/84283

class CJSObject final : public CJSObjectAPI {
 public:
  explicit CJSObject(v8::Local<v8::Object> v8_obj);
  ~CJSObject();
};

static_assert(!std::is_polymorphic<CJSObject>::value, "CJSObject should not be polymorphic.");