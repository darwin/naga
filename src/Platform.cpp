#include "Platform.h"

std::unique_ptr<v8::Platform> CPlatform::m_v8_platform;
bool CPlatform::m_inited = false;

void CPlatform::Expose(const pb::module& m) {
  // clang-format off
  pb::class_<CPlatform, CPlatformPtr>(m, "JSPlatform", "JSPlatform allows the V8 platform to be initialized")
      .def(pb::init<std::string>(),
           pb::arg("argv") = std::string())
      .def("init", &CPlatform::Init,
           "Initializes the platform");
  // clang-format on
}

void CPlatform::Init() {
  if (m_inited) {
    return;
  }

  // https://v8.dev/docs/i18n#embedding-v8
  v8::V8::InitializeICU();
  v8::V8::InitializeExternalStartupData(m_argv.c_str());

  m_v8_platform = v8::platform::NewDefaultPlatform();

  v8::V8::InitializePlatform(m_v8_platform.get());
  v8::V8::Initialize();

  m_inited = true;
}

CPlatform::CPlatform(std::string argv) : m_argv(std::move(argv)) {}
