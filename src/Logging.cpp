#include "Logging.h"
#include "Base.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/cfg/env.h"

static std::shared_ptr<spdlog::logger> g_loggers[kNumLoggers];

static void setupLogger(const std::shared_ptr<spdlog::logger>& logger) {
  // always flush
  logger->flush_on(spdlog::level::trace);
}

static void initLoggers() {
  auto logger_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/stpyv8.txt");

  g_loggers[kRootLogger] = std::make_shared<spdlog::logger>("stpyv8", logger_file_sink);
  g_loggers[kPythonObjectLogger] = std::make_shared<spdlog::logger>("stpyv8pyobj", logger_file_sink);

  for (auto& logger : g_loggers) {
    setupLogger(logger);
    spdlog::register_logger(logger);
  }

  spdlog::set_default_logger(g_loggers[kRootLogger]);
}

static bool initLogging() {
  initLoggers();

  // set the log level to "info" and mylogger to to "trace":
  // SPDLOG_LEVEL=info,mylogger=trace && ./example

  // note: this call must go after initLoggers() because it modifies existing loggers registry
  spdlog::cfg::load_env_levels();

  return true;
}

void useLogging() {
  [[maybe_unused]] static bool initialized = initLogging();
}

LoggerRef getLogger(Loggers which) {
  auto logger = g_loggers[which];
  return LoggerRef(logger.get());
}