#pragma once

#include "Base.h"

class CJSObject {
 public:
  typedef std::uint8_t RoleFlagsType;
  enum class Roles : RoleFlagsType {
    JSObject = 0,  // always on
    JSFunction = 1 << 0,
    JSArray = 1 << 1,
    CLJSObject = 1 << 2
  };

 private:
  Roles m_roles;
  // we need to have CJSObject movable for pybind
  v8::Global<v8::Object> m_v8_obj;

  // JSFunction
  py::object Call(const py::list& py_args,
                  const py::dict& py_kwargs,
                  std::optional<v8::Local<v8::Object>> opt_v8_this = std::nullopt);

 public:
  explicit CJSObject(v8::Local<v8::Object> v8_obj);
  ~CJSObject();

  bool HasRole(Roles roles) const;

  [[nodiscard]] v8::Local<v8::Object> Object() const;

  py::object GetAttr(py::object py_key) const;
  void SetAttr(py::object py_key, py::object py_obj) const;
  void DelAttr(py::object py_key) const;

  py::object ObjectGetAttr(py::object py_key) const;
  void ObjectSetAttr(py::object py_key, py::object py_obj) const;
  void ObjectDelAttr(py::object py_key) const;

  py::object GetItem(py::object py_key) const;
  py::object SetItem(py::object py_key, py::object py_value) const;
  py::object DelItem(py::object py_key) const;

  [[nodiscard]] py::list GetAttrList() const;
  [[nodiscard]] int GetIdentityHash() const;
  [[nodiscard]] CJSObjectPtr Clone() const;

  bool Contains(const py::object& py_key) const;
  [[nodiscard]] bool ObjectContains(const py::object& py_key) const;

  [[nodiscard]] py::object ToPythonInt() const;
  [[nodiscard]] py::object ToPythonFloat() const;
  [[nodiscard]] py::object ToPythonBool() const;
  [[nodiscard]] py::object ToPythonStr() const;

  [[nodiscard]] bool Equals(const CJSObjectPtr& other) const;
  [[nodiscard]] bool Unequals(const CJSObjectPtr& other) const { return !Equals(other); }

  void Dump(std::ostream& os) const;

  static py::object Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this);
  static py::object Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val);
  static py::object Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Object> v8_obj);
  static py::object Wrap(v8::IsolateRef v8_isolate, const CJSObjectPtr& obj);

  static void Expose(const py::module& py_module);

  // JSFunction
  static py::object CallWithArgs(py::args py_args, const py::kwargs& py_kwargs);
  static py::object CreateWithArgs(const CJSObjectPtr& proto, const py::tuple& py_args, const py::dict& py_kwds);

  py::object ApplyJavascript(const CJSObjectPtr& self, const py::list& py_args, const py::dict& py_kwds);
  py::object ApplyPython(py::object py_self, const py::list& py_args, const py::dict& py_kwds);
  py::object Invoke(const py::list& py_args, const py::dict& py_kwds);

  [[nodiscard]] std::string GetName() const;
  void SetName(const std::string& name);

  [[nodiscard]] int GetLineNumber() const;
  [[nodiscard]] int GetColumnNumber() const;
  [[nodiscard]] std::string GetResourceName() const;
  [[nodiscard]] std::string GetInferredName() const;
  [[nodiscard]] int GetLineOffset() const;
  [[nodiscard]] int GetColumnOffset() const;

  // JSArray
  size_t ArrayLength() const;
  py::object ArrayGetItem(py::object py_key) const;
  py::object ArraySetItem(py::object py_key, py::object py_value) const;
  py::object ArrayDelItem(py::object py_key) const;
  bool ArrayContains(const py::object& py_key) const;

 protected:
  void CheckAttr(v8::Local<v8::String> v8_name) const;
};

static_assert(!std::is_polymorphic<CJSObject>::value, "CJSObject should not be polymorphic.");

constexpr CJSObject::Roles operator|(CJSObject::Roles X, CJSObject::Roles Y) {
  return static_cast<CJSObject::Roles>(static_cast<CJSObject::RoleFlagsType>(X) |
                                       static_cast<CJSObject::RoleFlagsType>(Y));
}

constexpr CJSObject::Roles operator&(CJSObject::Roles X, CJSObject::Roles Y) {
  return static_cast<CJSObject::Roles>(static_cast<CJSObject::RoleFlagsType>(X) &
                                       static_cast<CJSObject::RoleFlagsType>(Y));
}

inline CJSObject::Roles& operator|=(CJSObject::Roles& X, CJSObject::Roles Y) {
  X = X | Y;
  return X;
}