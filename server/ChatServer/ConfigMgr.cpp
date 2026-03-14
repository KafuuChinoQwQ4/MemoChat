#include "ConfigMgr.h"

std::string& ConfigMgr::ConfigPathStorage() {
    static std::string config_path;
    return config_path;
}

ConfigMgr::ConfigMgr()
    : _config(ConfigPathStorage()) {
}

ConfigMgr& ConfigMgr::Inst() {
    static ConfigMgr cfg_mgr;
    return cfg_mgr;
}

void ConfigMgr::InitConfigPath(const std::string& config_path) {
    ConfigPathStorage() = config_path;
}

std::string ConfigMgr::GetConfigPath() {
    return ConfigPathStorage();
}

SectionInfo ConfigMgr::operator[](const std::string& section) const {
    return _config[section];
}

std::string ConfigMgr::GetValue(const std::string& section, const std::string& key) const {
    return _config.GetValue(section, key);
}
