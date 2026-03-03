#include "StatusServiceImpl.h"

#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include <climits>
#include <sstream>
#include <vector>

std::string generate_unique_string() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return to_string(uuid);
}

namespace {
std::string ExtractTraceId(ServerContext* context) {
    if (context == nullptr) {
        return "";
    }
    const auto trace_it = context->client_metadata().find("x-trace-id");
    if (trace_it == context->client_metadata().end()) {
        return "";
    }
    return std::string(trace_it->second.data(), trace_it->second.length());
}
} // namespace

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request,
                                        GetChatServerRsp* reply) {
    memolog::TraceScope trace_scope(ExtractTraceId(context));

    const auto& server = getChatServer();
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::Success);
    reply->set_token(generate_unique_string());
    insertToken(request->uid(), reply->token());

    memolog::LogInfo("status.get_chat_server", "select chat server",
                     {{"uid", std::to_string(request->uid())},
                      {"host", server.host},
                      {"port", server.port}});
    return Status::OK;
}

StatusServiceImpl::StatusServiceImpl() {
    auto& cfg = ConfigMgr::Inst();
    auto server_list = cfg["chatservers"]["Name"];

    std::vector<std::string> words;

    std::stringstream ss(server_list);
    std::string word;

    while (std::getline(ss, word, ',')) {
        words.push_back(word);
    }

    for (auto& one : words) {
        if (cfg[one]["Name"].empty()) {
            continue;
        }

        ChatServer server;
        server.port = cfg[one]["Port"];
        server.host = cfg[one]["Host"];
        server.name = cfg[one]["Name"];
        _servers[server.name] = server;
    }
}

ChatServer StatusServiceImpl::getChatServer() {
    std::lock_guard<std::mutex> guard(_server_mtx);
    auto minServer = _servers.begin()->second;
    return minServer;
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply) {
    memolog::TraceScope trace_scope(ExtractTraceId(context));

    auto uid = request->uid();
    auto token = request->token();

    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value;
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (success) {
        reply->set_error(ErrorCodes::UidInvalid);
        memolog::LogWarn("status.login.failed", "uid already active",
                         {{"uid", std::to_string(uid)},
                          {"error_code", std::to_string(ErrorCodes::UidInvalid)}});
        return Status::OK;
    }

    if (token_value != token) {
        reply->set_error(ErrorCodes::TokenInvalid);
        memolog::LogWarn("status.login.failed", "token mismatch",
                         {{"uid", std::to_string(uid)},
                          {"error_code", std::to_string(ErrorCodes::TokenInvalid)}});
        return Status::OK;
    }

    reply->set_error(ErrorCodes::Success);
    reply->set_uid(uid);
    reply->set_token(token);
    memolog::LogInfo("status.login", "token validated", {{"uid", std::to_string(uid)}});
    return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token) {
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    RedisMgr::GetInstance()->Set(token_key, token);
}
