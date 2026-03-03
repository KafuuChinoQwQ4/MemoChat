#include "logging/LogConfig.h"

#include <algorithm>
#include <cctype>

namespace memolog {
namespace {

std::string Trim(std::string value) {
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char c) {
        return !is_space(static_cast<unsigned char>(c));
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](char c) {
        return !is_space(static_cast<unsigned char>(c));
    }).base(), value.end());
    return value;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool ParseBool(const std::string& raw, bool fallback) {
    if (raw.empty()) {
        return fallback;
    }
    const std::string v = ToLower(Trim(raw));
    if (v == "1" || v == "true" || v == "yes" || v == "on") {
        return true;
    }
    if (v == "0" || v == "false" || v == "no" || v == "off") {
        return false;
    }
    return fallback;
}

int ParseInt(const std::string& raw, int fallback) {
    try {
        if (!raw.empty()) {
            return std::stoi(raw);
        }
    } catch (...) {
    }
    return fallback;
}

} // namespace

LogConfig LogConfig::FromGetter(
    const std::function<std::string(const std::string&, const std::string&)>& getter) {
    LogConfig cfg;
    if (!getter) {
        return cfg;
    }

    const auto read = [&](const std::string& key, const std::string& fallback) {
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
    if (cfg.max_files <= 0) {
        cfg.max_files = 14;
    }
    if (cfg.max_size_mb <= 0) {
        cfg.max_size_mb = 100;
    }
    if (cfg.rotate_mode != "daily" && cfg.rotate_mode != "size") {
        cfg.rotate_mode = "daily";
    }
    if (cfg.level != "trace" && cfg.level != "debug" && cfg.level != "info" &&
        cfg.level != "warn" && cfg.level != "error" && cfg.level != "critical") {
        cfg.level = "info";
    }
    return cfg;
}

} // namespace memolog
