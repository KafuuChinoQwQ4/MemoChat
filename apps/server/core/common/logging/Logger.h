#pragma once

#include "logging/LogConfig.h"

#include <spdlog/common.h>

#include <map>
#include <memory>
#include <string>

namespace spdlog {
class logger;
}

namespace memolog {

class Logger {
public:
    static void Init(const std::string& service_name, const LogConfig& cfg);
    static void Shutdown();
    static void Log(spdlog::level::level_enum level,
                    const std::string& event,
                    const std::string& message,
                    const std::map<std::string, std::string>& fields = {});
    static std::shared_ptr<spdlog::logger> Get();
    static const LogConfig& Config();
    static const std::string& ServiceName();

private:
    static std::shared_ptr<spdlog::logger> logger_;
    static LogConfig config_;
    static std::string service_name_;
};

spdlog::level::level_enum ParseLevel(const std::string& level);

inline void LogDebug(const std::string& event,
                     const std::string& message,
                     const std::map<std::string, std::string>& fields = {}) {
    Logger::Log(spdlog::level::debug, event, message, fields);
}

inline void LogInfo(const std::string& event,
                    const std::string& message,
                    const std::map<std::string, std::string>& fields = {}) {
    Logger::Log(spdlog::level::info, event, message, fields);
}

inline void LogWarn(const std::string& event,
                    const std::string& message,
                    const std::map<std::string, std::string>& fields = {}) {
    Logger::Log(spdlog::level::warn, event, message, fields);
}

inline void LogError(const std::string& event,
                     const std::string& message,
                     const std::map<std::string, std::string>& fields = {}) {
    Logger::Log(spdlog::level::err, event, message, fields);
}

} // namespace memolog
