#ifndef NAGA_JSPLATFORM_H_
#define NAGA_JSPLATFORM_H_

#include "Base.h"

class JSPlatform {
 private:
  bool m_initialized{false};
  std::unique_ptr<v8::Platform> m_v8_platform;

  // CPlatform is a singleton => make the constructor private, disable copy/move
 private:
  JSPlatform() = default;

 public:
  JSPlatform(const JSPlatform&) = delete;
  JSPlatform& operator=(const JSPlatform&) = delete;
  JSPlatform(JSPlatform&&) = delete;
  JSPlatform& operator=(JSPlatform&&) = delete;

  static JSPlatform* Instance();

  bool Initialized() const { return m_initialized; }
  bool Init(std::string argv);
};

#endif