#include "logging/Logger.h"

#include "logging/Redaction.h"
#include "logging/TraceContext.h"

#include <json/json.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <vector>

namespace memolog {

std::shared_ptr<spdlog::logger> Logger::logger_;
LogConfig Logger::config_{};
std::string Logger::service_name_ = "unknown";

namespace {

std::string ToLevelString(spdlog::level::level_enum level) {
    switch (level) {
    case spdlog::level::trace:
        return "trace";
    case spdlog::level::debug:
        return "debug";
    case spdlog::level::info:
        return "info";
    case spdlog::level::warn:
        return "warn";
    case spdlog::level::err:
        return "error";
    case spdlog::level::critical:
        return "critical";
    default:
        return "info";
    }
}

std::string NowIso8601() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto tt = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    char buf[32]{0};
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                  tm.tm_year + 1900,
                  tm.tm_mon + 1,
                  tm.tm_mday,
                  tm.tm_hour,
                  tm.tm_min,
                  tm.tm_sec,
                  static_cast<int>(ms.count()));
    return buf;
}

std::string NormalizeDir(std::string dir) {
    if (dir.empty()) {
        return "./logs";
    }
    return dir;
}

} // namespace

spdlog::level::level_enum ParseLevel(const std::string& level) {
    if (level == "trace") {
        return spdlog::level::trace;
    }
    if (level == "debug") {
        return spdlog::level::debug;
    }
    if (level == "warn") {
        return spdlog::level::warn;
    }
    if (level == "error") {
        return spdlog::level::err;
    }
    if (level == "critical") {
        return spdlog::level::critical;
    }
    return spdlog::level::info;
}

void Logger::Init(const std::string& service_name, const LogConfig& cfg) {
    config_ = cfg;
    service_name_ = service_name.empty() ? "unknown" : service_name;

    const std::string log_dir = NormalizeDir(config_.dir);
    std::filesystem::create_directories(log_dir);

    std::vector<spdlog::sink_ptr> sinks;
    if (config_.to_console) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%v");
        sinks.push_back(console_sink);
    }

    if (config_.rotate_mode == "size") {
        const auto max_size = static_cast<size_t>(config_.max_size_mb) * 1024U * 1024U;
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_dir + "/" + service_name_ + ".log.json", max_size, config_.max_files);
        file_sink->set_pattern("%v");
        sinks.push_back(file_sink);
    } else {
        auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            log_dir + "/" + service_name_ + ".log.json", 0, 0, false, config_.max_files);
        daily_sink->set_pattern("%v");
        sinks.push_back(daily_sink);
    }

    logger_ = std::make_shared<spdlog::logger>(service_name_, sinks.begin(), sinks.end());
    logger_->set_level(ParseLevel(config_.level));
    logger_->flush_on(spdlog::level::warn);
    spdlog::register_logger(logger_);
}

void Logger::Shutdown() {
    if (logger_) {
        logger_->flush();
    }
    spdlog::drop_all();
    logger_.reset();
}

void Logger::Log(spdlog::level::level_enum level,
                 const std::string& event,
                 const std::string& message,
                 const std::map<std::string, std::string>& fields) {
    if (!logger_) {
        return;
    }

    Json::Value root(Json::objectValue);
    root["ts"] = NowIso8601();
    root["level"] = ToLevelString(level);
    root["service"] = service_name_;
    root["env"] = config_.env;
    root["event"] = event;
    root["message"] = message;

    const auto& trace_id = TraceContext::GetTraceId();
    const auto& request_id = TraceContext::GetRequestId();
    const auto& uid = TraceContext::GetUid();
    const auto& session_id = TraceContext::GetSessionId();
    if (!trace_id.empty()) {
        root["trace_id"] = trace_id;
    }
    if (!request_id.empty()) {
        root["request_id"] = request_id;
    }
    if (!uid.empty()) {
        root["uid"] = uid;
    }
    if (!session_id.empty()) {
        root["session_id"] = session_id;
    }

    for (const auto& it : fields) {
        root[it.first] = RedactValue(it.first, it.second, config_.redact);
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string line = Json::writeString(builder, root);
    if (!line.empty() && line.back() == '\n') {
        line.pop_back();
    }
    logger_->log(level, line);
}

std::shared_ptr<spdlog::logger> Logger::Get() {
    return logger_;
}

const LogConfig& Logger::Config() {
    return config_;
}

const std::string& Logger::ServiceName() {
    return service_name_;
}

} // namespace memolog
