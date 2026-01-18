#include "ConfigMgr.h"

ConfigMgr gCfgMgr; // 全局实例

ConfigMgr::ConfigMgr() {
    // 获取当前路径下的 config.ini
    boost::filesystem::path current_path = boost::filesystem::current_path();
    boost::filesystem::path config_path = current_path / "config.ini";
    std::cout << "Config path: " << config_path << std::endl;

    if (!boost::filesystem::exists(config_path)) {
        std::cerr << "Config file not found!" << std::endl;
        return;
    }

    boost::property_tree::ptree pt;
    try {
        boost::property_tree::read_ini(config_path.string(), pt);
    } catch (std::exception& e) {
        std::cerr << "Read ini error: " << e.what() << std::endl;
        return;
    }

    for (const auto& section_pair : pt) {
        const std::string& section_name = section_pair.first;
        const boost::property_tree::ptree& section_tree = section_pair.second;

        std::map<std::string, std::string> section_config;
        for (const auto& key_value_pair : section_tree) {
            const std::string& key = key_value_pair.first;
            const std::string& value = key_value_pair.second.get_value<std::string>();
            section_config[key] = value;
        }
        SectionInfo sectionInfo;
        sectionInfo._section_datas = section_config;
        _config_map[section_name] = sectionInfo;
    }
}

SectionInfo ConfigMgr::operator[](const std::string& section) {
    if (_config_map.find(section) == _config_map.end()) {
        return SectionInfo();
    }
    return _config_map[section];
}