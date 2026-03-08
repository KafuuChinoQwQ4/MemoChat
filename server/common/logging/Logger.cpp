#include "logging/Logger.h"

#include "logging/Redaction.h"
#include "logging/Telemetry.h"
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
#include <set>
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

bool IsTopLevelField(const std::string& key) {
    static const std::set<std::string> known = {
        "service_instance",
        "module",
        "peer_service",
        "error_code",
        "error_type",
        "duration_ms",
        "uid",
        "session_id"
    };
    return known.find(key) != known.end();
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
    root["service_instance"] = Telemetry::ServiceInstance();
    root["env"] = config_.env;
    root["event"] = event;
    root["message"] = message;
    root["module"] = "";
    root["peer_service"] = "";
    root["error_code"] = "";
    root["error_type"] = "";
    root["duration_ms"] = 0;

    const auto& trace_id = TraceContext::GetTraceId();
    const auto& request_id = TraceContext::GetRequestId();
    const auto& span_id = TraceContext::GetSpanId();
    const auto& uid = TraceContext::GetUid();
    const auto& session_id = TraceContext::GetSessionId();
    if (!trace_id.empty()) {
        root["trace_id"] = trace_id;
    }
    if (!request_id.empty()) {
        root["request_id"] = request_id;
    }
    if (!span_id.empty()) {
        root["span_id"] = span_id;
    }
    if (!uid.empty()) {
        root["uid"] = uid;
    }
    if (!session_id.empty()) {
        root["session_id"] = session_id;
    }

    Json::Value attrs(Json::objectValue);
    for (const auto& it : fields) {
        const std::string redacted = RedactValue(it.first, it.second, config_.redact);
        if (it.first == "uid") {
            root["uid"] = redacted;
            continue;
        }
        if (it.first == "session_id") {
            root["session_id"] = redacted;
            continue;
        }
        if (IsTopLevelField(it.first)) {
            if (it.first == "duration_ms") {
                try {
                    root[it.first] = std::stod(redacted);
                } catch (...) {
                    root[it.first] = redacted;
                }
            } else {
                root[it.first] = redacted;
            }
            continue;
        }
        attrs[it.first] = redacted;
    }
    root["attrs"] = attrs;

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
