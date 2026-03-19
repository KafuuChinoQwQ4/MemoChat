#include "DrogonAppCfg.h"
#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>

using namespace drogon;

void DrogonAppCfg::Configure()
{
    // Configure Drogon with default settings
    app().addListener("0.0.0.0", 8081);
    app().setThreadNum(4);
    app().setUploadPath("./uploads");
    // Note: setSessionTimeout, setKeepaliveRequests, setKeepaliveTimeout are not available
    // in this version of Drogon - these settings should be in config.json instead
    app().setLogLevel(trantor::Logger::LogLevel::kInfo);
    app().setMaxConnectionNum(100000);
}

void DrogonAppCfg::ConfigureFromConfig(const std::string& host, int port, int threads)
{
    app().addListener(host, port);
    app().setThreadNum(threads);
    app().setUploadPath("./uploads");
    app().setLogLevel(trantor::Logger::LogLevel::kInfo);
    app().setMaxConnectionNum(100000);
}
