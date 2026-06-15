#pragma once

#include "runtime/IniConfig.h"

// VarifyServer owns its own thin ConfigMgr (wrapping common/runtime/IniConfig),
// mirroring the ChatServer pattern. This severs VarifyServer's only coupling to
// GateServerCore — VarifyServer no longer links the entire gate-core static lib
// just to read an ini file. See .ai/gateserver-microservice-split/a/contracts.md.

using SectionInfo = memochat::runtime::IniSection;

class ConfigMgr
{
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
