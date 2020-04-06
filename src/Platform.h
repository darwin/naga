#pragma once

#include "Base.h"

class CPlatform;

typedef std::shared_ptr<CPlatform> CPlatformPtr;

class CPlatform {
 private:
  static bool m_inited;
  std::string m_argv;
  static std::unique_ptr<v8::Platform> m_v8_platform;

 public:
  CPlatform() = default;
  ~CPlatform() = default;

  explicit CPlatform(std::string argv);

  void Init();

  static void Expose(const pb::module& m);
};
