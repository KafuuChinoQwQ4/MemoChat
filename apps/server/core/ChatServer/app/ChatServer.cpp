
//

#include "LogicSystem.hpp"
#include "LogicSystemConfig.hpp"
#include "SnowflakeUtil.hpp"
#include <csignal>
#include <mutex>
#include <unordered_set>
#include <vector>
#include "AsioIOServicePool.hpp"
#include "CServer.hpp"
#include "ConfigMgr.hpp"
#include "PostgresMgr.hpp"
#include "RedisMgr.hpp"
#include "ChatServiceImpl.hpp"
#include "ChatIngressCoordinator.hpp"
#include "ChatRuntime.hpp"
#include "const.hpp"
#include "auth/AuthSecret.hpp"
#include "cluster/ChatClusterDiscovery.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"
#include "runtime/ExplicitThread.hpp"
#include <charconv>
#include <cstdlib>
#include <iostream>

bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

namespace
{
std::string ServerOnlineUsersKey(const std::string& server_name)
{
    return std::string(SERVER_ONLINE_USERS_PREFIX) + server_name;
}

void CleanupTrackedOnlineState(const std::string& server_name)
{
    std::unordered_set<std::string> online_uids;
    const auto online_key = ServerOnlineUsersKey(server_name);
    std::vector<std::string> tracked_uids;
    RedisMgr::GetInstance()->SMembers(online_key, tracked_uids);
    for (const auto& uid : tracked_uids)
    {
        if (!uid.empty())
        {
            online_uids.insert(uid);
        }
    }

    std::vector<std::string> ip_keys;
    RedisMgr::GetInstance()->Keys(std::string(USERIPPREFIX) + "*", ip_keys);
    for (const auto& ip_key : ip_keys)
    {
        if (ip_key.rfind(USERIPPREFIX, 0) != 0)
        {
            continue;
        }
        std::string mapped_server;
        if (!RedisMgr::GetInstance()->Get(ip_key, mapped_server) || mapped_server != server_name)
        {
            continue;
        }
        online_uids.insert(ip_key.substr(std::strlen(USERIPPREFIX)));
    }

    for (const auto& uid : online_uids)
    {
        if (uid.empty())
        {
            continue;
        }
        RedisMgr::GetInstance()->Del(USERIPPREFIX + uid);
        RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid);
    }
    RedisMgr::GetInstance()->Del(online_key);
    RedisMgr::GetInstance()->InitCount(server_name);
    memolog::LogInfo("service.online_state_reset",
                     "ChatServer online state reset",
                     {{"name", server_name}, {"tracked_uid_count", std::to_string(online_uids.size())}});
}

bool ParseConfigPath(int argc, char** argv, std::string& path, std::string& error)
{
    path.clear();
    error.clear();
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--config")
        {
            if (i + 1 >= argc)
            {
                error = "missing value for --config";
                return false;
            }
            path = argv[++i];
            continue;
        }
        error = "unknown argument: " + arg;
        return false;
    }
    return true;
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
} // namespace

