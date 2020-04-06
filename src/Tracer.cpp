#include "Tracer.h"

// ObjectTracer::ObjectTracer(v8::Local<v8::Value> handle, py::object* object)
//    : m_handle(v8::Isolate::GetCurrent(), handle), m_object(object), m_living(GetLivingMapping()) {}
//
// ObjectTracer::~ObjectTracer() {
//  if (!m_handle.IsEmpty()) {
//    Dispose();
//
//    m_living->erase(m_object->ptr());
//  }
//}
//
// void ObjectTracer::Dispose() {
//  // m_handle.ClearWeak();
//  m_handle.Reset();
//}
//
// ObjectTracer& ObjectTracer::Trace(v8::Local<v8::Value> handle, py::object* object) {
//  std::unique_ptr<ObjectTracer> tracer(new ObjectTracer(handle, object));
//
//  tracer->Trace();
//
//  return *tracer.release();
//}
//
// void ObjectTracer::Trace() {
//  // m_handle.SetWeak(this, WeakCallback, v8::WeakCallbackType::kParameter);
//
//  m_living->insert(std::make_pair(m_object->ptr(), this));
//}
//
///*
// void ObjectTracer::WeakCallback(const v8::WeakCallbackInfo<ObjectTracer>& info)
//{
//  std::unique_ptr<ObjectTracer> tracer(info.GetParameter());
//
//  // assert(info.GetValue() == tracer->m_handle);
//}
//*/
//
// LivingMap* ObjectTracer::GetLivingMapping() {
//  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
//
//  v8::Local<v8::Context> ctxt = v8::Isolate::GetCurrent()->GetCurrentContext();
//
//  v8::Local<v8::String> key = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "__living__").ToLocalChecked();
//  v8::Local<v8::Private> privateKey = v8::Private::ForApi(v8::Isolate::GetCurrent(), key);
//
//  v8::MaybeLocal<v8::Value> value = ctxt->Global()->GetPrivate(ctxt, privateKey);
//
//  if (!value.IsEmpty()) {
//    auto val = value.ToLocalChecked();
//    if (val->IsExternal()) {
//      LivingMap* living = (LivingMap*)v8::External::Cast(*val)->Value();
//
//      if (living)
//        return living;
//    }
//  }
//
//  std::unique_ptr<LivingMap> living(new LivingMap());
//
//  ctxt->Global()->SetPrivate(ctxt, privateKey, v8::External::New(v8::Isolate::GetCurrent(), living.get()));
//
//  ContextTracer::Trace(ctxt, living.get());
//
//  return living.release();
//}
//
// v8::Local<v8::Value> ObjectTracer::FindCache(py::object obj) {
//  LivingMap* living = GetLivingMapping();
//
//  if (living) {
//    LivingMap::const_iterator it = living->find(obj.ptr());
//
//    if (it != living->end()) {
//      return v8::Local<v8::Value>::New(v8::Isolate::GetCurrent(), it->second->m_handle);
//    }
//  }
//
//  return v8::Local<v8::Value>();
//}
//
// ContextTracer::ContextTracer(v8::Local<v8::Context> ctxt, LivingMap* living)
//    : m_ctxt(v8::Isolate::GetCurrent(), ctxt), m_living(living) {}
//
// ContextTracer::~ContextTracer() {
//  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
//
//  v8::Local<v8::Context> ctxt = v8::Isolate::GetCurrent()->GetCurrentContext();
//  v8::Local<v8::String> key = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "__living__").ToLocalChecked();
//  v8::Local<v8::Private> privateKey = v8::Private::ForApi(v8::Isolate::GetCurrent(), key);
//
//  Context()->Global()->DeletePrivate(ctxt, privateKey);
//
//  for (LivingMap::const_iterator it = m_living->begin(); it != m_living->end(); it++) {
//    std::unique_ptr<ObjectTracer> tracer(it->second);
//
//    tracer->Dispose();
//  }
//}
//
// void ContextTracer::WeakCallback(const v8::WeakCallbackInfo<ContextTracer>& info) {
//  delete info.GetParameter();
//}
//
// void ContextTracer::Trace(v8::Local<v8::Context> ctxt, LivingMap* living) {
//  ContextTracer* tracer = new ContextTracer(ctxt, living);
//
//  tracer->Trace();
//}
//
// void ContextTracer::Trace() {
//  m_ctxt.SetWeak(this, WeakCallback, v8::WeakCallbackType::kFinalizer);
//}

