#pragma once

#include "Base.h"

template <typename V8T, typename NT>
class CJSRegistry {
  using V8TP = const V8T*;
  using NTP = NT*;
  using TRegistry = std::unordered_map<V8TP, NTP>;
  TRegistry m_registry;

 public:
  void Register(V8TP v8_thing, NTP our_thing) {
    HTRACE(kJSRegistryLogger, "CJSRegistry::Register {} v8_thing={} our_thing={}", THIS, (void*)v8_thing,
           (void*)our_thing);
    assert(m_registry.find(v8_thing) == m_registry.end());
    m_registry.insert(std::make_pair(v8_thing, our_thing));
  }

  void Unregister(V8TP v8_thing) {
    HTRACE(kJSRegistryLogger, "CJSRegistry::Unregister {} v8_thing={}", THIS, (void*)v8_thing);
    auto it = m_registry.find(v8_thing);
    assert(it != m_registry.end());
    m_registry.erase(it);
  }

  NTP LookupRegistered(V8TP v8_thing) const {
    auto lookup = m_registry.find(v8_thing);
    auto result = ([&]() {
      if (lookup != m_registry.end()) {
        return lookup->second;
      } else {
        return static_cast<NTP>(nullptr);
      }
    })();
    HTRACE(kJSRegistryLogger, "CJSRegistry::LookupRegistered {} v8_thing={} => {}", THIS, (void*)v8_thing,
           (void*)result);
    return result;
  }
};
