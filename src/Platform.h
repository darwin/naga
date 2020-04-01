#pragma once

#include <v8.h>

#include <fstream>
#include <utility>

#include "Config.h"

class CPlatform {
 private:
  static bool inited;
  static std::unique_ptr<v8::Platform> platform;
  std::string argv;

 public:
  CPlatform() : argv(std::string()) {}
  explicit CPlatform(std::string argv0) : argv(std::move(argv0)) {}
  ~CPlatform() = default;
  void Init();
};
