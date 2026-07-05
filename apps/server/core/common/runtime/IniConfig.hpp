#pragma once

#include <map>
#include <memory>
#include <string>

#include "EtcdConfig.hpp"

namespace memochat::runtime
{

struct IniSection
{
    std::map<std::string, std::string> values;
    std::string section_name; ///< Populated by IniConfig::operator[] so GetValue can check env overrides.

    std::string operator[](const std::string& key) const;

    /**
     * Look up a config value.
     * Priority: environment variable MEMOCHAT_<SECTION>_<KEY>  >  INI file value.
     * The env key is derived by upper-casing section + key and replacing non-alphanumeric
     * characters with '_', matching IniConfig::EnvKeyFor().
     */
    std::string GetValue(const std::string& key) const;

private:
    static std::string BuildEnvKey(const std::string& section, const std::string& key);
};

class IniConfig
{
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

    bool IsEtcdAvailable() const
    {
        return _etcd_config && _etcd_config->IsAvailable();
    }

private:
    static std::string SanitizeEnvToken(const std::string& raw);

    void OnEtcdConfigChange(const std::string& section, const std::string& key, const std::string& value);

    std::map<std::string, IniSection> _config;
    std::shared_ptr<EtcdConfig> _etcd_config;
};

} // namespace memochat::runtime
