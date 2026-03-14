#include "runtime/IniConfig.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

namespace memochat::runtime {

std::string IniSection::operator[](const std::string& key) const {
    return GetValue(key);
}

std::string IniSection::GetValue(const std::string& key) const {
    const auto it = values.find(key);
    if (it == values.end()) {
        return "";
    }
    return it->second;
}

std::string IniConfig::ResolvePath(const std::string& overridePath) {
    boost::filesystem::path currentPath = boost::filesystem::current_path();
    boost::filesystem::path configPath = overridePath.empty()
        ? (currentPath / "config.ini")
        : boost::filesystem::path(overridePath);
    return configPath.string();
}

IniConfig::IniConfig(const std::string& configPath) {
    const auto resolvedPath = ResolvePath(configPath);
    std::cout << "Config path: " << resolvedPath << std::endl;

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(resolvedPath, pt);

    for (const auto& sectionPair : pt) {
        IniSection section;
        for (const auto& keyValuePair : sectionPair.second) {
            section.values[keyValuePair.first] = keyValuePair.second.get_value<std::string>();
        }
        _config[sectionPair.first] = section;
    }

    for (const auto& [sectionName, section] : _config) {
        std::cout << "[" << sectionName << "]" << std::endl;
        for (const auto& [key, value] : section.values) {
            std::cout << key << "=" << value << std::endl;
        }
    }
}

IniSection IniConfig::operator[](const std::string& section) const {
    const auto it = _config.find(section);
    if (it == _config.end()) {
        return IniSection();
    }
    return it->second;
}

std::string IniConfig::GetValue(const std::string& section, const std::string& key) const {
    const auto it = _config.find(section);
    if (it == _config.end()) {
        return "";
    }
    return it->second.GetValue(key);
}

} // namespace memochat::runtime
