#pragma once
#include <string>
#include <map>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>

struct SectionInfo {
    SectionInfo() {}
    ~SectionInfo() { _section_datas.clear(); }
    SectionInfo(const SectionInfo& src) { _section_datas = src._section_datas; }
    SectionInfo& operator = (const SectionInfo& src) {
        if (&src == this) return *this;
        this->_section_datas = src._section_datas;
        return *this;
    }

    std::map<std::string, std::string> _section_datas;
    std::string operator[](const std::string& key) {
        if (_section_datas.find(key) == _section_datas.end()) return "";
        return _section_datas[key];
    }
};

class ConfigMgr {
public:
    ConfigMgr();
    SectionInfo operator[](const std::string& section);
private:
    std::map<std::string, SectionInfo> _config_map;
};

// 全局声明
extern ConfigMgr gCfgMgr;