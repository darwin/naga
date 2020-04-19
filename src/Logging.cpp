#include "Logging.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/cfg/env.h"

static std::shared_ptr<spdlog::logger> g_loggers[kNumLoggers];
size_t LoggerIndent::m_indent = 0;

// this is our attempt to replace fmt's v-flag with wide padding
// for some reason they support only max 64 characters
// while we are at it we also handle multi-line case, which would not be covered by standard padding
class wide_v_formatter final : public spdlog::custom_flag_formatter {
  static const size_t m_log_message_padding_width = 300;

 public:
  void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override {
    auto text_size = msg.payload.size();
    auto text = std::string_view(msg.payload.begin(), text_size);

    auto inner_indent = 2 * LoggerIndent::GetIndent();

    std::size_t lines_count = 1;
    std::size_t text_pos = -1;
    std::size_t last_line_start_pos = 0;
    while (true) {
      last_line_start_pos = text_pos + 1;
      text_pos = text.find("\n", text_pos + 1);
      if (text_pos == std::string::npos) {
        break;
      }
      lines_count++;
    }

    auto last_line_length =
        inner_indent + text_size - last_line_start_pos;  // in single-line case this is total string length

    assert(last_line_length >= 0);
    assert(lines_count > 0);
    auto padding_size = 0;
    if (m_log_message_padding_width > last_line_length) {
      padding_size = m_log_message_padding_width - last_line_length;
    }

    // we rely on the fact that dest is created fresh for each new log message
    // that means that current size is what was already printed as prefix
    // something like "22:15:18.521 T stpyv8_pyo | "
    // we are going to indent each line but first
    auto indent_size = dest.size();
    assert(indent_size >= 2);
    auto indents_size = (lines_count - 1) * indent_size;
    auto inner_indents_size = lines_count * inner_indent;

    dest.reserve(dest.size() + text_size + indents_size + padding_size + inner_indents_size);

    // print the text line by line
    text_pos = -1;
    while (true) {
      last_line_start_pos = text_pos + 1;
      text_pos = text.find("\n", text_pos + 1);
      if (last_line_start_pos > 0) {
        // print indent with separator,
        // -2 is stripping last two characters from prefix to draw separator below
        for (size_t i = 0; i < indent_size - 2; i++) {
          dest.push_back(' ');
        }
        dest.push_back('|');
        dest.push_back(' ');
      }
      // print inner indent for each line
      for (size_t i = 0; i < inner_indent; i++) {
        dest.push_back(' ');
      }
      if (text_pos == std::string::npos) {
        // this is the last line
        // print remainder with padding
        dest.append(text.data() + last_line_start_pos, text.data() + text_size);
        while (padding_size > 0) {
          dest.push_back(' ');
          padding_size--;
        }
        break;
      } else {
        // print the line, including the new line
        dest.append(text.data() + last_line_start_pos, text.data() + text_pos + 1);
      }
    }
  }

  [[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override {
    return spdlog::details::make_unique<wide_v_formatter>();
  }
};

static void setupLogger(const std::shared_ptr<spdlog::logger>& logger) {
  // always flush
  logger->flush_on(spdlog::level::trace);
}

static void initLoggers() {
  auto logger_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/stpyv8.txt");

  // keep all logger names same length to have logger names aligned
  g_loggers[kRootLogger] = std::make_shared<spdlog::logger>("stpyv8_rot", logger_file_sink);
  g_loggers[kPythonObjectLogger] = std::make_shared<spdlog::logger>("stpyv8_pyo", logger_file_sink);
  g_loggers[kContextLogger] = std::make_shared<spdlog::logger>("stpyv8_ctx", logger_file_sink);
  g_loggers[kEngineLogger] = std::make_shared<spdlog::logger>("stpyv8_eng", logger_file_sink);
  g_loggers[kIsolateLogger] = std::make_shared<spdlog::logger>("stpyv8_iso", logger_file_sink);
  g_loggers[kPlatformLogger] = std::make_shared<spdlog::logger>("stpyv8_plt", logger_file_sink);
  g_loggers[kScriptLogger] = std::make_shared<spdlog::logger>("stpyv8_scr", logger_file_sink);
  g_loggers[kLockingLogger] = std::make_shared<spdlog::logger>("stpyv8_lck", logger_file_sink);
  g_loggers[kJSExceptionLogger] = std::make_shared<spdlog::logger>("stpyv8_jse", logger_file_sink);
  g_loggers[kJSStackFrameLogger] = std::make_shared<spdlog::logger>("stpyv8_jsf", logger_file_sink);
  g_loggers[kJSStackTraceLogger] = std::make_shared<spdlog::logger>("stpyv8_jst", logger_file_sink);
  g_loggers[kJSObjectLogger] = std::make_shared<spdlog::logger>("stpyv8_jso", logger_file_sink);
  g_loggers[kV8TracingLogger] = std::make_shared<spdlog::logger>("stpyv8_v8t", logger_file_sink);
  g_loggers[kAuxLogger] = std::make_shared<spdlog::logger>("stpyv8_aux", logger_file_sink);

  for (auto& logger : g_loggers) {
    setupLogger(logger);
    spdlog::register_logger(logger);
  }

  spdlog::set_default_logger(g_loggers[kRootLogger]);

  auto custom_formatter = std::make_unique<spdlog::pattern_formatter>();
  custom_formatter->add_flag<wide_v_formatter>('*');
  custom_formatter->set_pattern("%H:%M:%S.%e %L %n | %-400*   |> %s:%#");
  spdlog::set_formatter(std::move(custom_formatter));
  spdlog::set_error_handler(
      [](const std::string& msg) { throw std::runtime_error(fmt::format("LOGGING ERROR: {}", msg)); });
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