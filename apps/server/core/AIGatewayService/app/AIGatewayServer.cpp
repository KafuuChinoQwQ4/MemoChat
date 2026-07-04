#ifdef _WIN32
#if defined(_MSC_VER) && !defined(__clang__)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif
#endif

#include "../../common/WinsockCompat.hpp"

#include "AIGatewayRuntime.hpp"
#include "CServer.hpp"
#include "ConfigMgr.hpp"
#include "GateGlobals.hpp"
#include "GateRouteProfileRegistrar.hpp"
#include "GateWorkerPool.hpp"
#include "LogicSystem.hpp"
#include "domain/AIHttpServiceRoutes.hpp"
#include "AsioIOServicePool.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"
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
    LogicSystem::ClearRouteProfileRegistrars();
    LogicSystem::AddRouteProfileRegistrar(memochat::gate::profiles::RegisterAIGateway);

    std::string port_str = cfgMgr["AIGateway"]["Port"];
    unsigned short gate_port = SelectAIGatewayListenPort(port_str, kDefaultAIGatewayPort);

    unsigned int num_threads = NormalizeAIGatewayIoThreads(std::thread::hardware_concurrency());
    auto worker_threads_str = cfgMgr.GetValue("AIGateway", "WorkerThreads");
    unsigned int worker_threads = SelectAIGatewayWorkerThreads(worker_threads_str, num_threads);

    gateglobals::g_worker_threads = worker_threads;
    gateglobals::g_main_ioc = nullptr;
    GateWorkerPool::GetInstance(worker_threads);

    memolog::LogInfo(
        "aigateway.thread_pool",
        "AIGatewayServer thread pool configured",
        {{"num_threads", std::to_string(num_threads)}, {"worker_threads", std::to_string(worker_threads)}});

    // Force LogicSystem construction now (registers the AIGateway route profile).
    LogicSystem::GetInstance();

    // The AIGateway route profile (RouteRegistry) covers /ai/chat, /ai/model/*,
    // /ai/kb/*, etc., but the SSE streaming + pet + games proxies live in the
    // legacy LogicSystem handler table (RegPost/RegGetPrefix) because they need
    // direct HttpConnection access. The retired GateServer monolith wired these
    // via AIHttpServiceRoutes::RegisterRoutes in its "full" profile block; the
    // split dropped that call, orphaning /ai/chat/stream, /ai/pet and /ai/games.
    // Re-register them here so the default client streaming path works.
    AIHttpServiceRoutes::RegisterRoutes(*LogicSystem::GetInstance());

    net::io_context ioc{static_cast<int>(num_threads)};
    // work_guard prevents ioc.run() from returning when the coroutine-based
    // AcceptLoop hasn't yet posted its first async_accept to the io_context.
    // GateDomainServer (used by all other split services) has this; its absence
    // here caused CServer::AcceptLoop to never execute: port was LISTEN but
    // every inbound connection silently hung with no HTTP response.
    auto work_guard = boost::asio::make_work_guard(ioc);
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
