#include "ChatRuntime.hpp"
#include "ConfigMgr.hpp"
#include "LogicSystem.hpp"
#include "PostgresMgr.hpp"
#include "RedisMgr.hpp"
#include "SnowflakeUtil.hpp"
#include "cluster/ChatClusterDiscovery.hpp"
#include "const.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

import memochat.chat.delivery_worker_runtime_algorithms;

namespace delivery_worker_modules = memochat::chat::delivery_worker::modules;

namespace
{
bool ParseConfigPath(int argc, char** argv, std::string& path, std::string& error)
{
    path.clear();
    error.clear();
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (delivery_worker_modules::IsConfigFlag(arg.data(), arg.size()))
        {
            if (delivery_worker_modules::ShouldRejectMissingConfigValue(i + 1 < argc))
            {
                error = delivery_worker_modules::MissingConfigValueMessage();
                return false;
            }
            path = argv[++i];
            continue;
        }
        error = std::string(delivery_worker_modules::UnknownArgumentPrefix()) + arg;
        return false;
    }
    return true;
}

void SetInstanceNameEnv(const std::string& instance_name)
{
    if (!delivery_worker_modules::ShouldSetInstanceName(instance_name.empty()))
    {
        return;
    }
#ifdef _WIN32
    _putenv_s("MEMOCHAT_INSTANCE_NAME", instance_name.c_str());
#else
    setenv("MEMOCHAT_INSTANCE_NAME", instance_name.c_str(), 1);
#endif
}

bool ParseInt64Or(const std::string& raw, int64_t fallback, int64_t& value)
{
    if (raw.empty())
    {
        value = fallback;
        return true;
    }
    const auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == raw.data() + raw.size();
}

bool InitSnowflake(ConfigMgr& cfg, std::string& error)
{
    const auto datacenter_id_str = cfg.GetValue("Snowflake", "DatacenterId");
    const auto worker_id_str = cfg.GetValue("Snowflake", "WorkerId");
    int64_t datacenter_id = 0;
    int64_t worker_id = 0;
    if (!ParseInt64Or(datacenter_id_str, delivery_worker_modules::DefaultSnowflakeDatacenterId(), datacenter_id) ||
        !ParseInt64Or(worker_id_str, delivery_worker_modules::DefaultSnowflakeWorkerId(), worker_id))
    {
        error = "invalid Snowflake numeric configuration";
        return false;
    }
    if (!SnowflakeUtil::getInstance().init(worker_id, datacenter_id))
    {
        error = "Snowflake WorkerId and DatacenterId must be in range 0..31";
        return false;
    }
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    std::string config_path;
    std::string startup_error;
    if (!ParseConfigPath(argc, argv, config_path, startup_error))
    {
        std::cerr << "ChatDeliveryWorker fatal: " << startup_error << std::endl;
        return EXIT_FAILURE;
    }
    ConfigMgr::InitConfigPath(config_path);
    auto& cfg = ConfigMgr::Inst();

    const std::string worker_name = memochat::cluster::ResolveSelfNodeName(
        [&cfg](const std::string& section, const std::string& key)
        {
            return cfg.GetValue(section, key);
        });
    if (delivery_worker_modules::ShouldRejectEmptyWorkerName(worker_name.empty()))
    {
        std::cerr << "ChatDeliveryWorker fatal: " << delivery_worker_modules::EmptyWorkerNameMessage() << std::endl;
        return EXIT_FAILURE;
    }
    SetInstanceNameEnv(worker_name);

    if (!InitSnowflake(cfg, startup_error))
    {
        std::cerr << "ChatDeliveryWorker fatal: " << startup_error << std::endl;
        return EXIT_FAILURE;
    }

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
    if (!memolog::Logger::Init(delivery_worker_modules::LoggerName(), log_cfg, &startup_error))
    {
        std::cerr << "ChatDeliveryWorker fatal: " << startup_error << std::endl;
        return EXIT_FAILURE;
    }
    memolog::Telemetry::Init(delivery_worker_modules::LoggerName(), telemetry_cfg);

    const auto storage_init_start = std::chrono::steady_clock::now();
    auto postgres_mgr = PostgresMgr::GetInstance();
    if (!postgres_mgr->Ready())
    {
        memolog::LogError("delivery_worker.postgresql_init_failed",
                          "ChatDeliveryWorker PostgreSQL initialization failed",
                          {{"error", postgres_mgr->startupError()}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
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
    if (!logic->Ready())
    {
        memolog::LogError("delivery_worker.start_failed",
                          "ChatDeliveryWorker runtime composition failed",
                          {{"error", logic->startupError()}});
        return EXIT_FAILURE;
    }
    memolog::LogInfo(
        "delivery_worker.start",
        "ChatDeliveryWorker started",
        {{"name", worker_name},
         {"worker_enabled", delivery_worker_modules::WorkerEnabledText(memochat::chatruntime::IsWorkerEnabled())}});

    boost::asio::io_context io_context;
    boost::asio::signal_set signals(io_context);
    boost::system::error_code signal_error;
#ifdef _WIN32
    signals.add(SIGINT, signal_error);
    if (!signal_error)
    {
        signals.add(SIGBREAK, signal_error);
    }
#else
    signals.add(SIGINT, signal_error);
    if (!signal_error)
    {
        signals.add(SIGTERM, signal_error);
    }
#endif
    if (signal_error)
    {
        memolog::LogError("delivery_worker.signal_setup_failed",
                          "ChatDeliveryWorker failed to register shutdown signals",
                          {{"error", signal_error.message()}});
        return EXIT_FAILURE;
    }
    signals.async_wait(
        [&io_context, logic](auto, auto)
        {
            io_context.stop();
        });

    io_context.run();
    memolog::LogInfo("delivery_worker.stop", "ChatDeliveryWorker stopped", {{"name", worker_name}});
    return 0;
}
