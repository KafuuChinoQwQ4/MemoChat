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
#include "SnowflakeUtil.hpp"
#include "AsioIOServicePool.hpp"
#include "auth/AuthSecret.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"
#include <aws/core/Aws.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <string>
#include <thread>

using namespace memolog;

int RunGateDomainServer(GateDomainRouteRegistrar registrar,
                        const std::string& service_name,
                        const std::string& config_section,
                        unsigned short default_port,
                        bool init_aws,
                        GateDomainLifecycleHook on_start,
                        GateDomainLifecycleHook on_stop)
{
    Aws::SDKOptions aws_options;
    if (init_aws)
    {
        Aws::InitAPI(aws_options);
    }

    auto& cfgMgr = ConfigMgr::Inst();
    memolog::Logger::Init(service_name,
                          memolog::LogConfig::FromGetter(
                              [&cfgMgr](const std::string& section, const std::string& key)
                              {
                                  return cfgMgr.GetValue(section, key);
                              }));
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
    memochat::auth::RequireNonDefaultChatAuthSecretInProduction(service_name, chat_auth_secret);
    auto jwt_access_secret = cfgMgr.GetValue("AuthToken", "JwtSecret");
    if (jwt_access_secret.empty())
    {
        jwt_access_secret = std::string(memochat::auth::kWellKnownDevJwtAccessSecret);
    }
    memochat::auth::RequireNonDefaultJwtAccessSecretInProduction(service_name, jwt_access_secret);

    // Register the domain route profile BEFORE the first LogicSystem::Instance().
    LogicSystem::ClearRouteProfileRegistrars();
    LogicSystem::AddRouteProfileRegistrar(registrar);

    if (on_start)
    {
        on_start();
    }

    // Snowflake id generator (moments/media id paths use it; harmless otherwise).
    {
        auto worker_id_str = cfgMgr.GetValue("Snowflake", "WorkerId");
        auto datacenter_id_str = cfgMgr.GetValue("Snowflake", "DatacenterId");
        int64_t worker_id = worker_id_str.empty() ? 0 : std::stoll(worker_id_str);
        int64_t datacenter_id = datacenter_id_str.empty() ? 0 : std::stoll(datacenter_id_str);
        SnowflakeUtil::getInstance().init(worker_id, datacenter_id);
    }

    std::string port_str = cfgMgr[config_section]["Port"];
    unsigned short port = SelectGateDomainListenPort(port_str, default_port);

    unsigned int num_threads = NormalizeGateDomainIoThreads(std::thread::hardware_concurrency());
    auto worker_threads_str = cfgMgr.GetValue(config_section, "WorkerThreads");
    unsigned int worker_threads = SelectGateDomainWorkerThreads(worker_threads_str, num_threads);

    gateglobals::g_worker_threads = worker_threads;
    gateglobals::g_main_ioc = nullptr;
    GateWorkerPool::GetInstance(worker_threads);

    LogicSystem::GetInstance(); // build registry with the selected profile

    net::io_context ioc{static_cast<int>(num_threads)};
    auto work_guard = boost::asio::make_work_guard(ioc);
    gateglobals::g_main_ioc = &ioc;
    std::make_shared<CServer>(ioc, port)->Start();
    memolog::LogInfo("service.start", service_name + " listening", {{"port", std::to_string(port)}});

    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
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

    memolog::LogInfo("service.stop", service_name + " stopped");
    if (on_stop)
    {
        on_stop();
    }
    GateWorkerPool::GetInstance()->Stop();
    if (init_aws)
    {
        Aws::ShutdownAPI(aws_options);
    }
    memolog::Telemetry::Shutdown();
    memolog::Logger::Shutdown();
    return 0;
}
