#pragma once

#include "Base.h"

class CPlatform;

typedef std::shared_ptr<CPlatform> CPlatformPtr;

class CPlatform {
 private:
  static bool m_inited;
  static std::unique_ptr<v8::Platform> m_platform;
  std::string m_argv;

 public:
  CPlatform() {}
  explicit CPlatform(std::string argv) : m_argv(std::move(argv)) {}
  ~CPlatform() = default;
  void Init();
};
