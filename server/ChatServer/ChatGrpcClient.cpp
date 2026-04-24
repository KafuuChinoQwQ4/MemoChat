#include "ChatGrpcClient.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "UserMgr.h"
#include "CSession.h"
#include "PostgresMgr.h"
#include "cluster/ChatClusterDiscovery.h"
#include "logging/GrpcTrace.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include <sstream>

ChatGrpcClient::ChatGrpcClient()
{
    auto& cfg = ConfigMgr::Inst();
    const std::string self_name = cfg.GetValue("SelfServer", "Name");
    const auto cluster = memochat::cluster::LoadChatClusterConfig(
        [&cfg](const std::string& section, const std::string& key) {
            return cfg.GetValue(section, key);
        },
        self_name);

    for (const auto& node : cluster.enabledNodes()) {
        if (node.name == self_name) {
            continue;
        }
        _pools[node.name] = std::make_unique<ChatConPool>(5, node.rpc_host, node.rpc_port);
    }
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req)
{
    memolog::SpanScope span("ChatService.NotifyAddFriend", "CLIENT",
        {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyAddFriend"}, {"peer_service", server_ip}});
    AddFriendRsp rsp;
    Defer defer([&rsp, &req]() {
        rsp.set_error(ErrorCodes::Success);
        rsp.set_applyuid(req.applyuid());
        rsp.set_touid(req.touid());
        });

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    auto stub = pool->getConnection();
    Status status = stub->NotifyAddFriend(&context, req, &rsp);
    Defer defercon([&stub, this, &pool]() {
        pool->returnConnection(std::move(stub));
        });

    if (!status.ok()) {
        span.SetStatusError("grpc", status.error_message());
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    return rsp;
}

bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        memochat::json::JsonValue root;
        if (!memochat::json::reader_parse(info_str, root)) {
            return false;
        }
        userinfo->uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
        userinfo->name = memochat::json::glaze_safe_get<std::string>(root, "name", "");
        userinfo->pwd = memochat::json::glaze_safe_get<std::string>(root, "pwd", "");
        userinfo->email = memochat::json::glaze_safe_get<std::string>(root, "email", "");
        userinfo->nick = memochat::json::glaze_safe_get<std::string>(root, "nick", "");
        userinfo->desc = memochat::json::glaze_safe_get<std::string>(root, "desc", "");
        userinfo->sex = memochat::json::glaze_safe_get<int>(root, "sex", 0);
        userinfo->icon = memochat::json::glaze_safe_get<std::string>(root, "icon", "");
    }
    else {
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = PostgresMgr::GetInstance()->GetUser(uid);
        if (user_info == nullptr) {
            return false;
        }

        userinfo = user_info;

        memochat::json::JsonValue redis_root;
        redis_root["uid"] = uid;
        redis_root["pwd"] = userinfo->pwd;
        redis_root["name"] = userinfo->name;
        redis_root["email"] = userinfo->email;
        redis_root["nick"] = userinfo->nick;
        redis_root["desc"] = userinfo->desc;
        redis_root["sex"] = userinfo->sex;
        redis_root["icon"] = userinfo->icon;
        RedisMgr::GetInstance()->Set(base_key, memochat::json::writeString(redis_root));
    }

    return true;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req) {
    memolog::SpanScope span("ChatService.NotifyAuthFriend", "CLIENT",
        {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyAuthFriend"}, {"peer_service", server_ip}});
    AuthFriendRsp rsp;
    rsp.set_error(ErrorCodes::Success);

    Defer defer([&rsp, &req]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
        });

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    auto stub = pool->getConnection();
    Status status = stub->NotifyAuthFriend(&context, req, &rsp);
    Defer defercon([&stub, this, &pool]() {
        pool->returnConnection(std::move(stub));
        });

    if (!status.ok()) {
        span.SetStatusError("grpc", status.error_message());
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    return rsp;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip,
    const TextChatMsgReq& req, const memochat::json::JsonValue& rtvalue) {
    memolog::SpanScope span("ChatService.NotifyTextChatMsg", "CLIENT",
        {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyTextChatMsg"}, {"peer_service", server_ip}});

    TextChatMsgRsp rsp;
    rsp.set_error(ErrorCodes::Success);

    Defer defer([&rsp, &req]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
        for (const auto& text_data : req.textmsgs()) {
            TextChatData* new_msg = rsp.add_textmsgs();
            new_msg->set_msgid(text_data.msgid());
            new_msg->set_msgcontent(text_data.msgcontent());
        }

        });

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        rsp.set_error(ErrorCodes::TargetOffline);
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    auto stub = pool->getConnection();
    Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
    Defer defercon([&stub, this, &pool]() {
        pool->returnConnection(std::move(stub));
        });

    if (!status.ok()) {
        span.SetStatusError("grpc", status.error_message());
        rsp.set_error(ErrorCodes::TargetOffline);
        return rsp;
    }

    return rsp;
}

KickUserRsp ChatGrpcClient::NotifyKickUser(std::string server_ip, const KickUserReq& req)
{
    memolog::SpanScope span("ChatService.NotifyKickUser", "CLIENT",
        {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyKickUser"}, {"peer_service", server_ip}});
    KickUserRsp rsp;
    Defer defer([&rsp, &req]() {
        rsp.set_error(ErrorCodes::Success);
        rsp.set_uid(req.uid());
        });

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    auto stub = pool->getConnection();
    Defer defercon([&stub, this, &pool]() {
        pool->returnConnection(std::move(stub));
        });
    Status status = stub->NotifyKickUser(&context, req, &rsp);

    if (!status.ok()) {
        span.SetStatusError("grpc", status.error_message());
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    return rsp;
}

GroupMessageNotifyRsp ChatGrpcClient::NotifyGroupMessage(std::string server_ip, const GroupMessageNotifyReq& req)
{
    memolog::SpanScope span("ChatService.NotifyGroupMessage", "CLIENT",
        {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyGroupMessage"}, {"peer_service", server_ip}});
    GroupMessageNotifyRsp rsp;
    rsp.set_error(ErrorCodes::Success);

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    auto stub = pool->getConnection();
    Status status = stub->NotifyGroupMessage(&context, req, &rsp);
    Defer defercon([&stub, this, &pool]() {
        pool->returnConnection(std::move(stub));
        });

    if (!status.ok()) {
        span.SetStatusError("grpc", status.error_message());
        rsp.set_error(ErrorCodes::RPCFailed);
    }
    return rsp;
}

GroupEventNotifyRsp ChatGrpcClient::NotifyGroupEvent(std::string server_ip, const GroupEventNotifyReq& req)
{
    memolog::SpanScope span("ChatService.NotifyGroupEvent", "CLIENT",
        {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyGroupEvent"}, {"peer_service", server_ip}});
    GroupEventNotifyRsp rsp;
    rsp.set_error(ErrorCodes::Success);

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    auto stub = pool->getConnection();
    Status status = stub->NotifyGroupEvent(&context, req, &rsp);
    Defer defercon([&stub, this, &pool]() {
        pool->returnConnection(std::move(stub));
        });

    if (!status.ok()) {
        span.SetStatusError("grpc", status.error_message());
        rsp.set_error(ErrorCodes::RPCFailed);
    }
    return rsp;
}

GroupMemberBatchRsp ChatGrpcClient::NotifyGroupMemberBatch(std::string server_ip, const GroupMemberBatchReq& req)
{
    memolog::SpanScope span("ChatService.NotifyGroupMemberBatch", "CLIENT",
        {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyGroupMemberBatch"}, {"peer_service", server_ip}});
    GroupMemberBatchRsp rsp;
    rsp.set_error(ErrorCodes::Success);

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    auto stub = pool->getConnection();
    Status status = stub->NotifyGroupMemberBatch(&context, req, &rsp);
    Defer defercon([&stub, this, &pool]() {
        pool->returnConnection(std::move(stub));
        });

    if (!status.ok()) {
        span.SetStatusError("grpc", status.error_message());
        rsp.set_error(ErrorCodes::RPCFailed);
    }
    return rsp;
}
