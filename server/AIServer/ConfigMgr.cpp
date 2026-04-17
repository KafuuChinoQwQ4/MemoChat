#include "ConfigMgr.h"

ConfigMgr::ConfigMgr()
    : _config() {
}

ConfigMgr& ConfigMgr::Inst() {
    static ConfigMgr cfg_mgr;
    return cfg_mgr;
}

SectionInfo ConfigMgr::operator[](const std::string& section) const {
    return _config[section];
}

std::string ConfigMgr::GetValue(const std::string& section, const std::string& key) const {
    return _config.GetValue(section, key);
}
