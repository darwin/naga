#ifndef NAGA_JSOBJECT_H_
#define NAGA_JSOBJECT_H_

#include "Base.h"

// We want to avoid having a C++ class hierarchy providing various wrappers to Python side [1]
//
// Ideally we want to have one single C++ class representing a common/unified wrapper for all V8 objects.
// The wrapper is not polymorphic, but it can decide how to behave at runtime depending on its role.
// This ultimately depends on capabilities of the underlying V8 object (the wrapped object).
// For example the wrapper can act as a generic JS object wrapper or alternatively it can act as a JS function wrapper
// if the underlying V8 object happens to be a function, see JSObject::Roles for available roles.
// Please note that roles are not necessarily mutually exclusive. The wrapper can take on multiple roles.
// Role detection happens at the time of wrapping.
//
// On the other hand we want to structure our code in a sane way and don't want to dump all functionality
// into a single class. So for readability reasons we break JSObject into multiple implementation files.
// For example we want to keep role-specific functionality in separate implementation files.
// Also exposed API should be separated from other code.
//
// [1] https://github.com/area1/stpyv8/issues/8#issuecomment-606702978

class JSObject : public std::enable_shared_from_this<JSObject> {
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

  JSObject& Self() { return *this; }
  const JSObject& Self() const { return *this; }

 public:
  explicit JSObject(v8::Local<v8::Object> v8_obj);
  ~JSObject();

  [[nodiscard]] v8::Local<v8::Object> ToV8(v8x::LockedIsolatePtr& v8_isolate) const;

  bool HasRole(Roles roles) const;
  Roles GetRoles() const;

  void Dump(std::ostream& os) const;

  // exposed API

  static py::object Create(const SharedJSObjectPtr& proto, const py::tuple& py_args, const py::dict& py_kwds);

  py::object GetAttr(const py::object& py_key) const;
  void SetAttr(const py::object& py_key, const py::object& py_obj) const;
  void DelAttr(const py::object& py_key) const;
  [[nodiscard]] py::list Dir() const;

  py::object GetItem(const py::object& py_key) const;
  py::object SetItem(const py::object& py_key, const py::object& py_value) const;
  py::object DelItem(const py::object& py_key) const;
  bool Contains(const py::object& py_key) const;

  [[nodiscard]] int Hash() const;
  [[nodiscard]] SharedJSObjectPtr Clone() const;

  [[nodiscard]] size_t Len() const;
  [[nodiscard]] py::object Int() const;
  [[nodiscard]] py::object Float() const;
  [[nodiscard]] py::object Bool() const;
  [[nodiscard]] py::str Str() const;
  [[nodiscard]] py::str Repr() const;
  [[nodiscard]] py::object Iter();

  [[nodiscard]] bool EQ(const SharedJSObjectPtr& other) const;
  [[nodiscard]] bool NE(const SharedJSObjectPtr& other) const;

  py::object Call(const py::args& py_args, const py::kwargs& py_kwargs);
  py::object Apply(const py::object& py_self, const py::list& py_args, const py::dict& py_kwds);
  py::object Invoke(const py::list& py_args, const py::dict& py_kwds);

  [[nodiscard]] std::string GetName() const;
  void SetName(const std::string& name);

  [[nodiscard]] int LineNumber() const;
  [[nodiscard]] int ColumnNumber() const;
  [[nodiscard]] int LineOffset() const;
  [[nodiscard]] int ColumnOffset() const;
  [[nodiscard]] std::string ResourceName() const;
  [[nodiscard]] std::string InferredName() const;

  [[nodiscard]] bool HasRoleArray() const;
  [[nodiscard]] bool HasRoleFunction() const;
  [[nodiscard]] bool HasRoleCLJS() const;
};

static_assert(!std::is_polymorphic<JSObject>::value, "JSObject should not be polymorphic.");

std::ostream& operator<<(std::ostream& os, const JSObject::Roles& v);

#include "JSObjectAux.h"

#endif