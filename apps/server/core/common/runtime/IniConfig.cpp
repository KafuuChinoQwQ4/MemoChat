#include "runtime/IniConfig.h"

#include <boost/filesystem.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace memochat::runtime {
namespace {

std::string Trim(std::string value) {
    auto is_space = [](unsigned char ch) {
        return std::isspace(ch) != 0;
    };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char ch) {
        return !is_space(static_cast<unsigned char>(ch));
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](char ch) {
        return !is_space(static_cast<unsigned char>(ch));
    }).base(), value.end());
    return value;
}

void StripUtf8Bom(std::string& line) {
    if (line.size() >= 3 &&
        static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB &&
        static_cast<unsigned char>(line[2]) == 0xBF) {
        line.erase(0, 3);
    }
}

bool LoadIniFile(const std::string& path, std::map<std::string, IniSection>& config) {
    std::ifstream input(path);
    if (!input.is_open()) {
        std::cerr << "[IniConfig] Failed to open config file '" << path << "'" << std::endl;
        return false;
    }

    std::string current_section;
    std::string line;
    size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (line_number == 1) {
            StripUtf8Bom(line);
        }

        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            current_section = Trim(line.substr(1, line.size() - 2));
            if (current_section.empty()) {
                std::cerr << "[IniConfig] Empty section in config file '" << path
                          << "' at line " << line_number << std::endl;
                return false;
            }
            config.try_emplace(current_section);
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            std::cerr << "[IniConfig] Invalid config line in '" << path
                      << "' at line " << line_number << ": '=' character not found" << std::endl;
            return false;
        }

        if (current_section.empty()) {
            std::cerr << "[IniConfig] Key outside section in config file '" << path
                      << "' at line " << line_number << std::endl;
            return false;
        }

        auto key = Trim(line.substr(0, equals));
        auto value = Trim(line.substr(equals + 1));
        if (key.empty()) {
            std::cerr << "[IniConfig] Empty key in config file '" << path
                      << "' at line " << line_number << std::endl;
            return false;
        }

        config[current_section].values[key] = value;
    }

    return true;
}

} // namespace

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

    try {
        if (!LoadIniFile(resolvedPath, _config)) {
            return;
        }
    } catch (const std::exception& e) {
        std::cerr << "[IniConfig] Error loading config file '" << resolvedPath
                  << "': " << e.what() << std::endl;
        return;
    }

    std::cout << "[IniConfig] Loaded " << _config.size()
              << " sections from " << resolvedPath << std::endl;
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
        if (_etcd_config) {
            std::string etcd_value;
            if (_etcd_config->Get(section, key, etcd_value)) {
                return etcd_value;
            }
        }
        return "";
    }
    return it->second.GetValue(key);
}

bool IniConfig::TryLoadFromEtcd(const std::string& endpoints, const std::string& service_name) {
    _etcd_config = EtcdConfigLoader::TryCreate(endpoints, service_name);
    if (!_etcd_config) {
        std::cout << "[IniConfig] etcd not available, using INI file only" << std::endl;
        return false;
    }

    std::cout << "[IniConfig] etcd available, loading config from etcd" << std::endl;

    _etcd_config->SetChangeCallback([this](const std::string& section, const std::string& key, const std::string& value) {
        OnEtcdConfigChange(section, key, value);
    });

    return true;
}

void IniConfig::StartEtcdWatch() {
    if (_etcd_config) {
        _etcd_config->StartWatch();
    }
}

void IniConfig::StopEtcdWatch() {
    if (_etcd_config) {
        _etcd_config->Stop();
    }
}

void IniConfig::OnEtcdConfigChange(const std::string& section, const std::string& key, const std::string& value) {
    std::cout << "[IniConfig] Config changed: [" << section << "] " << key << " = " << value << std::endl;
    _config[section].values[key] = value;
}

} // namespace memochat::runtime
