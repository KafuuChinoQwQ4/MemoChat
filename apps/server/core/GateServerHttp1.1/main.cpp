#include "H1Globals.h"
#include "H1Listener.h"
#include "H1LogicSystem.h"
#include "H1WorkerPool.h"

#include "AsioIOServicePool.h"
#include "ConfigMgr.h"
#include "GateAsyncSideEffects.h"
#include "GateGlobals.h"
#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "SnowflakeUtil.h"
#include "const.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"

#include <aws/core/Aws.h>
#include <hiredis/hiredis.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

int main()
{
    Aws::SDKOptions aws_options;
    bool aws_inited = false;

    try {
        auto& gCfgMgr = ConfigMgr::Inst();

        auto log_cfg = memolog::LogConfig::FromGetter(
            [&gCfgMgr](const std::string& section, const std::string& key) {
                return gCfgMgr.GetValue(section, key);
            });
        auto telemetry_cfg = memolog::TelemetryConfig::FromGetter(
            [&gCfgMgr](const std::string& section, const std::string& key) {
                return gCfgMgr.GetValue(section, key);
            });

        memolog::Logger::Init("GateServerHttp1.1", log_cfg);
        memolog::Telemetry::Init("GateServerHttp1.1", telemetry_cfg);

        Aws::InitAPI(aws_options);
        aws_inited = true;

        const auto postgres_init_start = std::chrono::steady_clock::now();
        PostgresMgr::GetInstance();
        memolog::LogInfo("service.postgres_ready", "GateServerHttp1.1 postgres ready",
            {{"postgres_init_ms",
                std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - postgres_init_start).count())}});

        RedisMgr::GetInstance();
        MongoMgr::GetInstance();

        const auto worker_id_str = gCfgMgr.GetValue("Snowflake", "WorkerId");
        const auto datacenter_id_str = gCfgMgr.GetValue("Snowflake", "DatacenterId");
        const int64_t worker_id = worker_id_str.empty() ? 0 : std::stoll(worker_id_str);
        const int64_t datacenter_id = datacenter_id_str.empty() ? 0 : std::stoll(datacenter_id_str);
        SnowflakeUtil::getInstance().init(worker_id, datacenter_id);

        GateAsyncSideEffects::Instance().Start();

        const std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
        unsigned short gate_port = static_cast<unsigned short>(std::atoi(gate_port_str.c_str()));
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads < 2) {
            num_threads = 4;
        }

        const auto worker_threads_str = gCfgMgr.GetValue("GateServer", "WorkerThreads");
        unsigned int worker_threads = worker_threads_str.empty() ? num_threads : std::atoi(worker_threads_str.c_str());
        if (worker_threads < 1) {
            worker_threads = num_threads;
        }
        h1globals::g_worker_threads = worker_threads;
        H1WorkerPool::GetInstance(worker_threads);

        memolog::LogInfo("gate.thread_pool", "GateServerHttp1.1 thread pool configured",
            {{"num_threads", std::to_string(num_threads)}, {"worker_threads", std::to_string(worker_threads)}});

        net::io_context ioc{static_cast<int>(num_threads)};
        h1globals::g_main_ioc = &ioc;

        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int) {
            if (!error) {
                ioc.stop();
            }
        });

        H1LogicSystem::GetInstance();
        std::make_shared<H1Listener>(ioc, gate_port)->Start();

        memolog::LogInfo("service.start", "GateServerHttp1.1 listening",
            {{"port", std::to_string(gate_port)}, {"protocol", "HTTP/1.1"}});

        ioc.run();

        memolog::LogInfo("service.stop", "GateServerHttp1.1 stopped");
        GateAsyncSideEffects::Instance().Stop();
        H1WorkerPool::GetInstance()->Stop();
        RedisMgr::GetInstance()->Close();
        Aws::ShutdownAPI(aws_options);
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        memolog::LogError("service.fatal", "GateServerHttp1.1 crashed", {{"error", e.what()}});
        GateAsyncSideEffects::Instance().Stop();
        RedisMgr::GetInstance()->Close();
        if (aws_inited) {
            Aws::ShutdownAPI(aws_options);
        }
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
}
