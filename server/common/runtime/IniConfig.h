#pragma once

#include <map>
#include <string>

namespace memochat::runtime {

struct IniSection {
    std::map<std::string, std::string> values;

    std::string operator[](const std::string& key) const;
    std::string GetValue(const std::string& key) const;
};

class IniConfig {
public:
    explicit IniConfig(const std::string& configPath = std::string());

    IniConfig(const IniConfig&) = default;
    IniConfig& operator=(const IniConfig&) = default;

    IniSection operator[](const std::string& section) const;
    std::string GetValue(const std::string& section, const std::string& key) const;

    static std::string ResolvePath(const std::string& overridePath);
    static std::string EnvKeyFor(const std::string& section, const std::string& key);

private:
    static std::string SanitizeEnvToken(const std::string& raw);

    std::map<std::string, IniSection> _config;
};

} // namespace memochat::runtime
