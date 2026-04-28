#include "StatusServiceImpl.h"

#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "StatusAsyncSideEffects.h"
#include "const.h"
#include "cluster/ChatClusterDiscovery.h"
#include "logging/GrpcTrace.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TraceContext.h"

#include <climits>
#include <algorithm>
#include <sstream>
#include <vector>

std::string generate_unique_string() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return to_string(uuid);
}

namespace {
std::string JoinStrings(const std::vector<std::string>& values, const char* delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << delimiter;
        }
        oss << values[i];
    }
    return oss.str();
}
} // namespace

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request,
                                        GetChatServerRsp* reply) {
    memolog::BindGrpcTraceContext(context);
    memolog::SpanScope span("StatusService.GetChatServer", "SERVER",
                            {{"rpc.system", "grpc"}, {"rpc.service", "StatusService"}, {"rpc.method", "GetChatServer"}});
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });

    std::vector<std::string> server_load_snapshot;
    std::vector<std::string> least_loaded_servers;
    const auto& server = getChatServer(&server_load_snapshot, &least_loaded_servers);
    const int uid = request->uid();
    const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    std::string token_value;
    bool has_token = RedisMgr::GetInstance()->Get(token_key, token_value);

    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::Success);
    if (has_token && !token_value.empty()) {
        // Reuse existing token to avoid invalidating in-flight chat login handshakes.
        reply->set_token(token_value);
    } else {
        reply->set_token(generate_unique_string());
        insertToken(uid, reply->token());
    }

    memolog::LogInfo("status.get_chat_server", "select chat server",
                     {{"uid", std::to_string(uid)},
                      {"module", "grpc"},
                      {"reuse_token", (has_token && !token_value.empty()) ? "true" : "false"},
                      {"server_loads", JoinStrings(server_load_snapshot, ",")},
                      {"least_loaded_servers", JoinStrings(least_loaded_servers, ",")},
                      {"server_count", std::to_string(server_load_snapshot.size())},
                      {"selected_server", server.name},
                      {"host", server.host},
                      {"port", server.port}});
    if (_side_effects) {
        _side_effects->PublishPresenceRefresh(uid, server.name, "get_chat_server");
    }
    return Status::OK;
}

StatusServiceImpl::StatusServiceImpl() {
    auto& cfg = ConfigMgr::Inst();
    const auto cluster = memochat::cluster::LoadChatClusterConfig(
        [&cfg](const std::string& section, const std::string& key) {
            return cfg.GetValue(section, key);
        });

    for (const auto& node : cluster.enabledNodes()) {
        ChatServer server;
        server.port = node.tcp_port;
        server.host = node.tcp_host;
        server.name = node.name;
        _servers[server.name] = server;
        _known_server_names.insert(server.name);
    }
    _side_effects = std::make_unique<StatusAsyncSideEffects>(_known_server_names);
    _side_effects->Start();
}

StatusServiceImpl::~StatusServiceImpl()
{
    if (_side_effects) {
        _side_effects->Stop();
    }
}

ChatServer StatusServiceImpl::getChatServer(std::vector<std::string>* server_load_snapshot,
                                           std::vector<std::string>* least_loaded_servers_snapshot) {
    std::lock_guard<std::mutex> guard(_server_mtx);
    if (server_load_snapshot) {
        server_load_snapshot->clear();
    }
    if (least_loaded_servers_snapshot) {
        least_loaded_servers_snapshot->clear();
    }
    if (_servers.empty()) {
        return ChatServer();
    }

    std::vector<ChatServer> ordered_servers;
    ordered_servers.reserve(_servers.size());
    for (const auto& entry : _servers) {
        ordered_servers.push_back(entry.second);
    }
    std::sort(ordered_servers.begin(), ordered_servers.end(), [](const ChatServer& lhs, const ChatServer& rhs) {
        return lhs.name < rhs.name;
    });

    int min_online = INT_MAX;
    std::vector<ChatServer> least_loaded;
    least_loaded.reserve(ordered_servers.size());
    for (auto server : ordered_servers) {
        const std::string online_users_key = std::string(SERVER_ONLINE_USERS_PREFIX) + server.name;
        int online_count = 0;
        const bool load_ok = RedisMgr::GetInstance()->SCard(online_users_key, online_count);
        if (!load_ok || online_count < 0) {
            online_count = 0;
        }
        server.con_count = online_count;
        if (server_load_snapshot) {
            std::ostringstream one;
            one << server.name << "=" << online_count;
            if (!load_ok) {
                one << "(redis-fallback)";
            }
            server_load_snapshot->push_back(one.str());
        }
        if (online_count < min_online) {
            min_online = online_count;
            least_loaded.clear();
            least_loaded.push_back(server);
            continue;
        }
        if (online_count == min_online) {
            least_loaded.push_back(server);
        }
    }

    if (least_loaded.empty()) {
        return ordered_servers.front();
    }

    if (least_loaded_servers_snapshot) {
        for (const auto& one : least_loaded) {
            least_loaded_servers_snapshot->push_back(one.name);
        }
    }

    const auto next_index = static_cast<size_t>(_rr_counter.fetch_add(1, std::memory_order_relaxed));
    return least_loaded[next_index % least_loaded.size()];
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply) {
    memolog::BindGrpcTraceContext(context);
    memolog::SpanScope span("StatusService.Login", "SERVER",
                            {{"rpc.system", "grpc"}, {"rpc.service", "StatusService"}, {"rpc.method", "Login"}});
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });

    auto uid = request->uid();
    auto token = request->token();

    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value;
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (!success || token_value.empty()) {
        reply->set_error(ErrorCodes::UidInvalid);
        memolog::LogWarn("status.login.failed", "uid token not found",
                         {{"uid", std::to_string(uid)},
                          {"module", "grpc"},
                          {"error_code", std::to_string(ErrorCodes::UidInvalid)}});
        return Status::OK;
    }

    if (token_value != token) {
        reply->set_error(ErrorCodes::TokenInvalid);
        memolog::LogWarn("status.login.failed", "token mismatch",
                         {{"uid", std::to_string(uid)},
                          {"module", "grpc"},
                          {"error_code", std::to_string(ErrorCodes::TokenInvalid)}});
        return Status::OK;
    }

    reply->set_error(ErrorCodes::Success);
    reply->set_uid(uid);
    reply->set_token(token);
    memolog::LogInfo("status.login", "token validated", {{"uid", std::to_string(uid)}, {"module", "grpc"}});
    if (_side_effects) {
        _side_effects->PublishAuditLogin(uid, "", "", "", "status_login_validated");
    }
    return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token) {
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    RedisMgr::GetInstance()->Set(token_key, token);
}