// ---------------------------------------------------------------------

ObjectTracer2::ObjectTracer2(v8::Local<v8::Value> handle, PyObject* object)
    : m_handle(v8::Isolate::GetCurrent(), handle), m_object(object), m_living(GetLivingMapping()) {
  Py_INCREF(m_object);
}

ObjectTracer2::~ObjectTracer2() {
  if (!m_handle.IsEmpty()) {
    Dispose();
    m_living->erase(m_object);
  }
}

void ObjectTracer2::Dispose() {
  m_handle.Reset();
  assert(m_object);
  Py_DECREF(m_object);
  m_object = nullptr;
}

ObjectTracer2& ObjectTracer2::Trace(v8::Local<v8::Value> handle, PyObject* object) {
  std::unique_ptr<ObjectTracer2> tracer(new ObjectTracer2(handle, object));
  tracer->Trace();
  return *tracer.release();
}

void ObjectTracer2::Trace() {
  m_living->insert(std::make_pair(m_object, this));
}

LivingMap2* ObjectTracer2::GetLivingMapping() {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Local<v8::Context> ctxt = v8::Isolate::GetCurrent()->GetCurrentContext();

  v8::Local<v8::String> key = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "__living__").ToLocalChecked();
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(v8::Isolate::GetCurrent(), key);

  v8::MaybeLocal<v8::Value> value = ctxt->Global()->GetPrivate(ctxt, privateKey);

  if (!value.IsEmpty()) {
    auto val = value.ToLocalChecked();
    if (val->IsExternal()) {
      LivingMap2* living = (LivingMap2*)v8::External::Cast(*val)->Value();

      if (living)
        return living;
    }
  }

  std::unique_ptr<LivingMap2> living(new LivingMap2());

  ctxt->Global()->SetPrivate(ctxt, privateKey, v8::External::New(v8::Isolate::GetCurrent(), living.get()));

  ContextTracer2::Trace(ctxt, living.get());

  return living.release();
}

v8::Local<v8::Value> ObjectTracer2::FindCache(PyObject* obj) {
  LivingMap2* living = GetLivingMapping();

  if (living) {
    LivingMap2::const_iterator it = living->find(obj);

    if (it != living->end()) {
      return v8::Local<v8::Value>::New(v8::Isolate::GetCurrent(), it->second->m_handle);
    }
  }

  return v8::Local<v8::Value>();
}

ContextTracer2::ContextTracer2(v8::Local<v8::Context> ctxt, LivingMap2* living)
    : m_ctxt(v8::Isolate::GetCurrent(), ctxt), m_living(living) {}

ContextTracer2::~ContextTracer2() {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Local<v8::Context> ctxt = v8::Isolate::GetCurrent()->GetCurrentContext();
  v8::Local<v8::String> key = v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), "__living__").ToLocalChecked();
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(v8::Isolate::GetCurrent(), key);

  Context()->Global()->DeletePrivate(ctxt, privateKey);

  for (LivingMap2::const_iterator it = m_living->begin(); it != m_living->end(); it++) {
    std::unique_ptr<ObjectTracer2> tracer(it->second);

    tracer->Dispose();
  }
}

void ContextTracer2::WeakCallback(const v8::WeakCallbackInfo<ContextTracer2>& info) {
  delete info.GetParameter();
}

void ContextTracer2::Trace(v8::Local<v8::Context> ctxt, LivingMap2* living) {
  ContextTracer2* tracer = new ContextTracer2(ctxt, living);

  tracer->Trace();
}

void ContextTracer2::Trace() {
  m_ctxt.SetWeak(this, WeakCallback, v8::WeakCallbackType::kFinalizer);
}