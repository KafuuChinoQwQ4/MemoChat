#pragma once

#include <string>

class DrogonAppCfg
{
public:
    static void Configure();
    static void ConfigureFromConfig(const std::string& host, int port, int threads);
};
