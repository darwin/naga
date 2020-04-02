#pragma once

#include "Base.h"
#include "Exception.h"

class CJSObject;
class CJavascriptFunction;

typedef std::shared_ptr<CJSObject> CJSObjectPtr;
typedef std::shared_ptr<CJavascriptFunction> CJavascriptFunctionPtr;

struct ILazyObject {
  virtual void LazyConstructor() = 0;
};

class CJSObject {
 protected:
  v8::Persistent<v8::Object> m_obj;

  void CheckAttr(v8::Local<v8::String> name) const;

  CJSObject() = default;

 public:
  explicit CJSObject(v8::Local<v8::Object> obj) : m_obj(v8::Isolate::GetCurrent(), obj) {}
  virtual ~CJSObject() { m_obj.Reset(); }

  static void Expose();

  v8::Local<v8::Object> Object() const { return v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), m_obj); }

  py::object GetAttr(const std::string& name);
  void SetAttr(const std::string& name, py::object value);
  void DelAttr(const std::string& name);

  py::list GetAttrList();
  int GetIdentityHash();
  CJSObjectPtr Clone();

  bool Contains(const std::string& name);

  explicit operator long() const;
  explicit operator double() const;
  explicit operator bool() const;

  bool Equals(CJSObjectPtr other) const;
  bool Unequals(CJSObjectPtr other) const { return !Equals(std::move(other)); }

  void Dump(std::ostream& os) const;

  static py::object Wrap(CJSObject* obj);
  static py::object Wrap(v8::Local<v8::Value> value, v8::Local<v8::Object> self = v8::Local<v8::Object>());
  static py::object Wrap(v8::Local<v8::Object> obj, v8::Local<v8::Object> self = v8::Local<v8::Object>());
};

class CJavascriptNull : public CJSObject {
 public:
  bool nonzero() const { return false; }
  const std::string str() const { return "null"; }
};

class CJavascriptUndefined : public CJSObject {
 public:
  bool nonzero() const { return false; }
  const std::string str() const { return "undefined"; }
};

class CJavascriptArray : public CJSObject, public ILazyObject {
  py::object m_items;
  size_t m_size;

 public:
  class ArrayIterator
      : public boost::iterator_facade<ArrayIterator, py::object const, boost::forward_traversal_tag, py::object> {
    CJavascriptArray* m_array;
    size_t m_idx;

   public:
    ArrayIterator(CJavascriptArray* array, size_t idx) : m_array(array), m_idx(idx) {}

    void increment() { m_idx++; }

    bool equal(ArrayIterator const& other) const { return m_array == other.m_array && m_idx == other.m_idx; }

    reference dereference() const { return m_array->GetItem(py::long_(m_idx)); }
  };

  explicit CJavascriptArray(v8::Local<v8::Array> array) : CJSObject(array), m_size(array->Length()) {}

  explicit CJavascriptArray(py::object items) : m_items(std::move(items)), m_size(0) {}

  size_t Length();

  py::object GetItem(py::object key);
  py::object SetItem(py::object key, py::object value);
  py::object DelItem(py::object key);
  bool Contains(py::object item);

  ArrayIterator begin() { return {this, 0}; }
  ArrayIterator end() { return {this, Length()}; }

  // ILazyObject
  void LazyConstructor() override;
};

class CJavascriptFunction : public CJSObject {
  v8::Persistent<v8::Object> m_self;

  py::object Call(v8::Local<v8::Object> self, py::list args, py::dict kwds);

 public:
  CJavascriptFunction(v8::Local<v8::Object> self, v8::Local<v8::Function> func)
      : CJSObject(func), m_self(v8::Isolate::GetCurrent(), self) {}

  ~CJavascriptFunction() override { m_self.Reset(); }

  v8::Local<v8::Object> Self() const { return v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), m_self); }

  static py::object CallWithArgs(py::tuple args, py::dict kwds);
  static py::object CreateWithArgs(CJavascriptFunctionPtr proto, py::tuple args, py::dict kwds);

  py::object ApplyJavascript(CJSObjectPtr self, py::list args, py::dict kwds);
  py::object ApplyPython(py::object self, py::list args, py::dict kwds);
  py::object Invoke(py::list args, py::dict kwds);

  const std::string GetName() const;
  void SetName(const std::string& name);

  int GetLineNumber() const;
  int GetColumnNumber() const;
  const std::string GetResourceName() const;
  const std::string GetInferredName() const;
  int GetLineOffset() const;
  int GetColumnOffset() const;

  py::object GetOwner() const;
};

class ObjectTracer;

typedef std::map<PyObject*, ObjectTracer*> LivingMap;

class ObjectTracer {
  v8::Persistent<v8::Value> m_handle;
  std::unique_ptr<py::object> m_object;

  LivingMap* m_living;

  void Trace();

  static LivingMap* GetLivingMapping();

 public:
  ObjectTracer(v8::Local<v8::Value> handle, py::object* object);
  ~ObjectTracer();

  py::object* Object() const { return m_object.get(); }

  void Dispose();

  static ObjectTracer& Trace(v8::Local<v8::Value> handle, py::object* object);

  static v8::Local<v8::Value> FindCache(py::object obj);
};

class ContextTracer {
  v8::Persistent<v8::Context> m_ctxt;
  std::unique_ptr<LivingMap> m_living;

  void Trace();

  static void WeakCallback(const v8::WeakCallbackInfo<ContextTracer>& info);

 public:
  ContextTracer(v8::Local<v8::Context> ctxt, LivingMap* living);
  ~ContextTracer();

  v8::Local<v8::Context> Context() const { return v8::Local<v8::Context>::New(v8::Isolate::GetCurrent(), m_ctxt); }

  static void Trace(v8::Local<v8::Context> ctxt, LivingMap* living);
};