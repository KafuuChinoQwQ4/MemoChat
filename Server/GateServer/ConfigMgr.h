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
    ~ConfigMgr() { _config_map.clear(); }

    // 1. 获取单例实例的静态方法
    static ConfigMgr& Inst() {
        static ConfigMgr instance;
        return instance;
    }

    SectionInfo operator[](const std::string& section);

    // 2. 删除拷贝构造和赋值，防止复制单例
    ConfigMgr(const ConfigMgr&) = delete;
    ConfigMgr& operator=(const ConfigMgr&) = delete;

    std::string GetValue(const std::string& section, const std::string& key);

private:
    // 3. 构造函数私有化
    ConfigMgr(); 

    std::map<std::string, SectionInfo> _config_map;
};

// 4. 删除这一行，不再需要全局变量声明
// extern ConfigMgr gCfgMgr;