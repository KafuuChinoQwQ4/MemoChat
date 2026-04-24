#ifdef _WIN32
#if defined(_MSC_VER) && !defined(__clang__)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif
// OPENSSL_Applink: must be compiled once per EXE (not per translation unit).
// Use the OpenSSL::applink target from vcpkg instead of including the .c file directly.
#endif

#include "../../server/common/WinsockCompat.h"

#include "GateHttp3Listener.h"
#if defined(MEMOCHAT_HAVE_NGHTTP2)
#include "NgHttp2Server.h"
#endif
#include "LogicSystem.h"
#include "SnowflakeUtil.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include <hiredis/hiredis.h>
#include "RedisMgr.h"
#include "PostgresMgr.h"
#include "GateAsyncSideEffects.h"
#include "GateWorkerPool.h"
#include "GateGlobals.h"
#include "AsioIOServicePool.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"
#include <chrono>
#include <thread>
#include <aws/core/Aws.h>
#include <iostream>
#include "const.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>

using namespace memolog;

static Aws::SDKOptions g_aws_options;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Aws::InitAPI(g_aws_options);

    auto& cfgMgr = ConfigMgr::Inst();
    memolog::Logger::Init("GateServer", memolog::LogConfig::FromGetter(
        [&cfgMgr](const std::string& section, const std::string& key) {
            return cfgMgr.GetValue(section, key);
        }));

    memolog::LogInfo("gate.start", "GateServer starting...");

    // Initialize Snowflake ID generator
    {
        auto worker_id_str = cfgMgr.GetValue("Snowflake", "WorkerId");
        auto datacenter_id_str = cfgMgr.GetValue("Snowflake", "DatacenterId");
        int64_t worker_id = worker_id_str.empty() ? 0 : std::stoll(worker_id_str);
        int64_t datacenter_id = datacenter_id_str.empty() ? 0 : std::stoll(datacenter_id_str);
        SnowflakeUtil::getInstance().init(worker_id, datacenter_id);
    }

    GateAsyncSideEffects::Instance().Start();

    std::string gate_port_str = cfgMgr["GateServer"]["Port"];
    unsigned short gate_port = atoi(gate_port_str.c_str());
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads < 2) num_threads = 4;
    auto worker_threads_str = cfgMgr.GetValue("GateServer", "WorkerThreads");
    unsigned int worker_threads = worker_threads_str.empty() ? num_threads : std::atoi(worker_threads_str.c_str());
    if (worker_threads < 1) worker_threads = num_threads;

    gateglobals::g_worker_threads = worker_threads;
    gateglobals::g_main_ioc = nullptr;
    GateWorkerPool::GetInstance(worker_threads);

    memolog::LogInfo("gate.thread_pool", "GateServer thread pool configured", {
        {"num_threads", std::to_string(num_threads)},
        {"worker_threads", std::to_string(worker_threads)}
    });

    // ── Start HTTP/1.1 server (Boost.Asio) on gate_port ───────────────────────
    net::io_context ioc{ static_cast<int>(num_threads) };
    gateglobals::g_main_ioc = &ioc;
    std::make_shared<CServer>(ioc, gate_port)->Start();
    memolog::LogInfo("service.start", "GateServer listening", {
        {"port", std::to_string(gate_port)}
    });

    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
        (void)signal_number;
        if (error) { return; }
        memolog::LogInfo("gate.signal", "GateServer received shutdown signal");
        ioc.stop();
    });

    // ── Start HTTP/3 listener on port gate_port + 1 (QUIC) ────────────────────
#if defined(MEMOCHAT_ENABLE_HTTP3)
    int http3_port = gate_port + 1;
    std::string http3_error;
    auto http3_listener = std::make_shared<GateHttp3Listener>(ioc, *LogicSystem::GetInstance(), http3_port);
    if (http3_listener->Start(http3_error)) {
        memolog::LogInfo("service.http3.start", "GateServer HTTP/3 listener started", {{"port", std::to_string(http3_port)}});
    } else {
        memolog::LogWarn("service.http3.start.fail", "GateServer HTTP/3 listener failed to start", {{"error", http3_error}});
    }
#endif

    // ── Start HTTP/2 server (Boost.Asio + nghttp2) on port gate_port + 2 ──────
#if defined(MEMOCHAT_HAVE_NGHTTP2)
    auto h2_port_str = cfgMgr.GetValue("GateServer", "Http2Port");
    int h2_port = h2_port_str.empty() ? static_cast<int>(gate_port) + 2 : std::atoi(h2_port_str.c_str());
    auto& h2_server = NgHttp2Server::GetInstance();
    h2_server.SetPort(h2_port);
    if (h2_server.Initialize()) {
        memolog::LogInfo("service.http2.start", "GateServer HTTP/2 (nghttp2) initializing", {{"port", std::to_string(h2_port)}});
        std::thread h2_thread([&h2_server, h2_port]() {
            memolog::LogInfo("service.http2.run", "NgHttp2Server thread started", {{"port", std::to_string(h2_port)}});
            try {
                h2_server.Run();
            } catch (const std::exception& e) {
                memolog::LogError("gate.http2.thread.exc", "NgHttp2Server thread threw uncaught exception", {{"error", e.what()}});
            } catch (...) {
                memolog::LogError("gate.http2.thread.exc", "NgHttp2Server thread threw unknown exception");
            }
            memolog::LogInfo("service.http2.stop", "NgHttp2Server thread stopped");
        });
        h2_thread.detach();
    } else {
        memolog::LogWarn("service.http2.init.fail", "NgHttp2Server Initialize() returned false — HTTP/2 disabled");
    }
#endif

    ioc.run();

    memolog::LogInfo("service.stop", "GateServer stopped");
    GateAsyncSideEffects::Instance().Stop();
    GateWorkerPool::GetInstance()->Stop();
    RedisMgr::GetInstance()->Close();
    Aws::ShutdownAPI(g_aws_options);
    memolog::Logger::Shutdown();
    return 0;
}
