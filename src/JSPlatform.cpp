#include "JSPlatform.h"

#define TRACE(...) \
  LOGGER_INDENT;   \
  SPDLOG_LOGGER_TRACE(getLogger(kJSPlatformLogger), __VA_ARGS__)

// enforce singleton contract
constexpr auto singleton_invariants = !std::is_constructible<CJSPlatform>::value &&            //
                                      !std::is_assignable<CJSPlatform, CJSPlatform>::value &&  //
                                      !std::is_swappable<CJSPlatform>::value;                  //
static_assert(singleton_invariants, "CJSPlatform should be a singleton.");

CJSPlatform* CJSPlatform::Instance() {
  static CJSPlatform g_platform;
  TRACE("CJSPlatform::Instance => {}", (void*)&g_platform);
  return &g_platform;
}

bool CJSPlatform::Init(std::string argv) {
  TRACE("CJSPlatform::Init {} argv='{}'", THIS, argv);
  if (m_initialized) {
    TRACE("CJSPlatform::Init {} => [already initialized]", THIS);
    return false;
  }

#ifndef NDEBUG
  v8::V8::SetFlagsFromString("--expose-gc --allow-natives-syntax --track-retaining-path");
#endif

  // https://v8.dev/docs/i18n#embedding-v8
  v8::V8::InitializeICU();
  v8::V8::InitializeExternalStartupData(argv.c_str());

  m_v8_platform = v8::platform::NewDefaultPlatform();

  v8::V8::InitializePlatform(m_v8_platform.get());
  v8::V8::Initialize();

  m_initialized = true;
  return true;
}