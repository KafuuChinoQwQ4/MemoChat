#include "logging/Logger.hpp"

#include <charconv>

#include "logging/Redaction.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TraceContext.hpp"

#include "json/GlazeCompat.hpp"
#include <spdlog/logger.h>
#include <spdlog/sinks/sink.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <limits>
#include <mutex>
#include <system_error>
#include <utility>
#include <vector>

import memochat.logging.logger_algorithms;

namespace memolog
{

std::shared_ptr<spdlog::logger> Logger::logger_;
LogConfig Logger::config_{};
std::string Logger::service_name_ = "unknown";

namespace
{

std::string ToLevelString(spdlog::level::level_enum level)
{
    switch (level)
    {
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

std::string NowIso8601()
{
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    char buf[32]{0};
    std::snprintf(buf,
                  sizeof(buf),
                  "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                  tm.tm_year + 1900,
                  tm.tm_mon + 1,
                  tm.tm_mday,
                  tm.tm_hour,
                  tm.tm_min,
                  tm.tm_sec,
                  static_cast<int>(ms.count()));
    return buf;
}

std::string NormalizeDir(std::string dir)
{
    if (memochat::logging::logger_modules::ShouldUseDefaultLogDir(dir.empty()))
    {
        return "./logs";
    }
    return dir;
}

bool IsTopLevelField(const std::string& key)
{
    return memochat::logging::logger_modules::IsTopLevelFieldName(key.data(), key.size());
}

void AssignError(std::string* error, std::string message)
{
    if (error != nullptr)
    {
        *error = std::move(message);
    }
}

std::string ErrnoMessage(int error_number)
{
    if (error_number == 0)
    {
        return "I/O error";
    }
    return std::error_code(error_number, std::generic_category()).message();
}

bool WriteLine(std::FILE* stream, const char* data, std::size_t size)
{
    if (stream == nullptr)
    {
        return false;
    }

    std::size_t offset = 0;
    while (offset < size)
    {
        const std::size_t written = std::fwrite(data + offset, 1, size - offset, stream);
        if (written == 0)
        {
            return false;
        }
        offset += written;
    }
    return std::fputc('\n', stream) != EOF;
}

bool LocalDateStamp(std::string* stamp)
{
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local_time{};
#ifdef _WIN32
    if (localtime_s(&local_time, &now) != 0)
    {
        return false;
    }
#else
    if (localtime_r(&now, &local_time) == nullptr)
    {
        return false;
    }
#endif

    char buffer[16]{};
    const int length = std::snprintf(buffer,
                                     sizeof(buffer),
                                     "%04d-%02d-%02d",
                                     local_time.tm_year + 1900,
                                     local_time.tm_mon + 1,
                                     local_time.tm_mday);
    if (length != 10)
    {
        return false;
    }
    *stamp = std::string(buffer, static_cast<std::size_t>(length));
    return true;
}

class NonThrowingJsonSink final : public spdlog::sinks::sink
{
public:
    NonThrowingJsonSink(std::filesystem::path directory,
                        std::string service_name,
                        bool to_console,
                        bool size_rotation,
                        std::size_t max_files,
                        std::size_t max_size)
        : directory_(std::move(directory))
        , base_path_(directory_ / (std::move(service_name) + ".log.json"))
        , to_console_(to_console)
        , size_rotation_(size_rotation)
        , max_files_(max_files)
        , max_size_(max_size)
    {
    }

    ~NonThrowingJsonSink() override
    {
        if (file_ != nullptr && std::fclose(file_) != 0)
        {
            ReportFailure("failed to close log file: " + ErrnoMessage(errno));
        }
    }

    bool Initialize(std::string* error)
    {
        if (error != nullptr)
        {
            error->clear();
        }

        std::error_code directory_error;
        std::filesystem::create_directories(directory_, directory_error);
        if (directory_error)
        {
            return DisableFile("failed to create log directory '" + directory_.string() +
                                   "': " + directory_error.message(),
                               error);
        }

        const bool is_directory = std::filesystem::is_directory(directory_, directory_error);
        if (directory_error || !is_directory)
        {
            const std::string reason = directory_error ? directory_error.message() : "path is not a directory";
            return DisableFile("log directory '" + directory_.string() + "' is unavailable: " + reason, error);
        }

        if (size_rotation_)
        {
            std::string open_error;
            if (!OpenFile(base_path_, &open_error))
            {
                return DisableFile(std::move(open_error), error);
            }
            return true;
        }

        if (!LocalDateStamp(&daily_stamp_))
        {
            return DisableFile("failed to calculate local date for daily log rotation", error);
        }
        const auto daily_path = DailyPath(daily_stamp_);
        std::string open_error;
        if (!OpenFile(daily_path, &open_error))
        {
            return DisableFile(std::move(open_error), error);
        }
        daily_files_.push_back(daily_path);
        return true;
    }

    void log(const spdlog::details::log_msg& message) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const char* data = message.payload.data();
        const std::size_t size = message.payload.size();

        if (!file_ready_)
        {
            (void) WriteLine(stderr, data, size);
            return;
        }

        if (to_console_)
        {
            (void) WriteLine(stdout, data, size);
        }

        std::string error;
        if (!PrepareForWrite(size + 1U, &error) || !WriteLine(file_, data, size))
        {
            if (error.empty())
            {
                error = "failed to write log file '" + active_path_.string() + "': " + ErrnoMessage(errno);
            }
            DisableFile(std::move(error), nullptr);
            (void) WriteLine(stderr, data, size);
            return;
        }

        const std::size_t line_size = size + 1U;
        if (active_size_ > std::numeric_limits<std::size_t>::max() - line_size)
        {
            active_size_ = std::numeric_limits<std::size_t>::max();
        }
        else
        {
            active_size_ += line_size;
        }
    }

    void flush() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (to_console_)
        {
            (void) std::fflush(stdout);
        }
        if (!file_ready_ || file_ == nullptr)
        {
            (void) std::fflush(stderr);
            return;
        }
        if (std::fflush(file_) != 0)
        {
            DisableFile("failed to flush log file '" + active_path_.string() + "': " + ErrnoMessage(errno), nullptr);
        }
    }

    void set_pattern(const std::string&) override
    {
    }

    void set_formatter(std::unique_ptr<spdlog::formatter>) override
    {
    }

private:
    bool PrepareForWrite(std::size_t line_size, std::string* error)
    {
        if (size_rotation_)
        {
            const bool exceeds_limit = line_size > max_size_ || active_size_ > max_size_ - line_size;
            if (active_size_ != 0 && exceeds_limit)
            {
                return RotateBySize(error);
            }
            return true;
        }

        std::string current_stamp;
        if (!LocalDateStamp(&current_stamp))
        {
            AssignError(error, "failed to calculate local date for daily log rotation");
            return false;
        }
        if (current_stamp == daily_stamp_)
        {
            return true;
        }
        return RotateDaily(std::move(current_stamp), error);
    }

    bool RotateBySize(std::string* error)
    {
        if (!CloseFile(error))
        {
            return false;
        }

        for (std::size_t index = max_files_; index > 0; --index)
        {
            const auto source = index == 1 ? base_path_ : IndexedPath(index - 1);
            std::error_code exists_error;
            const bool source_exists = std::filesystem::exists(source, exists_error);
            if (exists_error)
            {
                AssignError(error,
                            "failed to inspect rotated log file '" + source.string() + "': " + exists_error.message());
                return false;
            }
            if (!source_exists)
            {
                continue;
            }

            const auto target = IndexedPath(index);
            std::error_code remove_error;
            (void) std::filesystem::remove(target, remove_error);
            if (remove_error)
            {
                AssignError(error,
                            "failed to remove rotated log file '" + target.string() + "': " + remove_error.message());
                return false;
            }

            std::error_code rename_error;
            std::filesystem::rename(source, target, rename_error);
            if (rename_error)
            {
                AssignError(error,
                            "failed to rotate log file '" + source.string() + "' to '" + target.string() +
                                "': " + rename_error.message());
                return false;
            }
        }
        return OpenFile(base_path_, error);
    }

    bool RotateDaily(std::string new_stamp, std::string* error)
    {
        if (!CloseFile(error))
        {
            return false;
        }

        const auto new_path = DailyPath(new_stamp);
        if (!OpenFile(new_path, error))
        {
            return false;
        }

        if (daily_files_.size() >= max_files_)
        {
            const auto expired = daily_files_.front();
            std::error_code remove_error;
            (void) std::filesystem::remove(expired, remove_error);
            if (remove_error)
            {
                AssignError(error,
                            "failed to remove expired daily log file '" + expired.string() +
                                "': " + remove_error.message());
                return false;
            }
            daily_files_.erase(daily_files_.begin());
        }

        daily_files_.push_back(new_path);
        daily_stamp_ = std::move(new_stamp);
        return true;
    }

    bool OpenFile(const std::filesystem::path& path, std::string* error)
    {
        errno = 0;
        file_ = std::fopen(path.string().c_str(), "ab");
        if (file_ == nullptr)
        {
            AssignError(error, "failed to open log file '" + path.string() + "': " + ErrnoMessage(errno));
            return false;
        }

        std::error_code size_error;
        const std::uintmax_t file_size = std::filesystem::file_size(path, size_error);
        if (size_error)
        {
            const std::string message = "failed to inspect log file '" + path.string() + "': " + size_error.message();
            (void) std::fclose(file_);
            file_ = nullptr;
            AssignError(error, message);
            return false;
        }

        active_path_ = path;
        active_size_ = file_size > std::numeric_limits<std::size_t>::max() ? std::numeric_limits<std::size_t>::max()
                                                                           : static_cast<std::size_t>(file_size);
        file_ready_ = true;
        return true;
    }

    bool CloseFile(std::string* error)
    {
        if (file_ == nullptr)
        {
            file_ready_ = false;
            return true;
        }

        std::FILE* current = std::exchange(file_, nullptr);
        file_ready_ = false;
        errno = 0;
        if (std::fclose(current) != 0)
        {
            AssignError(error, "failed to close log file '" + active_path_.string() + "': " + ErrnoMessage(errno));
            return false;
        }
        return true;
    }

    bool DisableFile(std::string message, std::string* error)
    {
        if (file_ != nullptr)
        {
            (void) std::fclose(std::exchange(file_, nullptr));
        }
        file_ready_ = false;
        AssignError(error, message);
        ReportFailure(message);
        return false;
    }

    static void ReportFailure(const std::string& message)
    {
        std::fprintf(stderr, "[Logger] %s; using stderr fallback\n", message.c_str());
        (void) std::fflush(stderr);
    }

    std::filesystem::path IndexedPath(std::size_t index) const
    {
        return base_path_.parent_path() /
               (base_path_.stem().string() + "." + std::to_string(index) + base_path_.extension().string());
    }

    std::filesystem::path DailyPath(const std::string& stamp) const
    {
        return base_path_.parent_path() / (base_path_.stem().string() + "_" + stamp + base_path_.extension().string());
    }

    std::mutex mutex_;
    std::filesystem::path directory_;
    std::filesystem::path base_path_;
    std::filesystem::path active_path_;
    std::FILE* file_ = nullptr;
    bool to_console_ = false;
    bool size_rotation_ = false;
    bool file_ready_ = false;
    std::size_t max_files_ = 1;
    std::size_t max_size_ = 1;
    std::size_t active_size_ = 0;
    std::string daily_stamp_;
    std::vector<std::filesystem::path> daily_files_;
};

} // namespace

spdlog::level::level_enum ParseLevel(const std::string& level)
{
    if (level == "trace")
    {
        return spdlog::level::trace;
    }
    if (level == "debug")
    {
        return spdlog::level::debug;
    }
    if (level == "warn")
    {
        return spdlog::level::warn;
    }
    if (level == "error")
    {
        return spdlog::level::err;
    }
    if (level == "critical")
    {
        return spdlog::level::critical;
    }
    return spdlog::level::info;
}

bool Logger::Init(const std::string& service_name, const LogConfig& cfg, std::string* error)
{
    Shutdown();
    config_ = cfg;
    service_name_ = service_name.empty() ? "unknown" : service_name;

    const std::string log_dir = NormalizeDir(config_.dir);
    const int configured_max_files = config_.max_files > 0 ? config_.max_files : 14;
    const int configured_max_size_mb = config_.max_size_mb > 0 ? config_.max_size_mb : 100;
    const auto max_size = static_cast<std::size_t>(configured_max_size_mb) * 1024U * 1024U;
    auto sink = std::make_shared<NonThrowingJsonSink>(std::filesystem::path(log_dir),
                                                      service_name_,
                                                      config_.to_console,
                                                      config_.rotate_mode == "size",
                                                      static_cast<std::size_t>(configured_max_files),
                                                      max_size);
    const bool initialized = sink->Initialize(error);
    logger_ = std::make_shared<spdlog::logger>(service_name_, std::move(sink));
    logger_->set_level(ParseLevel(config_.level));
    logger_->flush_on(spdlog::level::warn);
    return initialized;
}

void Logger::Shutdown()
{
    if (logger_)
    {
        logger_->flush();
    }
    logger_.reset();
}

void Logger::Log(spdlog::level::level_enum level,
                 const std::string& event,
                 const std::string& message,
                 const std::map<std::string, std::string>& fields)
{
    if (!logger_)
    {
        return;
    }

    memochat::json::JsonValue root(memochat::json::object_t{});
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
    if (!trace_id.empty())
    {
        root["trace_id"] = trace_id;
    }
    if (!request_id.empty())
    {
        root["request_id"] = request_id;
    }
    if (!span_id.empty())
    {
        root["span_id"] = span_id;
    }
    if (!uid.empty())
    {
        root["uid"] = uid;
    }
    if (!session_id.empty())
    {
        root["session_id"] = session_id;
    }

    memochat::json::JsonValue attrs(memochat::json::object_t{});
    for (const auto& it : fields)
    {
        const std::string redacted = RedactValue(it.first, it.second, config_.redact);
        if (it.first == "uid")
        {
            root["uid"] = redacted;
            continue;
        }
        if (it.first == "session_id")
        {
            root["session_id"] = redacted;
            continue;
        }
        if (IsTopLevelField(it.first))
        {
            if (it.first == "duration_ms")
            {
                double duration_ms = 0.0;
                const auto parsed = std::from_chars(redacted.data(), redacted.data() + redacted.size(), duration_ms);
                if (parsed.ec == std::errc{} && parsed.ptr == redacted.data() + redacted.size())
                {
                    root[it.first] = duration_ms;
                }
                else
                {
                    root[it.first] = redacted;
                }
            }
            else
            {
                root[it.first] = redacted;
            }
            continue;
        }
        attrs[it.first] = redacted;
    }
    root["attrs"] = attrs;

    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string line = memochat::json::writeString(builder, root);
    if (!line.empty() && line.back() == '\n')
    {
        line.pop_back();
    }
    logger_->log(level, spdlog::string_view_t(line.data(), line.size()));
}

std::shared_ptr<spdlog::logger> Logger::Get()
{
    return logger_;
}

const LogConfig& Logger::Config()
{
    return config_;
}

const std::string& Logger::ServiceName()
{
    return service_name_;
}

} // namespace memolog
