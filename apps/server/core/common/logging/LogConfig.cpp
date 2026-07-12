#include "logging/LogConfig.hpp"

#include <charconv>

import memochat.logging.config_algorithms;
import memochat.logging.log_config_algorithms;

namespace memolog
{
namespace log_config_modules = memochat::logging::log_config_modules;

namespace
{

std::string Trim(std::string value)
{
    const auto begin = memochat::logging::modules::TrimAsciiBegin(value.data(), value.size());
    const auto end = memochat::logging::modules::TrimAsciiEnd(value.data(), value.size());
    return value.substr(begin, end - begin);
}

std::string ToLower(std::string value)
{
    memochat::logging::modules::LowerAsciiInPlace(value.data(), value.size());
    return value;
}

bool ParseBool(const std::string& raw, bool fallback)
{
    if (raw.empty())
    {
        return fallback;
    }
    const std::string v = ToLower(Trim(raw));
    if (log_config_modules::IsTrueBoolToken(v.data(), v.size()))
    {
        return true;
    }
    if (log_config_modules::IsFalseBoolToken(v.data(), v.size()))
    {
        return false;
    }
    return fallback;
}

int ParseInt(const std::string& raw, int fallback)
{
    if (raw.empty())
    {
        return fallback;
    }
    int value = 0;
    const auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == raw.data() + raw.size() ? value : fallback;
}

} // namespace

LogConfig LogConfig::FromGetter(const std::function<std::string(const std::string&, const std::string&)>& getter)
{
    LogConfig cfg;
    if (!getter)
    {
        return cfg;
    }

    const auto read = [&](const std::string& key, const std::string& fallback)
    {
        const std::string v = Trim(getter("Log", key));
        return v.empty() ? fallback : v;
    };

    cfg.level = ToLower(read("Level", cfg.level));
    cfg.dir = read("Dir", cfg.dir);
    cfg.format = ToLower(read("Format", cfg.format));
    cfg.to_console = ParseBool(read("ToConsole", cfg.to_console ? "true" : "false"), cfg.to_console);
    cfg.rotate_mode = ToLower(read("RotateMode", cfg.rotate_mode));
    cfg.max_files = ParseInt(read("MaxFiles", std::to_string(cfg.max_files)), cfg.max_files);
    cfg.max_size_mb = ParseInt(read("MaxSizeMB", std::to_string(cfg.max_size_mb)), cfg.max_size_mb);
    cfg.redact = ParseBool(read("Redact", cfg.redact ? "true" : "false"), cfg.redact);
    cfg.env = read("Env", cfg.env);
    cfg.max_files = log_config_modules::NormalizeMaxFiles(cfg.max_files);
    cfg.max_size_mb = log_config_modules::NormalizeMaxSizeMb(cfg.max_size_mb);
    if (!log_config_modules::IsValidRotateMode(cfg.rotate_mode.data(), cfg.rotate_mode.size()))
    {
        cfg.rotate_mode = log_config_modules::DefaultRotateMode();
    }
    if (!log_config_modules::IsValidLogLevel(cfg.level.data(), cfg.level.size()))
    {
        cfg.level = log_config_modules::DefaultLogLevel();
    }
    return cfg;
}

} // namespace memolog
