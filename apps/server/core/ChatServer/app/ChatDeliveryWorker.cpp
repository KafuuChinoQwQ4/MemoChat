#include "ChatRuntime.h"
#include "ConfigMgr.h"
#include "LogicSystem.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "SnowflakeUtil.h"
#include "cluster/ChatClusterDiscovery.h"
#include "const.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
std::string ParseConfigPath(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--config")
        {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("missing value for --config");
            }
            return argv[++i];
        }
        throw std::runtime_error("unknown argument: " + arg);
    }
    return "";
}

void SetInstanceNameEnv(const std::string& instance_name)
{
    if (instance_name.empty())
    {
        return;
    }
#ifdef _WIN32
    _putenv_s("MEMOCHAT_INSTANCE_NAME", instance_name.c_str());
#else
    setenv("MEMOCHAT_INSTANCE_NAME", instance_name.c_str(), 1);
#endif
}

void InitSnowflake(ConfigMgr& cfg)
{
    const auto datacenter_id_str = cfg.GetValue("Snowflake", "DatacenterId");
    const auto worker_id_str = cfg.GetValue("Snowflake", "WorkerId");
    const int64_t datacenter_id = datacenter_id_str.empty() ? 1 : std::stoll(datacenter_id_str);
    const int64_t worker_id = worker_id_str.empty() ? 1 : std::stoll(worker_id_str);
    SnowflakeUtil::getInstance().init(worker_id, datacenter_id);
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        ConfigMgr::InitConfigPath(ParseConfigPath(argc, argv));
        auto& cfg = ConfigMgr::Inst();

        const std::string worker_name = memochat::cluster::ResolveSelfNodeName(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            });
        if (worker_name.empty())
        {
            throw std::runtime_error("chat delivery worker node name is empty");
        }
        SetInstanceNameEnv(worker_name);

        InitSnowflake(cfg);

        const auto log_cfg = memolog::LogConfig::FromGetter(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            });
        const auto telemetry_cfg = memolog::TelemetryConfig::FromGetter(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            });
        memolog::Logger::Init("ChatDeliveryWorker", log_cfg);
        memolog::Telemetry::Init("ChatDeliveryWorker", telemetry_cfg);

        const auto storage_init_start = std::chrono::steady_clock::now();
        PostgresMgr::GetInstance();
        memolog::LogInfo("delivery_worker.postgresql_ready",
                         "ChatDeliveryWorker postgresql ready",
                         {{"name", worker_name},
                          {"storage_init_ms",
                           std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::steady_clock::now() - storage_init_start)
                                              .count())}});

        Defer cleanup(
            [worker_name]()
            {
                RedisMgr::GetInstance()->DelCount(worker_name);
                RedisMgr::GetInstance()->Close();
                memolog::Telemetry::Shutdown();
                memolog::Logger::Shutdown();
            });

        auto logic = LogicSystem::GetInstance();
        memolog::LogInfo(
            "delivery_worker.start",
            "ChatDeliveryWorker started",
            {{"name", worker_name}, {"worker_enabled", memochat::chatruntime::IsWorkerEnabled() ? "true" : "false"}});

        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context
#ifdef _WIN32
                                        ,
                                        SIGINT,
                                        SIGBREAK
#else
                                        ,
                                        SIGINT,
                                        SIGTERM
#endif
        );
        signals.async_wait(
            [&io_context, logic](auto, auto)
            {
                io_context.stop();
            });

        io_context.run();
        memolog::LogInfo("delivery_worker.stop", "ChatDeliveryWorker stopped", {{"name", worker_name}});
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ChatDeliveryWorker fatal: " << e.what() << std::endl;
        memolog::LogError("delivery_worker.fatal", "ChatDeliveryWorker crashed", {{"error", e.what()}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
}
