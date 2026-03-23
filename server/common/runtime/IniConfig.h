#pragma once

#include <map>
#include <memory>
#include <string>

#include "EtcdConfig.h"

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

    bool TryLoadFromEtcd(const std::string& endpoints, const std::string& service_name);
    void StartEtcdWatch();
    void StopEtcdWatch();

    bool IsEtcdAvailable() const { return _etcd_config && _etcd_config->IsAvailable(); }

private:
    static std::string SanitizeEnvToken(const std::string& raw);

    void OnEtcdConfigChange(const std::string& section, const std::string& key, const std::string& value);

    std::map<std::string, IniSection> _config;
    std::shared_ptr<EtcdConfig> _etcd_config;
};

} // namespace memochat::runtime
