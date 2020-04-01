#include "Platform.h"

#include "libplatform/libplatform.h"

std::unique_ptr<v8::Platform> CPlatform::platform;
bool CPlatform::inited = false;

void CPlatform::Init() {
  if (inited)
    return;

  // https://v8.dev/docs/i18n#embedding-v8
  v8::V8::InitializeICU();
  v8::V8::InitializeExternalStartupData(argv.c_str());

  platform = v8::platform::NewDefaultPlatform();

  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  inited = true;
}
