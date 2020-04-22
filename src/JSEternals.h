#pragma once

#include "Base.h"
#include "JSIsolate.h"

// We maintain a table of eternal objects for fast access
// We keep one table per isolate and destroy the table before isolate goes away

class CJSEternals {
 public:
  enum EternalID { kJSObjectTemplate = 0, kJSExceptionType, kJSExceptionValue, kNumEternals };

  template <typename T>
  using EternalCreateFn = v8::Eternal<T>(v8::IsolateRef v8_isolate);

 private:
  v8::IsolateRef m_v8_isolate;
  // v8::Eternal is templated so we keep static array of std:any slots for them.
  // Initial creation might involve dynamic allocation, but lookups should be cheap.
  // We might consider implementing it as a simple array of pointers in release mode.
  std::array<std::any, kNumEternals> m_cache;

 public:
  explicit CJSEternals(v8::IsolateRef v8_isolate);
  ~CJSEternals();

  template <typename T>
  v8::Eternal<T> Get(EternalID id, EternalCreateFn<T>* create_fn = nullptr) {
    auto& lookup = m_cache[id];
    if (!lookup.has_value()) {
      HTRACE(kEternalsLogger, "CEternals::Get {} m_v8_isolate={} id={} creating...", THIS, P$(m_v8_isolate),
             magic_enum::enum_name(id));
      assert(create_fn);
      lookup = create_fn(m_v8_isolate);
    }
    auto v8_result = std::any_cast<v8::Eternal<T>>(lookup);
    assert(!v8_result.IsEmpty());
    HTRACE(kEternalsLogger, "CEternals::Get {} id={} => {}", THIS, magic_enum::enum_name(id), v8_result);
    return v8_result;
  }
};

template <typename T>
v8::Local<T> lookupEternal(v8::IsolateRef v8_isolate,
                           CJSEternals::EternalID id,
                           CJSEternals::EternalCreateFn<T>* create_fn = nullptr) {
  HTRACE(kEternalsLogger, "lookupEternal v8_isolate={} id={}", P$(v8_isolate), magic_enum::enum_name(id));
  auto isolate = CJSIsolate::FromV8(v8_isolate);
  auto v8_eternal_val = isolate->Eternals().Get(id, create_fn);
  return v8_eternal_val.Get(v8_isolate);
}
