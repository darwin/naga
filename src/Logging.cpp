#include "Logging.h"

static bool initLogging() {
  // https://github.com/gabime/spdlog/issues/1318#issuecomment-602248684
  // we are using compile-time log levels, so we crank up runtime levels to max
  spdlog::set_level(spdlog::level::trace);
  return true;
}

void useLogging() {
  [[maybe_unused]] static bool initialized = initLogging();
}