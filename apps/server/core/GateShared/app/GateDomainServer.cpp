#ifdef _WIN32
#if defined(_MSC_VER) && !defined(__clang__)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif
#endif

#include "../../common/WinsockCompat.hpp"

#include "GateDomainServer.hpp"

#include "CServer.hpp"
#include "ConfigMgr.hpp"
#include "GateDomainRuntime.hpp"
#include "GateGlobals.hpp"
#include "GateWorkerPool.hpp"
#include "LogicSystem.hpp"
#include "MongoMgr.hpp"
#include "PostgresMgr.hpp"
#include "RedisMgr.hpp"
#include "SnowflakeUtil.hpp"
#include "AsioIOServicePool.hpp"
#include "auth/AuthSecret.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"
#include "runtime/ExplicitThread.hpp"
#include <aws/core/Aws.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <charconv>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

using namespace memolog;

namespace
{

bool ParseInt64OrDefault(std::string_view raw, int64_t default_value, int64_t* out)
{
    if (out == nullptr)
    {
        return false;
    }
    if (raw.empty())
    {
        *out = default_value;
        return true;
    }
    int64_t value = 0;
    const auto [ptr, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    if (ec != std::errc{} || ptr != raw.data() + raw.size())
    {
        return false;
    }
    *out = value;
    return true;
}

bool CheckGateDomainDependencies(const GateDomainDependencies& dependencies, std::string* error)
{
    if (dependencies.postgres)
    {
        const auto postgres = PostgresMgr::GetInstance();
        if (!postgres->Ready())
        {
            if (error != nullptr)
            {
                *error = "Postgres: " + postgres->StartupError();
            }
            return false;
        }
    }
    if (dependencies.redis)
    {
        const auto redis = RedisMgr::GetInstance();
        if (!redis->Ready())
        {
            if (error != nullptr)
            {
                *error = "Redis: " + redis->StartupError();
            }
            return false;
        }
    }
    if (dependencies.mongo)
    {
        const auto mongo = MongoMgr::GetInstance();
        if (!mongo->Ready())
        {
            if (error != nullptr)
            {
                *error = "Mongo: " + mongo->StartupError();
            }
            return false;
        }
    }
    return true;
}

} // namespace

int RunGateDomainServer(GateDomainRouteRegistrar registrar,
                        const std::string& service_name,
                        const std::string& config_section,
                        unsigned short default_port,
                        bool init_aws,
                        GateDomainStartHook on_start,
                        GateDomainLifecycleHook on_stop,
                        GateDomainDependencies dependencies)
{
    Aws::SDKOptions aws_options;
    bool aws_initialized = false;

    auto& cfgMgr = ConfigMgr::Inst();
    const auto log_config = memolog::LogConfig::FromGetter(
        [&cfgMgr](const std::string& section, const std::string& key)
        {
            return cfgMgr.GetValue(section, key);
        });
    std::string logger_error;
    if (!memolog::Logger::Init(service_name, log_config, &logger_error))
    {
        std::cerr << service_name << " fatal: " << logger_error << std::endl;
        return 1;
    }
    memolog::Telemetry::Init(service_name,
                             memolog::TelemetryConfig::FromGetter(
                                 [&cfgMgr](const std::string& section, const std::string& key)
                                 {
                                     return cfgMgr.GetValue(section, key);
                                 }));

    memolog::LogInfo("gatedomain.start", service_name + " starting...");
    auto chat_auth_secret = cfgMgr.GetValue("ChatAuth", "HmacSecret");
    if (chat_auth_secret.empty())
    {
        chat_auth_secret = std::string(memochat::auth::kWellKnownDevHmacSecret);
    }
    std::string startup_error;
    if (!memochat::auth::RequireNonDefaultChatAuthSecretInProduction(service_name, chat_auth_secret, startup_error))
    {
        memolog::LogError("gatedomain.config_invalid", "invalid chat auth configuration", {{"error", startup_error}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return 1;
    }
    auto jwt_access_secret = cfgMgr.GetValue("AuthToken", "JwtSecret");
    if (jwt_access_secret.empty())
    {
        jwt_access_secret = std::string(memochat::auth::kWellKnownDevJwtAccessSecret);
    }
    if (!memochat::auth::RequireNonDefaultJwtAccessSecretInProduction(service_name, jwt_access_secret, startup_error))
    {
        memolog::LogError("gatedomain.config_invalid", "invalid JWT access configuration", {{"error", startup_error}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return 1;
    }

    // Register the domain route profile BEFORE the first LogicSystem::Instance().
    LogicSystem::ClearRouteProfileRegistrars();
    LogicSystem::AddRouteProfileRegistrar(registrar);

    // Snowflake id generator (moments/media id paths use it; harmless otherwise).
    {
        auto worker_id_str = cfgMgr.GetValue("Snowflake", "WorkerId");
        auto datacenter_id_str = cfgMgr.GetValue("Snowflake", "DatacenterId");
        int64_t worker_id = 0;
        int64_t datacenter_id = 0;
        if (!ParseInt64OrDefault(worker_id_str, 0, &worker_id) ||
            !ParseInt64OrDefault(datacenter_id_str, 0, &datacenter_id) ||
            !SnowflakeUtil::getInstance().init(worker_id, datacenter_id))
        {
            memolog::LogError("gatedomain.config_invalid",
                              "invalid Snowflake configuration",
                              {{"worker_id", worker_id_str}, {"datacenter_id", datacenter_id_str}});
            memolog::Telemetry::Shutdown();
            memolog::Logger::Shutdown();
            return 1;
        }
    }

    if (init_aws)
    {
        Aws::InitAPI(aws_options);
        aws_initialized = true;
    }

    if (!CheckGateDomainDependencies(dependencies, &startup_error))
    {
        memolog::LogError("gatedomain.dependency_unavailable",
                          service_name + " dependency initialization failed",
                          {{"error", startup_error}});
        if (aws_initialized)
        {
            Aws::ShutdownAPI(aws_options);
        }
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return 1;
    }

    auto io_pool = AsioIOServicePool::GetInstance();
    if (!io_pool->Ready())
    {
        memolog::LogError("service.thread_pool_start_failed",
                          service_name + " I/O thread pool failed to start",
                          {{"error", io_pool->startupError()}});
        if (aws_initialized)
        {
            Aws::ShutdownAPI(aws_options);
        }
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return 1;
    }

    if (on_start)
    {
        if (!on_start(&startup_error))
        {
            memolog::LogError("gatedomain.start_hook_failed",
                              service_name + " startup hook failed",
                              {{"error", startup_error}});
            io_pool->Stop();
            if (on_stop)
            {
                on_stop();
            }
            if (aws_initialized)
            {
                Aws::ShutdownAPI(aws_options);
            }
            memolog::Telemetry::Shutdown();
            memolog::Logger::Shutdown();
            return 1;
        }
    }

    std::string port_str = cfgMgr[config_section]["Port"];
    unsigned short port = SelectGateDomainListenPort(port_str, default_port);

    unsigned int num_threads = NormalizeGateDomainIoThreads(memochat::runtime::ExplicitThread::HardwareConcurrency());
    auto worker_threads_str = cfgMgr.GetValue(config_section, "WorkerThreads");
    unsigned int worker_threads = SelectGateDomainWorkerThreads(worker_threads_str, num_threads);

    gateglobals::g_worker_threads = worker_threads;
    gateglobals::g_main_ioc = nullptr;
    auto worker_pool = GateWorkerPool::GetInstance();
    std::string worker_pool_error;
    if (!worker_pool->Start(worker_threads, &worker_pool_error))
    {
        memolog::LogError("service.worker_pool_start_failed",
                          service_name + " failed to start worker pool",
                          {{"error", worker_pool_error}});
        worker_pool->Stop();
        io_pool->Stop();
        if (on_stop)
        {
            on_stop();
        }
        if (aws_initialized)
        {
            Aws::ShutdownAPI(aws_options);
        }
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return 1;
    }

    LogicSystem::GetInstance(); // build registry with the selected profile

    net::io_context ioc{static_cast<int>(num_threads)};
    auto work_guard = boost::asio::make_work_guard(ioc);
    gateglobals::g_main_ioc = &ioc;
    auto server = std::make_shared<CServer>(ioc);
    std::string listen_error;
    if (!server->Open(port, &listen_error))
    {
        memolog::LogError("service.start_failed",
                          service_name + " failed to open listener",
                          {{"port", std::to_string(port)}, {"error", listen_error}});
        gateglobals::g_main_ioc = nullptr;
        worker_pool->Stop();
        io_pool->Stop();
        if (on_stop)
        {
            on_stop();
        }
        if (aws_initialized)
        {
            Aws::ShutdownAPI(aws_options);
        }
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return 1;
    }
    server->Start();
    memolog::LogInfo("service.start", service_name + " listening", {{"port", std::to_string(port)}});

    boost::asio::signal_set signals(ioc);
    boost::system::error_code signal_error;
    signals.add(SIGINT, signal_error);
    if (!signal_error)
    {
        signals.add(SIGTERM, signal_error);
    }
    if (signal_error)
    {
        memolog::LogError("service.signal_init_failed",
                          service_name + " failed to install shutdown signals",
                          {{"error", signal_error.message()}});
        gateglobals::g_main_ioc = nullptr;
        worker_pool->Stop();
        io_pool->Stop();
        if (on_stop)
        {
            on_stop();
        }
        if (aws_initialized)
        {
            Aws::ShutdownAPI(aws_options);
        }
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return 1;
    }
    signals.async_wait(
        [&ioc](const boost::system::error_code& error, int signal_number)
        {
            (void) signal_number;
            if (error)
            {
                return;
            }
            ioc.stop();
        });

    ioc.run();
    gateglobals::g_main_ioc = nullptr;

    memolog::LogInfo("service.stop", service_name + " stopped");
    io_pool->Stop();
    worker_pool->Stop();
    if (on_stop)
    {
        on_stop();
    }
    if (aws_initialized)
    {
        Aws::ShutdownAPI(aws_options);
    }
    memolog::Telemetry::Shutdown();
    memolog::Logger::Shutdown();
    return 0;
}
