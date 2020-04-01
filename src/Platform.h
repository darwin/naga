#pragma once

#include <v8.h>

#include <fstream>
#include <utility>

#include "Config.h"

class CPlatform {
 private:
  static bool inited;
  static std::unique_ptr<v8::Platform> platform;
//  constexpr static const char* icu_data = ICU_DATA;

  const char *GetICUDataFile() {
    // TODO: revisit
//    if (icu_data == nullptr)
//      return nullptr;
//
//    std::ifstream ifile(icu_data);
//    if (ifile)
//      return icu_data;

    return nullptr;
  }

  std::string argv;

 public:
  CPlatform() : argv(std::string()) {}
  explicit CPlatform(std::string argv0) : argv(std::move(argv0)) {}
  ~CPlatform() = default;
  void Init();
};
