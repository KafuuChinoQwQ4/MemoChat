#pragma once

#include "runtime/IniConfig.h"

using SectionInfo = memochat::runtime::IniSection;

class ConfigMgr {
public:
    ConfigMgr();
    ~ConfigMgr() = default;

    ConfigMgr(const ConfigMgr&) = default;
    ConfigMgr& operator=(const ConfigMgr&) = default;

    SectionInfo operator[](const std::string& section) const;

    static ConfigMgr& Inst();

    static void InitConfigPath(const std::string& config_path);
    static std::string GetConfigPath();

    std::string GetValue(const std::string& section, const std::string& key) const;

private:
    static std::string& ConfigPathStorage();

    memochat::runtime::IniConfig _config;
};

