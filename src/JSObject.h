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

  // Exposed Python API
  py::object PythonGetAttr(py::object py_key) const;
  void PythonSetAttr(py::object py_key, py::object py_obj) const;
  void PythonDelAttr(py::object py_key) const;

  py::object PythonGetItem(py::object py_key) const;
  py::object PythonSetItem(py::object py_key, py::object py_value) const;
  py::object PythonDelItem(py::object py_key) const;

  [[nodiscard]] py::list PythonGetAttrList() const;
  [[nodiscard]] int PythonIdentityHash() const;
  [[nodiscard]] CJSObjectPtr PythonClone() const;

  bool PythonContains(const py::object& py_key) const;

  [[nodiscard]] size_t PythonLength() const;
  [[nodiscard]] py::object PythonInt() const;
  [[nodiscard]] py::object PythonFloat() const;
  [[nodiscard]] py::object PythonBool() const;
  [[nodiscard]] py::object PythonStr() const;
  [[nodiscard]] py::object PythonRepr() const;

  [[nodiscard]] bool PythonEquals(const CJSObjectPtr& other) const;
  [[nodiscard]] bool PythonNotEquals(const CJSObjectPtr& other) const;

  // ---

  void Dump(std::ostream& os) const;
  static void Expose(const py::module& py_module);

  static py::object Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val, v8::Local<v8::Object> v8_this);
  static py::object Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Value> v8_val);
  static py::object Wrap(v8::IsolateRef v8_isolate, v8::Local<v8::Object> v8_obj);
  static py::object Wrap(v8::IsolateRef v8_isolate, const CJSObjectPtr& obj);

  // Default JSObject implementations
  static py::object PythonCreateWithArgs(const CJSObjectPtr& proto, const py::tuple& py_args, const py::dict& py_kwds);

  [[nodiscard]] bool ObjectContains(const py::object& py_key) const;

  py::object ObjectGetAttr(py::object py_key) const;
  void ObjectSetAttr(py::object py_key, py::object py_obj) const;
  void ObjectDelAttr(py::object py_key) const;

  // JSFunction
  static py::object PythonCallWithArgs(py::args py_args, const py::kwargs& py_kwargs);

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

  // CLJSObject
  size_t CLJSLength() const;
  py::object CLJSRepr() const;
  py::object CLJSStr() const;
  py::object CLJSGetItem(const py::object& py_key) const;
  py::object CLJSGetAttr(const py::object& py_key) const;

 private:
  py::object CLJSGetItemSlice(const py::object& py_slice) const;
  py::object CLJSGetItemIndex(const py::object& py_index) const;
  py::object CLJSGetItemString(const py::object& py_str) const;

  void CheckAttr(v8::Local<v8::String> v8_name) const;
};

static_assert(!std::is_polymorphic<CJSObject>::value, "CJSObject should not be polymorphic.");

#include "JSObjectAux.h"