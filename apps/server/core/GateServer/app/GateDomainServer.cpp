#ifdef _WIN32
#if defined(_MSC_VER) && !defined(__clang__)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif
#endif

#include "../../common/WinsockCompat.h"

#include "GateDomainServer.h"

#include "CServer.h"
#include "ConfigMgr.h"
#include "GateAsyncSideEffects.h"
#include "GateGlobals.h"
#include "GateWorkerPool.h"
#include "SnowflakeUtil.h"
#include "AsioIOServicePool.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include <aws/core/Aws.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <string>
#include <thread>

using namespace memolog;

int RunGateDomainServer(LogicSystem::RouteProfile profile,
                        const std::string& service_name,
                        const std::string& config_section,
                        unsigned short default_port,
                        bool init_aws,
                        bool init_async)
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

    memolog::LogInfo("gatedomain.start", service_name + " starting...");

    // Select the domain route profile BEFORE the first LogicSystem::Instance().
    LogicSystem::SetRouteProfile(profile);

    // Account services (register/login/account) emit audit + cache-invalidate
    // side effects through GateAsyncSideEffects; start it when requested.
    if (init_async)
    {
        GateAsyncSideEffects::Instance().Start();
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
    unsigned short port = port_str.empty() ? default_port : static_cast<unsigned short>(std::atoi(port_str.c_str()));

    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads < 2)
        num_threads = 4;
    auto worker_threads_str = cfgMgr.GetValue(config_section, "WorkerThreads");
    unsigned int worker_threads = worker_threads_str.empty() ? num_threads : std::atoi(worker_threads_str.c_str());
    if (worker_threads < 1)
        worker_threads = num_threads;

    gateglobals::g_worker_threads = worker_threads;
    gateglobals::g_main_ioc = nullptr;
    GateWorkerPool::GetInstance(worker_threads);

    LogicSystem::GetInstance(); // build registry with the selected profile

    net::io_context ioc{static_cast<int>(num_threads)};
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
    if (init_async)
    {
        GateAsyncSideEffects::Instance().Stop();
    }
    GateWorkerPool::GetInstance()->Stop();
    if (init_aws)
    {
        Aws::ShutdownAPI(aws_options);
    }
    memolog::Logger::Shutdown();
    return 0;
}
