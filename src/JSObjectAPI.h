#ifndef NAGA_JSOBJECTAPI_H_
#define NAGA_JSOBJECTAPI_H_

#include "Base.h"
#include "JSObjectBase.h"
#include "JSObjectGenericImpl.h"
#include "JSObjectFunctionImpl.h"
#include "JSObjectArrayImpl.h"
#include "JSObjectCLJSImpl.h"

// this class maintains JSObject API exposed to Python
class JSObjectAPI : public JSObjectBase {
  // this is dirty but we know that each JSObjectAPI instance is also full JSObject instance
  JSObject& Self() { return *static_cast<JSObject*>(static_cast<void*>(this)); }
  const JSObject& Self() const { return *static_cast<const JSObject*>(static_cast<const void*>(this)); }

 public:
  explicit JSObjectAPI(v8::Local<v8::Object> v8_obj) : JSObjectBase(v8_obj) {}

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
#endif