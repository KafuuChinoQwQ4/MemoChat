#pragma once
#ifndef MEMOCHAT_LOADTEST_LOGGER_H
#define MEMOCHAT_LOADTEST_LOGGER_H

// winhttp.h / windows.h may define DEBUG as a macro — undefine before LogLevel enum
#ifdef DEBUG
#undef DEBUG
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/pattern_formatter.h>

#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <mutex>
#include <shared_mutex>

namespace memochat {
namespace loadtest {

enum class LogLevel { LV_DEBUG, LV_INFO, LV_WARN, LV_ERROR };

class JsonLogger {
public:
    static JsonLogger& instance();

    void init(const std::string& name,
              const std::string& log_dir,
              LogLevel level = LogLevel::LV_INFO);

    void set_level(LogLevel level);

    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

    void log_event(const std::string& event,
                   LogLevel level,
                   const std::vector<std::pair<std::string, std::string>>& attrs);

    // Convenience structured log helpers
    void log_load_start(const std::string& scenario,
                        int total, int concurrency,
                        int accounts);
    void log_load_summary(const std::string& scenario,
                          int success, int failed,
                          double rps,
                          double p50_ms, double p95_ms, double p99_ms);
    void log_phase(const std::string& phase_name, double duration_ms);
    void log_error(const std::string& trace_id,
                   const std::string& stage,
                   const std::string& err_msg);

    spdlog::logger* logger() const { return logger_.get(); }

private:
    JsonLogger() = default;
    ~JsonLogger() = default;
    JsonLogger(const JsonLogger&) = delete;
    JsonLogger& operator=(const JsonLogger&) = delete;

    std::string name_;
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_mutex mu_;
};

// Custom JSON formatter for spdlog
class JsonFormatter : public spdlog::formatter {
public:
    void format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest) override;
    std::unique_ptr<formatter> clone() const override;
};

// Load test event names
constexpr const char* EVT_LOAD_START      = "loadtest.start";
constexpr const char* EVT_LOAD_SUMMARY    = "loadtest.summary";
constexpr const char* EVT_LOAD_FAILED     = "loadtest.failed";
constexpr const char* EVT_WARMUP_START    = "loadtest.warmup.start";
constexpr const char* EVT_WARMUP_ITER     = "loadtest.warmup.iter";
constexpr const char* EVT_PHASE           = "loadtest.phase";
constexpr const char* EVT_ERROR           = "loadtest.error";

} // namespace loadtest
} // namespace memochat

#endif // MEMOCHAT_LOADTEST_LOGGER_H
