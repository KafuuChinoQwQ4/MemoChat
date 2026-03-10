#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "CSession.h"
#include "logging/GrpcTrace.h"
#include "logging/Telemetry.h"
#include "logging/TraceContext.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <chrono>
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "MongoMgr.h"

ChatServiceImpl::ChatServiceImpl()
{
}

Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* reply)
{
    memolog::BindGrpcTraceContext(context);
    memolog::SpanScope span("ChatService.NotifyAddFriend", "SERVER",
                            {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyAddFriend"}});
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });
    auto touid = request->touid();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
        });

    if (session == nullptr) {
        return Status::OK;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"] = request->name();
    rtvalue["desc"] = request->desc();
    rtvalue["icon"] = request->icon();
    rtvalue["sex"] = request->sex();
    rtvalue["nick"] = request->nick();
    auto apply_info = MysqlMgr::GetInstance()->GetUser(request->applyuid());
    if (apply_info) {
        rtvalue["user_id"] = apply_info->user_id;
    }

    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request,
    AuthFriendRsp* reply) {
    memolog::BindGrpcTraceContext(context);
    memolog::SpanScope span("ChatService.NotifyAuthFriend", "SERVER",
                            {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyAuthFriend"}});
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });
    auto touid = request->touid();
    auto fromuid = request->fromuid();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_fromuid(request->fromuid());
        reply->set_touid(request->touid());
        });

    if (session == nullptr) {
        return Status::OK;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = request->fromuid();
    rtvalue["touid"] = request->touid();

    std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
    auto user_info = std::make_shared<UserInfo>();
    bool b_info = GetBaseInfo(base_key, fromuid, user_info);
    if (b_info) {
        rtvalue["name"] = user_info->name;
        rtvalue["nick"] = user_info->nick;
        rtvalue["icon"] = user_info->icon;
        rtvalue["sex"] = user_info->sex;
        rtvalue["user_id"] = user_info->user_id;
    }
    else {
        rtvalue["error"] = ErrorCodes::UidInvalid;
    }

    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context,
    const TextChatMsgReq* request, TextChatMsgRsp* reply) {
    memolog::BindGrpcTraceContext(context);
    memolog::SpanScope span("ChatService.NotifyTextChatMsg", "SERVER",
                            {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyTextChatMsg"}});
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });
    auto touid = request->touid();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    if (session == nullptr) {
        reply->set_error(ErrorCodes::TargetOffline);
        return Status::OK;
    }

    reply->set_error(ErrorCodes::Success);
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = request->fromuid();
    rtvalue["touid"] = request->touid();

    Json::Value text_array;
    for (auto& msg : request->textmsgs()) {
        Json::Value element;
        element["content"] = msg.msgcontent();
        element["msgid"] = msg.msgid();
        std::shared_ptr<PrivateMessageInfo> private_msg;
        int64_t created_at = 0;
        if ((MongoMgr::GetInstance()->Enabled() && MongoMgr::GetInstance()->GetPrivateMessageByMsgId(msg.msgid(), private_msg) && private_msg) ||
            (MysqlMgr::GetInstance()->GetPrivateMessageByMsgId(msg.msgid(), private_msg) && private_msg)) {
            created_at = private_msg->created_at;
        }
        if (created_at <= 0) {
            created_at = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        }
        element["created_at"] = static_cast<Json::Int64>(created_at);
        text_array.append(element);
    }
    rtvalue["text_array"] = text_array;

    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
    return Status::OK;
}

bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        userinfo->uid = root["uid"].asInt();
        userinfo->user_id = root["user_id"].asString();
        userinfo->name = root["name"].asString();
        userinfo->pwd = root["pwd"].asString();
        userinfo->email = root["email"].asString();
        userinfo->nick = root["nick"].asString();
        userinfo->desc = root["desc"].asString();
        userinfo->sex = root["sex"].asInt();
        userinfo->icon = root["icon"].asString();
    }
    else {
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::GetInstance()->GetUser(uid);
        if (user_info == nullptr) {
            return false;
        }

        userinfo = user_info;

        Json::Value redis_root;
        redis_root["uid"] = uid;
        redis_root["user_id"] = userinfo->user_id;
        redis_root["pwd"] = userinfo->pwd;
        redis_root["name"] = userinfo->name;
        redis_root["email"] = userinfo->email;
        redis_root["nick"] = userinfo->nick;
        redis_root["desc"] = userinfo->desc;
        redis_root["sex"] = userinfo->sex;
        redis_root["icon"] = userinfo->icon;
        RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
    }

    return true;
}

Status ChatServiceImpl::NotifyKickUser(::grpc::ServerContext* context,
    const KickUserReq* request, KickUserRsp* reply)
{
    memolog::BindGrpcTraceContext(context);
    memolog::SpanScope span("ChatService.NotifyKickUser", "SERVER",
                            {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyKickUser"}});
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });
    auto uid = request->uid();
    auto session = UserMgr::GetInstance()->GetSession(uid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_uid(request->uid());
        });

    if (session == nullptr) {
        return Status::OK;
    }

    session->NotifyOffline(uid);
    _p_server->ClearSession(session->GetSessionId());

    return Status::OK;
}

Status ChatServiceImpl::NotifyGroupMessage(::grpc::ServerContext* context,
    const GroupMessageNotifyReq* request, GroupMessageNotifyRsp* response)
{
    memolog::BindGrpcTraceContext(context);
    memolog::SpanScope span("ChatService.NotifyGroupMessage", "SERVER",
                            {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyGroupMessage"}});
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });
    int delivered = 0;
    for (int i = 0; i < request->touids_size(); ++i) {
        auto uid = request->touids(i);
        auto session = UserMgr::GetInstance()->GetSession(uid);
        if (!session) {
            continue;
        }
        session->Send(request->payload_json(), static_cast<short>(request->tcp_msgid()));
        delivered++;
    }
    response->set_error(ErrorCodes::Success);
    response->set_delivered(delivered);
    return Status::OK;
}

Status ChatServiceImpl::NotifyGroupEvent(::grpc::ServerContext* context,
    const GroupEventNotifyReq* request, GroupEventNotifyRsp* response)
{
    memolog::BindGrpcTraceContext(context);
    memolog::SpanScope span("ChatService.NotifyGroupEvent", "SERVER",
                            {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyGroupEvent"}});
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });
    int delivered = 0;
    for (int i = 0; i < request->touids_size(); ++i) {
        auto uid = request->touids(i);
        auto session = UserMgr::GetInstance()->GetSession(uid);
        if (!session) {
            continue;
        }
        session->Send(request->payload_json(), static_cast<short>(request->tcp_msgid()));
        delivered++;
    }
    response->set_error(ErrorCodes::Success);
    response->set_delivered(delivered);
    return Status::OK;
}

Status ChatServiceImpl::NotifyGroupMemberBatch(::grpc::ServerContext* context,
    const GroupMemberBatchReq* request, GroupMemberBatchRsp* response)
{
    memolog::BindGrpcTraceContext(context);
    memolog::SpanScope span("ChatService.NotifyGroupMemberBatch", "SERVER",
                            {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyGroupMemberBatch"}});
    Defer clear_trace([]() { memolog::TraceContext::Clear(); });
    int delivered = 0;
    for (int i = 0; i < request->touids_size(); ++i) {
        auto uid = request->touids(i);
        auto session = UserMgr::GetInstance()->GetSession(uid);
        if (!session) {
            continue;
        }
        session->Send(request->payload_json(), static_cast<short>(request->tcp_msgid()));
        delivered++;
    }
    response->set_error(ErrorCodes::Success);
    response->set_delivered(delivered);
    return Status::OK;
}

void ChatServiceImpl::RegisterServer(std::shared_ptr<CServer> pServer)
{
    _p_server = pServer;
}
