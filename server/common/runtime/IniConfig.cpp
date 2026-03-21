#include "runtime/IniConfig.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
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

std::string IniConfig::SanitizeEnvToken(const std::string& raw) {
    std::string token;
    token.reserve(raw.size());
    for (unsigned char ch : raw) {
        if (std::isalnum(ch)) {
            token.push_back(static_cast<char>(std::toupper(ch)));
        } else {
            token.push_back('_');
        }
    }
    return token;
}

std::string IniConfig::EnvKeyFor(const std::string& section, const std::string& key) {
    return "MEMOCHAT_" + SanitizeEnvToken(section) + "_" + SanitizeEnvToken(key);
}

IniConfig::IniConfig(const std::string& configPath) {
    const auto resolvedPath = ResolvePath(configPath);
    std::cout << "Config path: " << resolvedPath << std::endl;

    boost::property_tree::ptree pt;
    try {
        boost::property_tree::read_ini(resolvedPath, pt);
    } catch (const boost::property_tree::ini_parser_error& e) {
        std::cerr << "[IniConfig] Failed to parse config file '" << resolvedPath
                  << "': " << e.what() << std::endl;
        return;
    } catch (const std::exception& e) {
        std::cerr << "[IniConfig] Error reading config file '" << resolvedPath
                  << "': " << e.what() << std::endl;
        return;
    }

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
    const auto env_key = EnvKeyFor(section, key);
    if (const char* env_value = std::getenv(env_key.c_str())) {
        return std::string(env_value);
    }

    const auto it = _config.find(section);
    if (it == _config.end()) {
        return "";
    }
    return it->second.GetValue(key);
}

} // namespace memochat::runtime
