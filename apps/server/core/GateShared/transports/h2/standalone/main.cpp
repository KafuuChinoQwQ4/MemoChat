#include "WinCompat.h"
#include "GateServerHttp2.h"
#include "ConfigMgr.h"
#include "GateAsyncSideEffects.h"
#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "SnowflakeUtil.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"

#include <aws/core/Aws.h>

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

static volatile bool g_running = true;

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    Aws::SDKOptions aws_options;
    bool aws_inited = false;

    std::signal(SIGINT,
                [](int)
                {
                    std::cout << "GateServerHttp2: SIGINT received, shutting down..." << std::endl;
                    g_running = false;
                });
    std::signal(SIGTERM,
                [](int)
                {
                    std::cout << "GateServerHttp2: SIGTERM received, shutting down..." << std::endl;
                    g_running = false;
                });

    try
    {
        auto& gCfgMgr = ConfigMgr::Inst();
        auto log_cfg = memolog::LogConfig::FromGetter(
            [&gCfgMgr](const std::string& section, const std::string& key)
            {
                return gCfgMgr.GetValue(section, key);
            });
        auto telemetry_cfg = memolog::TelemetryConfig::FromGetter(
            [&gCfgMgr](const std::string& section, const std::string& key)
            {
                return gCfgMgr.GetValue(section, key);
            });

        memolog::Logger::Init("GateServerHttp2", log_cfg);
        memolog::Telemetry::Init("GateServerHttp2", telemetry_cfg);

        Aws::InitAPI(aws_options);
        aws_inited = true;

        const auto postgres_init_start = std::chrono::steady_clock::now();
        PostgresMgr::GetInstance();
        memolog::LogInfo("service.postgres_ready",
                         "GateServerHttp2 postgres ready",
                         {{"postgres_init_ms",
                           std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::steady_clock::now() - postgres_init_start)
                                              .count())}});

        RedisMgr::GetInstance();
        MongoMgr::GetInstance();

        {
            auto worker_id_str = gCfgMgr.GetValue("Snowflake", "WorkerId");
            auto datacenter_id_str = gCfgMgr.GetValue("Snowflake", "DatacenterId");
            int64_t worker_id = worker_id_str.empty() ? 0 : std::stoll(worker_id_str);
            int64_t datacenter_id = datacenter_id_str.empty() ? 0 : std::stoll(datacenter_id_str);
            SnowflakeUtil::getInstance().init(worker_id, datacenter_id);
        }

        GateAsyncSideEffects::Instance().Start();
        memolog::LogInfo("service.init", "GateServerHttp2 core services initialized", {});

        GateServerHttp2::GetInstance()->Initialize();
        GateServerHttp2::GetInstance()->Run();

        memolog::LogInfo("service.stop", "GateServerHttp2 stopped");
        GateAsyncSideEffects::Instance().Stop();
        RedisMgr::GetInstance()->Close();
        Aws::ShutdownAPI(aws_options);
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_SUCCESS;
    }
    catch (std::exception const& e)
    {
        memolog::LogError("service.fatal", "GateServerHttp2 crashed", {{"error", e.what()}});
        GateAsyncSideEffects::Instance().Stop();
        RedisMgr::GetInstance()->Close();
        if (aws_inited)
        {
            Aws::ShutdownAPI(aws_options);
        }
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
}
