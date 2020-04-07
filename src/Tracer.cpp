#include "Tracer.h"

ObjectTracer::ObjectTracer(v8::Local<v8::Value> v8_handle, PyObject* raw_object)
    : m_v8_handle(v8u::getCurrentIsolate(), v8_handle), m_raw_object(raw_object), m_living(GetLivingMapping()) {
  Py_INCREF(m_raw_object);
}

ObjectTracer::~ObjectTracer() {
  if (!m_v8_handle.IsEmpty()) {
    Dispose();
    m_living->erase(m_raw_object);
  }
}

void ObjectTracer::Dispose() {
  m_v8_handle.Reset();
  assert(m_raw_object);
  Py_DECREF(m_raw_object);
  m_raw_object = nullptr;
}

ObjectTracer& ObjectTracer::Trace(v8::Local<v8::Value> v8_handle, PyObject* raw_object) {
  std::unique_ptr<ObjectTracer> tracer(new ObjectTracer(v8_handle, raw_object));
  tracer->Trace();
  return *tracer.release();
}

void ObjectTracer::Trace() {
  m_living->insert(std::make_pair(m_raw_object, this));
}

LivingMap2* ObjectTracer::GetLivingMapping() {
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  auto v8_key = v8::String::NewFromUtf8(v8_isolate, "__living__").ToLocalChecked();
  auto v8_key_api = v8::Private::ForApi(v8_isolate, v8_key);

  auto v8_value = v8_context->Global()->GetPrivate(v8_context, v8_key_api);

  if (!v8_value.IsEmpty()) {
    auto v8_val = v8_value.ToLocalChecked();
    if (v8_val->IsExternal()) {
      LivingMap2* living = (LivingMap2*)v8_val.As<v8::External>()->Value();

      if (living) {
        return living;
      }
    }
  }

  std::unique_ptr<LivingMap2> living(new LivingMap2());

  auto v8_living = v8::External::New(v8u::getCurrentIsolate(), living.get());
  v8_context->Global()->SetPrivate(v8_context, v8_key_api, v8_living);

  ContextTracer::Trace(v8_context, living.get());

  return living.release();
}

v8::Local<v8::Value> ObjectTracer::FindCache(PyObject* raw_object) {
  auto living = GetLivingMapping();
  if (living) {
    auto it = living->find(raw_object);
    if (it != living->end()) {
      return v8::Local<v8::Value>::New(v8u::getCurrentIsolate(), it->second->m_v8_handle);
    }
  }

  return v8::Local<v8::Value>();
}
py::object ObjectTracer::Object() const {
  return py::reinterpret_borrow<py::object>(m_raw_object);
}

ContextTracer::ContextTracer(v8::Local<v8::Context> v8_context, LivingMap2* living)
    : m_v8_context(v8u::getCurrentIsolate(), v8_context), m_living(living) {}

ContextTracer::~ContextTracer() {
  auto v8_isolate = v8u::getCurrentIsolate();
  auto v8_scope = v8u::openScope(v8_isolate);
  auto v8_context = v8_isolate->GetCurrentContext();

  auto v8_key = v8::String::NewFromUtf8(v8_isolate, "__living__").ToLocalChecked();
  auto v8_key_api = v8::Private::ForApi(v8_isolate, v8_key);

  Context()->Global()->DeletePrivate(v8_context, v8_key_api);

  for (auto& it : *m_living) {
    std::unique_ptr<ObjectTracer> tracer(it.second);
    tracer->Dispose();
  }
}

void ContextTracer::WeakCallback(const v8::WeakCallbackInfo<ContextTracer>& v8_info) {
  delete v8_info.GetParameter();
}

void ContextTracer::Trace(v8::Local<v8::Context> v8_context, LivingMap2* living) {
  auto tracer = new ContextTracer(v8_context, living);
  tracer->Trace();
}

void ContextTracer::Trace() {
  m_v8_context.SetWeak(this, WeakCallback, v8::WeakCallbackType::kFinalizer);
}

v8::Local<v8::Context> ContextTracer::Context() const {
  auto v8_isolate = v8u::getCurrentIsolate();
  return v8::Local<v8::Context>::New(v8_isolate, m_v8_context);
}
