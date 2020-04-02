#include "Platform.h"

#include "libplatform/libplatform.h"

std::unique_ptr<v8::Platform> CPlatform::m_platform;
bool CPlatform::m_inited = false;

void CPlatform::Init() {
  if (m_inited)
    return;

  // https://v8.dev/docs/i18n#embedding-v8
  v8::V8::InitializeICU();
  v8::V8::InitializeExternalStartupData(m_argv.c_str());

  m_platform = v8::platform::NewDefaultPlatform();

  v8::V8::InitializePlatform(m_platform.get());
  v8::V8::Initialize();

  m_inited = true;
}
