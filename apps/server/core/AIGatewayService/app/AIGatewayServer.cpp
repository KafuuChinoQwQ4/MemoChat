#ifdef _WIN32
#if defined(_MSC_VER) && !defined(__clang__)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif
#endif

#include "../../common/WinsockCompat.h"

#include "CServer.h"
#include "ConfigMgr.h"
#include "GateGlobals.h"
#include "GateWorkerPool.h"
#include "LogicSystem.h"
#include "AsioIOServicePool.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <string>
#include <thread>

// AIGatewayServer — the first microservice peeled off GateServer (Phase 3 of the
// gateserver split). It reuses the shared GateAppCore but selects the AIGateway
// route profile, so it serves ONLY /healthz, /readyz and /ai/* and proxies to
// AIServer/AIOrchestrator. It owns no database, so unlike GateServer it does not
// initialize Postgres/Mongo/AWS — only the H1 listener + worker pool the AI
// proxy + SSE streaming need. It starts by default after Envoy cut-over.

using namespace memolog;

namespace
{
constexpr unsigned short kDefaultAIGatewayPort = 8093;
} // namespace

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    auto& cfgMgr = ConfigMgr::Inst();
    memolog::Logger::Init("AIGatewayServer",
                          memolog::LogConfig::FromGetter(
                              [&cfgMgr](const std::string& section, const std::string& key)
                              {
                                  return cfgMgr.GetValue(section, key);
                              }));

    memolog::LogInfo("aigateway.start", "AIGatewayServer starting...");

    // Serve only health + /ai/* routes. MUST be set before LogicSystem::Instance().
    LogicSystem::SetRouteProfile(LogicSystem::RouteProfile::AIGateway);

    std::string port_str = cfgMgr["AIGateway"]["Port"];
    unsigned short gate_port =
        port_str.empty() ? kDefaultAIGatewayPort : static_cast<unsigned short>(std::atoi(port_str.c_str()));

    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads < 2)
        num_threads = 4;
    auto worker_threads_str = cfgMgr.GetValue("AIGateway", "WorkerThreads");
    unsigned int worker_threads = worker_threads_str.empty() ? num_threads : std::atoi(worker_threads_str.c_str());
    if (worker_threads < 1)
        worker_threads = num_threads;

    gateglobals::g_worker_threads = worker_threads;
    gateglobals::g_main_ioc = nullptr;
    GateWorkerPool::GetInstance(worker_threads);

    memolog::LogInfo(
        "aigateway.thread_pool",
        "AIGatewayServer thread pool configured",
        {{"num_threads", std::to_string(num_threads)}, {"worker_threads", std::to_string(worker_threads)}});

    // Force LogicSystem construction now (registers the AIGateway route profile).
    LogicSystem::GetInstance();

    net::io_context ioc{static_cast<int>(num_threads)};
    gateglobals::g_main_ioc = &ioc;
    std::make_shared<CServer>(ioc, gate_port)->Start();
    memolog::LogInfo("service.start", "AIGatewayServer listening", {{"port", std::to_string(gate_port)}});

    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&ioc](const boost::system::error_code& error, int signal_number)
        {
            (void) signal_number;
            if (error)
            {
                return;
            }
            memolog::LogInfo("aigateway.signal", "AIGatewayServer received shutdown signal");
            ioc.stop();
        });

    ioc.run();

    memolog::LogInfo("service.stop", "AIGatewayServer stopped");
    GateWorkerPool::GetInstance()->Stop();
    memolog::Logger::Shutdown();
    return 0;
}
