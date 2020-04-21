#pragma once

#include "Base.h"
#include "Isolate.h"

// We maintain a table of eternal objects for fast access
// We keep one table per isolate and destroy the table before isolate goes away

class CEternals {
 public:
  enum EternalID { kJSObjectTemplate = 0, kNumEternals };

  template <typename T>
  using EternalCreateFn = v8::Eternal<T>(v8::IsolateRef v8_isolate);

 private:
  v8::IsolateRef m_v8_isolate;
  // Eternal is templated so we keep static array of std:any slots for them
  // initial creation might involve dynamic allocation, but lookups should be cheap
  std::array<std::any, kNumEternals> m_cache;

 public:
  explicit CEternals(v8::IsolateRef v8_isolate);
  ~CEternals();

  template <typename T>
  v8::Eternal<T> Get(EternalID id, EternalCreateFn<T>* create_fn = nullptr) {
    auto& lookup = m_cache[id];
    if (!lookup.has_value()) {
      HTRACE(kEternalsLogger, "CEternals::Get {} m_v8_isolate={} id={} creating...", THIS,
             isolateref_printer{m_v8_isolate}, magic_enum::enum_name(id));
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
v8::Eternal<T> getCachedEternal(v8::IsolateRef v8_isolate,
                                CEternals::EternalID id,
                                CEternals::EternalCreateFn<T>* create_fn = nullptr) {
  HTRACE(kEternalsLogger, "getCachedEternal v8_isolate={} id={}", isolateref_printer{v8_isolate},
         magic_enum::enum_name(id));
  auto isolate = CIsolate::FromV8(v8_isolate);
  return isolate->Eternals()->Get(id, create_fn);
}
