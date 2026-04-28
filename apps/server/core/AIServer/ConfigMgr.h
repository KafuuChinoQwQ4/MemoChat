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
    std::string GetValue(const std::string& section, const std::string& key) const;

private:
    memochat::runtime::IniConfig _config;
};
