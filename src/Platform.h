#pragma once

#include "Base.h"

class CPlatform {
 private:
  bool m_initialized{false};
  std::unique_ptr<v8::Platform> m_v8_platform;

  // CPlatform is a singleton => make the constructor private, disable copy/move
 private:
  CPlatform() = default;

 public:
  CPlatform(const CPlatform&) = delete;
  CPlatform& operator=(const CPlatform&) = delete;
  CPlatform(CPlatform&&) = delete;
  CPlatform& operator=(CPlatform&&) = delete;

  static CPlatform* Instance();

  bool Initialized() const { return m_initialized; }
  bool Init(std::string argv);
};