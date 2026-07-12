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
#include "RedisMgr.hpp"
#include "domain/AIHttpServiceRoutes.hpp"
#include "AsioIOServicePool.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"
#include "auth/AuthSecret.hpp"
#include "runtime/ExplicitThread.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

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
    const auto log_config = memolog::LogConfig::FromGetter(
        [&cfgMgr](const std::string& section, const std::string& key)
        {
            return cfgMgr.GetValue(section, key);
        });
    std::string logger_error;
    if (!memolog::Logger::Init("AIGatewayServer", log_config, &logger_error))
    {
        std::cerr << "AIGatewayServer fatal: " << logger_error << std::endl;
        return 1;
    }

    memolog::LogInfo("aigateway.start", "AIGatewayServer starting...");

    auto jwt_access_secret = cfgMgr.GetValue("AuthToken", "JwtSecret");
    if (jwt_access_secret.empty())
    {
        jwt_access_secret = std::string(memochat::auth::kWellKnownDevJwtAccessSecret);
    }
    std::string startup_error;
    if (!memochat::auth::RequireNonDefaultJwtAccessSecretInProduction("AIGatewayServer",
                                                                      jwt_access_secret,
                                                                      startup_error))
    {
        memolog::LogError("aigateway.config_invalid", "invalid JWT access configuration", {{"error", startup_error}});
        memolog::Logger::Shutdown();
        return 1;
    }

    // Serve only health + /ai/* routes. MUST be set before LogicSystem::Instance().
    LogicSystem::ClearRouteProfileRegistrars();
    LogicSystem::AddRouteProfileRegistrar(memochat::gate::profiles::RegisterAIGateway);

    std::string port_str = cfgMgr["AIGateway"]["Port"];
    unsigned short gate_port = SelectAIGatewayListenPort(port_str, kDefaultAIGatewayPort);

    unsigned int num_threads = NormalizeAIGatewayIoThreads(memochat::runtime::ExplicitThread::HardwareConcurrency());
    auto worker_threads_str = cfgMgr.GetValue("AIGateway", "WorkerThreads");
    unsigned int worker_threads = SelectAIGatewayWorkerThreads(worker_threads_str, num_threads);

    gateglobals::g_worker_threads = worker_threads;
    gateglobals::g_main_ioc = nullptr;
    auto io_pool = AsioIOServicePool::GetInstance();
    if (!io_pool->Ready())
    {
        memolog::LogError("service.thread_pool_start_failed",
                          "AIGatewayServer I/O thread pool failed to start",
                          {{"error", io_pool->startupError()}});
        memolog::Logger::Shutdown();
        return 1;
    }
    const auto redis = RedisMgr::GetInstance();
    if (!redis->Ready())
    {
        memolog::LogError("service.dependency_unavailable",
                          "AIGatewayServer Redis initialization failed",
                          {{"error", redis->StartupError()}});
        io_pool->Stop();
        memolog::Logger::Shutdown();
        return 1;
    }
    auto worker_pool = GateWorkerPool::GetInstance();
    std::string worker_pool_error;
    if (!worker_pool->Start(worker_threads, &worker_pool_error))
    {
        memolog::LogError("service.worker_pool_start_failed",
                          "AIGatewayServer failed to start worker pool",
                          {{"error", worker_pool_error}});
        io_pool->Stop();
        memolog::Logger::Shutdown();
        return 1;
    }

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
    auto work_guard = boost::asio::make_work_guard(ioc);
    gateglobals::g_main_ioc = &ioc;
    auto server = std::make_shared<CServer>(ioc);
    std::string listen_error;
    if (!server->Open(gate_port, &listen_error))
    {
        memolog::LogError("service.start_failed",
                          "AIGatewayServer failed to open listener",
                          {{"port", std::to_string(gate_port)}, {"error", listen_error}});
        gateglobals::g_main_ioc = nullptr;
        worker_pool->Stop();
        io_pool->Stop();
        memolog::Logger::Shutdown();
        return 1;
    }
    server->Start();
    memolog::LogInfo("service.start", "AIGatewayServer listening", {{"port", std::to_string(gate_port)}});

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
                          "AIGatewayServer failed to install shutdown signals",
                          {{"error", signal_error.message()}});
        gateglobals::g_main_ioc = nullptr;
        worker_pool->Stop();
        io_pool->Stop();
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
            memolog::LogInfo("aigateway.signal", "AIGatewayServer received shutdown signal");
            ioc.stop();
        });

    ioc.run();
    gateglobals::g_main_ioc = nullptr;

    memolog::LogInfo("service.stop", "AIGatewayServer stopped");
    worker_pool->Stop();
    io_pool->Stop();
    memolog::Logger::Shutdown();
    return 0;
}
