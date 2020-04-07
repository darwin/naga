#include "Logging.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/cfg/env.h"

static bool initLogging() {
  // Set the log level to "info" and mylogger to to "trace":
  // SPDLOG_LEVEL=info,mylogger=trace && ./example
  spdlog::cfg::load_env_levels();

  // Set the default logger to file logger
  auto file_logger = spdlog::basic_logger_mt("stpyv8", "logs/stpyv8.txt");
  spdlog::set_default_logger(file_logger);

  return true;
}

void useLogging() {
  [[maybe_unused]] static bool initialized = initLogging();
}