int main(int argc, char** argv)
{
    std::string config_path;
    std::string startup_error;
    if (!ParseConfigPath(argc, argv, config_path, startup_error))
    {
        std::cerr << "ChatServer fatal: " << startup_error << std::endl;
        return EXIT_FAILURE;
    }
    ConfigMgr::InitConfigPath(config_path);
    auto& cfg = ConfigMgr::Inst();
    const std::string server_name = memochat::cluster::ResolveSelfNodeName(
        [&cfg](const std::string& section, const std::string& key)
        {
            return cfg.GetValue(section, key);
        });
    if (server_name.empty())
    {
        std::cerr << "ChatServer fatal: chat self node name is empty" << std::endl;
        return EXIT_FAILURE;
    }

    memochat::cluster::ChatClusterConfig cluster;
    if (!memochat::cluster::LoadChatClusterConfig(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            },
            server_name,
            cluster,
            startup_error))
    {
        std::cerr << "ChatServer fatal: " << startup_error << std::endl;
        return EXIT_FAILURE;
    }
    const auto* self_node = cluster.findNode(server_name);
    if (self_node == nullptr)
    {
        std::cerr << "ChatServer fatal: self node missing from cluster config: " << server_name << std::endl;
        return EXIT_FAILURE;
    }
    SetInstanceNameEnv(server_name);

    int64_t datacenter_id = 0;
    int64_t worker_id = 0;
    if (!ParseInt64Or(cfg.GetValue("Snowflake", "DatacenterId"), 1, datacenter_id) ||
        !ParseInt64Or(cfg.GetValue("Snowflake", "WorkerId"), 1, worker_id) ||
        !SnowflakeUtil::getInstance().init(worker_id, datacenter_id))
    {
        std::cerr << "ChatServer fatal: invalid Snowflake WorkerId or DatacenterId" << std::endl;
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
    if (!memolog::Logger::Init("ChatServer", log_cfg, &startup_error))
    {
        std::cerr << "ChatServer fatal: " << startup_error << std::endl;
        return EXIT_FAILURE;
    }
    memolog::Telemetry::Init("ChatServer", telemetry_cfg);

    auto chat_auth_secret = cfg.GetValue("ChatAuth", "HmacSecret");
    if (chat_auth_secret.empty())
    {
        chat_auth_secret = std::string(memochat::auth::kWellKnownDevHmacSecret);
    }
    if (!memochat::auth::RequireNonDefaultChatAuthSecretInProduction("ChatServer", chat_auth_secret, startup_error))
    {
        memolog::LogError("service.start_failed", "ChatServer auth configuration rejected", {{"error", startup_error}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }

    auto pool = AsioIOServicePool::GetInstance();
    if (!pool->Ready())
    {
        memolog::LogError("service.thread_pool_start_failed",
                          "ChatServer I/O thread pool failed to start",
                          {{"error", pool->startupError()}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
    const auto storage_init_start = std::chrono::steady_clock::now();
    auto postgres_mgr = PostgresMgr::GetInstance();
    if (!postgres_mgr->Ready())
    {
        memolog::LogError("service.postgresql_init_failed",
                          "ChatServer PostgreSQL initialization failed",
                          {{"error", postgres_mgr->startupError()}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
    memolog::LogInfo("service.postgresql_ready",
                     "ChatServer postgresql ready",
                     {{"name", server_name},
                      {"storage_init_ms",
                       std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::steady_clock::now() - storage_init_start)
                                          .count())}});

    const bool ingress_enabled = memochat::chatruntime::IsIngressEnabled();
    if (ingress_enabled)
    {
        CleanupTrackedOnlineState(server_name);
    }
    Defer cleanup(
        [server_name, ingress_enabled]()
        {
            if (ingress_enabled)
            {
                CleanupTrackedOnlineState(server_name);
            }
            RedisMgr::GetInstance()->DelCount(server_name);
            RedisMgr::GetInstance()->Close();
            memolog::Telemetry::Shutdown();
            memolog::Logger::Shutdown();
        });

    boost::asio::io_context io_context;
    std::shared_ptr<CServer> pointer_server;
    std::unique_ptr<memochat::chatserver::ChatIngressCoordinator> ingress_coordinator;
    if (ingress_enabled)
    {
        ingress_coordinator = std::make_unique<memochat::chatserver::ChatIngressCoordinator>(io_context);
        if (!ingress_coordinator->Start(*self_node, &startup_error))
        {
            memolog::LogError("service.ingress_start_failed",
                              "ChatServer TCP ingress failed to start",
                              {{"error", startup_error}});
            return EXIT_FAILURE;
        }
        pointer_server = ingress_coordinator->tcpServer();
    }

    LogicSystem::SetWorkerConfig(LogicSystemConfig{});
    auto logic = LogicSystem::GetInstance();
    if (!logic->Ready())
    {
        memolog::LogError("service.runtime_start_failed",
                          "ChatServer runtime composition failed",
                          {{"error", logic->startupError()}});
        return EXIT_FAILURE;
    }
    logic->SetServer(pointer_server.get());

    const std::string server_address(self_node->rpc_host + ":" + self_node->rpc_port);
    ChatServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    service.RegisterServer(pointer_server);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server)
    {
        memolog::LogError("service.start_failed",
                          "ChatServer failed to listen",
                          {{"rpc_address", server_address}, {"name", server_name}});
        return EXIT_FAILURE;
    }
    memolog::LogInfo("service.start", "ChatServer listening", {{"rpc_address", server_address}, {"name", server_name}});

    memochat::runtime::ExplicitThread grpc_server_thread;
    std::string thread_error;
    if (!grpc_server_thread.Start(
            [&server]() noexcept
            {
                server->Wait();
            },
            &thread_error))
    {
        memolog::LogError("service.thread_start_failed",
                          "ChatServer failed to start gRPC wait thread",
                          {{"error", thread_error}});
        server->Shutdown();
        return EXIT_FAILURE;
    }

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
        memolog::LogError("service.signal_setup_failed",
                          "ChatServer failed to register shutdown signals",
                          {{"error", signal_error.message()}});
        server->Shutdown();
        if (!grpc_server_thread.Join(&thread_error))
        {
            memolog::LogError("service.thread_join_failed",
                              "ChatServer failed to join gRPC wait thread",
                              {{"error", thread_error}});
        }
        return EXIT_FAILURE;
    }
    signals.async_wait(
        [&io_context, pool, &server, &ingress_coordinator](const boost::system::error_code&, int)
        {
            if (ingress_coordinator)
            {
                ingress_coordinator->Stop();
            }
            boost::asio::post(io_context,
                              [&io_context, pool, &server]()
                              {
                                  boost::asio::post(io_context,
                                                    [&io_context, pool, &server]()
                                                    {
                                                        io_context.stop();
                                                        pool->Stop();
                                                        server->Shutdown();
                                                    });
                              });
        });

    io_context.run();
    if (!grpc_server_thread.Join(&thread_error))
    {
        memolog::LogError("service.thread_join_failed",
                          "ChatServer failed to join gRPC wait thread",
                          {{"error", thread_error}});
        return EXIT_FAILURE;
    }
    memolog::LogInfo("service.stop", "ChatServer stopped");
    return EXIT_SUCCESS;
}
