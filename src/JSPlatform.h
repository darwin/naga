#ifndef NAGA_JSPLATFORM_H_
#define NAGA_JSPLATFORM_H_

#include "Base.h"

class CJSPlatform {
 private:
  bool m_initialized{false};
  std::unique_ptr<v8::Platform> m_v8_platform;

  // CPlatform is a singleton => make the constructor private, disable copy/move
 private:
  CJSPlatform() = default;

 public:
  CJSPlatform(const CJSPlatform&) = delete;
  CJSPlatform& operator=(const CJSPlatform&) = delete;
  CJSPlatform(CJSPlatform&&) = delete;
  CJSPlatform& operator=(CJSPlatform&&) = delete;

  static CJSPlatform* Instance();

  bool Initialized() const { return m_initialized; }
  bool Init(std::string argv);
};

#endif