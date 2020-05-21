#ifndef NAGA_JSETERNALS_H_
#define NAGA_JSETERNALS_H_

#include "Base.h"
#include "JSIsolate.h"
#include "Printing.h"
#include "Logging.h"

// We maintain a table of eternal objects for fast access
// We keep one table per isolate and destroy the table before isolate goes away

class JSEternals {
 public:
  enum EternalID {
    kJSWrapperTemplate = 0,
    kJSExceptionType,
    kJSExceptionValue,
    kTracerPayload,
    kConstructorString,
    kCLJSLangTypeString,
    kNumEternals
  };

  template <typename T>
  using EternalCreateFn = v8::Eternal<T>(v8x::LockedIsolatePtr& v8_isolate);

 private:
  v8x::ProtectedIsolatePtr m_v8_isolate;
  // v8::Eternal is templated so we keep static array of std:any slots for them.
  // Initial creation might involve dynamic allocation, but lookups should be cheap.
  // We might consider implementing it as a simple array of pointers in release mode.
  std::array<std::any, kNumEternals> m_cache;

 public:
  explicit JSEternals(v8x::ProtectedIsolatePtr v8_protected_isolate);
  ~JSEternals();

  template <typename T>
  v8::Eternal<T> GetOrCreate(EternalID id, EternalCreateFn<T>* create_fn = nullptr) {
    auto& lookup = m_cache[id];
    if (!lookup.has_value()) {
      HTRACE(kJSEternalsLogger, "JSEternals::GetOrCreate {} m_v8_isolate={} id={} creating...", THIS, m_v8_isolate,
             magic_enum::enum_name(id));
      assert(create_fn);
      auto v8_isolate = m_v8_isolate.lock();
      lookup = create_fn(v8_isolate);
    }
    auto v8_result = std::any_cast<v8::Eternal<T>>(lookup);
    assert(!v8_result.IsEmpty());
    HTRACE(kJSEternalsLogger, "JSEternals::GetOrCreate {} id={} => {}", THIS, magic_enum::enum_name(id), v8_result);
    return v8_result;
  }
};

template <typename T>
v8::Local<T> lookupEternal(v8x::LockedIsolatePtr& v8_isolate,
                           JSEternals::EternalID id,
                           JSEternals::EternalCreateFn<T>* create_fn = nullptr) {
  HTRACE(kJSEternalsLogger, "lookupEternal v8_isolate={} id={}", P$(v8_isolate), magic_enum::enum_name(id));
  auto isolate = JSIsolate::FromV8(v8_isolate);
  auto v8_eternal_val = isolate->Eternals().GetOrCreate(id, create_fn);
  return v8_eternal_val.Get(v8_isolate);
}

#endif