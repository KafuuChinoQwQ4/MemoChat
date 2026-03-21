#include "logger.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <cstdio>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <mutex>
#include <filesystem>

namespace memochat {
namespace loadtest {

// ---- JSON log sink for spdlog ----
class JsonFileSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    explicit JsonFileSink(const std::string& filename) {
        file_.open(filename, std::ios::app);
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (!file_) return;
        file_ << format_to_json(msg) << "\n";
        // Do NOT flush here — spdlog controls flush cadence;
        // flushing on every call causes thread contention with 200+ workers.
        // The file_ destructor or explicit flush_() will drain the buffer.
    }

    void flush_() override {
        if (file_) file_.flush();
    }

private:
    std::string format_to_json(const spdlog::details::log_msg& msg) {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm_utc = *std::gmtime(&now_c);

        std::ostringstream ts;
        ts << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S");
        ts << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";

        std::string level = msg.level == spdlog::level::level_enum::err ? "error"
            : msg.level == spdlog::level::level_enum::warn ? "warn"
            : msg.level == spdlog::level::level_enum::debug ? "debug" : "info";

        std::string payload;
        {
            std::string raw_payload(msg.payload.data(), msg.payload.size());
            if (raw_payload.size() > 2 && raw_payload[0] == '{') {
                payload = std::move(raw_payload);
            } else {
                payload = "\"" + escape_json(raw_payload) + "\"";
            }
        }

        std::string logger_name_sv(msg.logger_name.data(), msg.logger_name.size());
        std::string event_name = logger_name_sv.empty() ? "log" : logger_name_sv;

        return "{"
            "\"ts\":\"" + ts.str() + "\","
            "\"level\":\"" + level + "\","
            "\"service\":\"loadtest\","
            "\"event\":\"" + event_name + "\","
            "\"message\":" + payload + ","
            "\"thread\":\"" + std::to_string(msg.thread_id) + "\""
            "}";
    }

    std::string escape_json(const std::string& s) {
        std::string out;
        for (char ch : s) {
            if (ch == '"') out += "\\\"";
            else if (ch == '\\') out += "\\\\";
            else if (ch == '\n') out += "\\n";
            else if (ch == '\r') out += "\\r";
            else if (ch == '\t') out += "\\t";
            else out += ch;
        }
        return out;
    }

    std::ofstream file_;
};

// ---- JsonLogger ----
JsonLogger& JsonLogger::instance() {
    static JsonLogger inst;
    return inst;
}

void JsonLogger::init(const std::string& name,
                       const std::string& log_dir,
                       LogLevel level) {
    std::lock_guard<std::shared_mutex> lock(mu_);
    if (logger_) return;

    name_ = name;
    std::filesystem::create_directories(log_dir);

    auto sink = std::make_shared<JsonFileSink>(
        (std::filesystem::path(log_dir) / (name + ".json")).string()
    );

    // File-only logger: ProgressMonitor handles console output via raw std::cout
    // to avoid multi-threaded console lock contention between spdlog's internal
    // mutex and stdout sink in high-concurrency scenarios (200+ threads).
    logger_ = std::make_shared<spdlog::logger>("json_logger", sink);
    logger_->set_pattern("%v");
    logger_->set_level(spdlog::level::info);
    spdlog::set_default_logger(logger_);

    set_level(level);
}

void JsonLogger::set_level(LogLevel level) {
    // NOTE: Do NOT acquire mu_ here. This method is called from init()
    // which already holds mu_. Additionally, spdlog::logger::set_level
    // is thread-safe internally. Acquiring the lock here would cause a
    // self-deadlock (same thread re-entering std::shared_mutex in exclusive mode).
    if (!logger_) return;
    switch (level) {
        case LogLevel::LV_DEBUG: logger_->set_level(spdlog::level::debug); break;
        case LogLevel::LV_INFO:  logger_->set_level(spdlog::level::info);  break;
        case LogLevel::LV_WARN:  logger_->set_level(spdlog::level::warn);  break;
        case LogLevel::LV_ERROR: logger_->set_level(spdlog::level::err);   break;
    }
}

void JsonLogger::log_event(const std::string& event,
                            LogLevel level,
                            const std::vector<std::pair<std::string, std::string>>& attrs) {
    std::ostringstream ss;
    ss << event;
    for (const auto& [k, v] : attrs) {
        ss << " " << k << "=" << v;
    }
    switch (level) {
        case LogLevel::LV_DEBUG: logger_->debug(ss.str()); break;
        case LogLevel::LV_INFO:  logger_->info(ss.str());  break;
        case LogLevel::LV_WARN:  logger_->warn(ss.str());  break;
        case LogLevel::LV_ERROR: logger_->error(ss.str()); break;
    }
}

void JsonLogger::debug(const std::string& msg) {
    // spdlog logger methods are thread-safe; no external locking needed.
    if (logger_) logger_->debug(msg);
}

void JsonLogger::info(const std::string& msg) {
    if (logger_) logger_->info(msg);
}

void JsonLogger::warn(const std::string& msg) {
    if (logger_) logger_->warn(msg);
}

void JsonLogger::error(const std::string& msg) {
    if (logger_) logger_->error(msg);
}

void JsonLogger::log_load_start(const std::string& scenario,
                                 int total, int concurrency,
                                 int accounts) {
    std::ostringstream ss;
    ss << "scenario=" << scenario
       << " total=" << total
       << " concurrency=" << concurrency
       << " accounts=" << accounts;
    info(ss.str());
}

void JsonLogger::log_load_summary(const std::string& scenario,
                                    int success, int failed,
                                    double rps,
                                    double p50_ms, double p95_ms, double p99_ms) {
    std::ostringstream ss;
    ss << "scenario=" << scenario
       << " success=" << success << " failed=" << failed
       << " rps=" << std::fixed << std::setprecision(2) << rps
       << " p50_ms=" << p50_ms
       << " p95_ms=" << p95_ms
       << " p99_ms=" << p99_ms;
    info(ss.str());
}

void JsonLogger::log_phase(const std::string& phase_name, double duration_ms) {
    std::ostringstream ss;
    ss << phase_name << " duration_ms=" << std::fixed << std::setprecision(3) << duration_ms;
    info(ss.str());
}

void JsonLogger::log_error(const std::string& trace_id,
                            const std::string& stage,
                            const std::string& err_msg) {
    std::ostringstream ss;
    ss << "trace_id=" << trace_id << " stage=" << stage << " error=" << err_msg;
    error(ss.str());
}

} // namespace loadtest
} // namespace memochat
