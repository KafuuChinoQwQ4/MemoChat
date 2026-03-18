#include "PrivateMessageService.h"

#include "ChatGrpcClient.h"
#include "ChatRuntime.h"
#include "ConfigMgr.h"
#include "CSession.h"
#include "LogicSystem.h"
#include "MessageDeliveryService.h"
#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "cluster/ChatClusterDiscovery.h"
#include "logging/Logger.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <json/json.h>
#include <set>
#include <unordered_set>
#include <vector>

namespace {
enum class OnlineRouteKind {
    Offline,
    Local,
    Remote,
    Stale
};

struct OnlineRouteDecision {
    OnlineRouteKind kind = OnlineRouteKind::Offline;
    std::shared_ptr<CSession> session;
    std::string redis_server;
    bool local_session_found = false;
};

std::string TrimCopyLocal(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return std::string();
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string ServerOnlineUsersKeyLocal(const std::string& server_name) {
    return std::string(SERVER_ONLINE_USERS_PREFIX) + server_name;
}

std::vector<std::string> KnownChatServerNamesLocal() {
    std::vector<std::string> servers;
    auto& cfg = ConfigMgr::Inst();
    static const auto cluster = memochat::cluster::LoadChatClusterConfig(
        [&cfg](const std::string& section, const std::string& key) {
            return cfg.GetValue(section, key);
        },
        TrimCopyLocal(cfg["SelfServer"]["Name"]));
    for (const auto& node : cluster.enabledNodes()) {
        servers.push_back(node.name);
    }
    return servers;
}

std::string ResolveServerFromOnlineSetsLocal(const std::string& uid_str) {
    if (uid_str.empty()) {
        return std::string();
    }

    for (const auto& server_name : KnownChatServerNamesLocal()) {
        std::vector<std::string> online_uids;
        RedisMgr::GetInstance()->SMembers(ServerOnlineUsersKeyLocal(server_name), online_uids);
        if (std::find(online_uids.begin(), online_uids.end(), uid_str) != online_uids.end()) {
            return server_name;
        }
    }
    return std::string();
}

void RepairOnlineRouteStateLocal(int uid, const std::shared_ptr<CSession>& session, const std::string& server_name) {
    if (uid <= 0 || !session || server_name.empty()) {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RedisMgr::GetInstance()->Set(USERIPPREFIX + uid_str, server_name);
    RedisMgr::GetInstance()->Set(USER_SESSION_PREFIX + uid_str, session->GetSessionId());
    RedisMgr::GetInstance()->SAdd(ServerOnlineUsersKeyLocal(server_name), uid_str);
}

void ClearTrackedOnlineRouteLocal(int uid, const std::string& server_name) {
    if (uid <= 0 || server_name.empty()) {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
    RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
    RedisMgr::GetInstance()->SRem(ServerOnlineUsersKeyLocal(server_name), uid_str);
}

OnlineRouteDecision ResolveOnlineRouteLocal(int uid) {
    OnlineRouteDecision route;
    if (uid <= 0) {
        return route;
    }

    const auto self_name = ConfigMgr::Inst()["SelfServer"]["Name"];
    const auto uid_str = std::to_string(uid);

    // 首先检查本地 UserMgr 中是否有会话
    auto session = UserMgr::GetInstance()->GetSession(uid);
    memolog::LogInfo("chat.private.route.debug", "resolve online route start",
        {{"uid", uid_str}, {"self_server", self_name},
         {"local_session_found", session ? "true" : "false"}});

    if (session) {
        route.kind = OnlineRouteKind::Local;
        route.session = session;
        route.redis_server = self_name;
        route.local_session_found = true;
        RepairOnlineRouteStateLocal(uid, session, self_name);
        memolog::LogInfo("chat.private.route.debug", "resolved to local session",
            {{"uid", uid_str}, {"self_server", self_name}});
        return route;
    }

    // 检查 Redis 中的用户在线状态
    std::string redis_server;
    bool redis_has_key = RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, redis_server);
    memolog::LogInfo("chat.private.route.debug", "checking redis for online status",
        {{"uid", uid_str}, {"redis_key_found", redis_has_key ? "true" : "false"},
         {"redis_server", redis_server}});

    if (!redis_has_key || redis_server.empty()) {
        // 尝试从 SERVER_ONLINE_USERS_SET 中查找
        redis_server = ResolveServerFromOnlineSetsLocal(uid_str);
        memolog::LogInfo("chat.private.route.debug", "checked online sets",
            {{"uid", uid_str}, {"found_server", redis_server}});
        if (redis_server.empty()) {
            memolog::LogInfo("chat.private.route.debug", "user not found in redis",
                {{"uid", uid_str}});
            return route;
        }
    }

    route.redis_server = redis_server;
    if (redis_server != self_name) {
        route.kind = OnlineRouteKind::Remote;
        memolog::LogInfo("chat.private.route.debug", "resolved to remote server",
            {{"uid", uid_str}, {"target_server", redis_server}});
        return route;
    }

    // 如果 Redis 中记录的是当前服务器，但本地没有会话，说明状态过期
    std::string reloaded_server;
    if (RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, reloaded_server) && !reloaded_server.empty()) {
        route.redis_server = reloaded_server;
        if (reloaded_server != self_name) {
            route.kind = OnlineRouteKind::Remote;
            return route;
        }
    }

    route.kind = OnlineRouteKind::Stale;
    ClearTrackedOnlineRouteLocal(uid, self_name);
    return route;
}

const char* RouteResultNameLocal(OnlineRouteKind kind) {
    switch (kind) {
    case OnlineRouteKind::Local:
        return "local";
    case OnlineRouteKind::Remote:
        return "remote";
    case OnlineRouteKind::Stale:
        return "stale";
    case OnlineRouteKind::Offline:
    default:
        return "offline";
    }
}

int64_t NowMsLocal() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string JsonToCompactStringLocal(const Json::Value& value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

bool ParseJsonObjectLocal(const std::string& payload, Json::Value& root) {
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errors;
    return reader->parse(payload.data(), payload.data() + payload.size(), &root, &errors) && root.isObject();
}

void LogPrivateRouteLocal(const std::string& event,
    int from_uid,
    int to_uid,
    const std::string& msg_id,
    const OnlineRouteDecision& route,
    const std::string& grpc_status,
    bool notify_delivered) {
    const auto fields = std::map<std::string, std::string>{
        {"from_uid", std::to_string(from_uid)},
        {"to_uid", std::to_string(to_uid)},
        {"msg_id", msg_id},
        {"redis_server", route.redis_server},
        {"route_result", RouteResultNameLocal(route.kind)},
        {"local_session_found", route.local_session_found ? "true" : "false"},
        {"grpc_status", grpc_status},
        {"notify_delivered", notify_delivered ? "true" : "false"}
    };
    if (route.kind == OnlineRouteKind::Stale || !notify_delivered) {
        memolog::LogWarn(event, "private message notify not delivered", fields);
        return;
    }
    memolog::LogInfo(event, "private message notify delivered", fields);
}

bool KafkaBackendEnabledLocal()
{
    return memochat::chatruntime::AsyncEventBusBackend() == "kafka";
}
}

PrivateMessageService::PrivateMessageService(LogicSystem& logic)
    : _logic(logic) {
}

void PrivateMessageService::HandleTextChatMessage(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Value root;
    ParseJsonObjectLocal(msg_data, root);

    const auto uid = root["fromuid"].asInt();
    const auto touid = root["touid"].asInt();
    const Json::Value arrays = root["text_array"];
    const bool kafka_backend = KafkaBackendEnabledLocal();
    const bool kafka_primary = kafka_backend && memochat::chatruntime::FeatureEnabled("chat_private_kafka_primary");
    const bool kafka_shadow = kafka_backend && !kafka_primary && memochat::chatruntime::FeatureEnabled("chat_private_kafka_shadow");

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = uid;
    rtvalue["touid"] = touid;

    Defer defer([&rtvalue, session]() {
        session->Send(JsonToCompactStringLocal(rtvalue), ID_TEXT_CHAT_MSG_RSP);
    });

    if (uid <= 0 || touid <= 0 || !arrays.isArray() || arrays.empty()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    Json::Value normalized(Json::arrayValue);
    std::vector<PrivateMessageInfo> pending_messages;
    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(uid);
    text_msg_req.set_touid(touid);
    const int conv_uid_min = std::min(uid, touid);
    const int conv_uid_max = std::max(uid, touid);
    std::string first_msg_id;

    for (const auto& txt_obj : arrays) {
        PrivateMessageInfo msg;
        msg.msg_id = txt_obj["msgid"].asString();
        msg.content = txt_obj["content"].asString();
        msg.conv_uid_min = conv_uid_min;
        msg.conv_uid_max = conv_uid_max;
        msg.from_uid = uid;
        msg.to_uid = touid;
        msg.reply_to_server_msg_id = txt_obj.get("reply_to_server_msg_id", 0).asInt64();
        msg.edited_at_ms = txt_obj.get("edited_at_ms", 0).asInt64();
        msg.deleted_at_ms = txt_obj.get("deleted_at_ms", 0).asInt64();
        msg.created_at = txt_obj.get("created_at", 0).asInt64();
        if (txt_obj.isMember("forward_meta")) {
            msg.forward_meta_json = JsonToCompactStringLocal(txt_obj["forward_meta"]);
        }
        if (msg.created_at <= 0) {
            msg.created_at = NowMsLocal();
        }
        if (msg.msg_id.empty() || msg.content.empty()) {
            rtvalue["error"] = ErrorCodes::Error_Json;
            return;
        }
        if (first_msg_id.empty()) {
            first_msg_id = msg.msg_id;
        }
        pending_messages.push_back(msg);

        Json::Value element;
        element["msgid"] = msg.msg_id;
        element["content"] = msg.content;
        element["created_at"] = static_cast<Json::Int64>(msg.created_at);
        if (msg.reply_to_server_msg_id > 0) {
            element["reply_to_server_msg_id"] = static_cast<Json::Int64>(msg.reply_to_server_msg_id);
        }
        if (!msg.forward_meta_json.empty()) {
            Json::Value forward_meta;
            if (ParseJsonObjectLocal(msg.forward_meta_json, forward_meta)) {
                element["forward_meta"] = forward_meta;
            }
        }
        if (msg.edited_at_ms > 0) {
            element["edited_at_ms"] = static_cast<Json::Int64>(msg.edited_at_ms);
        }
        if (msg.deleted_at_ms > 0) {
            element["deleted_at_ms"] = static_cast<Json::Int64>(msg.deleted_at_ms);
        }
        normalized.append(element);

        auto* text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(msg.msg_id);
        text_msg->set_msgcontent(msg.content);
    }

    const auto accept_ts = NowMsLocal();
    rtvalue["client_msg_id"] = first_msg_id;
    rtvalue["accept_node"] = memochat::chatruntime::SelfServerName();
    rtvalue["accept_ts"] = static_cast<Json::Int64>(accept_ts);
    rtvalue["status"] = kafka_primary ? "accepted" : "persisted";
    if (!kafka_primary) {
        rtvalue["text_array"] = normalized;
    }

    Json::Value event_payload;
    event_payload["fromuid"] = uid;
    event_payload["touid"] = touid;
    event_payload["trace_id"] = root.get("trace_id", "").asString();
    event_payload["request_id"] = root.get("request_id", "").asString();
    event_payload["span_id"] = root.get("span_id", "").asString();
    event_payload["event_id"] = first_msg_id;
    event_payload["accept_node"] = memochat::chatruntime::SelfServerName();
    event_payload["accept_ts"] = static_cast<Json::Int64>(accept_ts);
    event_payload["text_array"] = normalized;

    if (kafka_primary || kafka_shadow) {
        std::string publish_error;
        if (!_logic.PublishAsyncEvent(memochat::chatruntime::TopicPrivate(), event_payload, &publish_error)) {
            if (kafka_primary) {
                rtvalue["error"] = ErrorCodes::RPCFailed;
                rtvalue["status"] = "failed";
                return;
            }
            memolog::LogWarn("chat.private.shadow_publish_failed", "private shadow publish failed",
                { {"error", publish_error}, {"client_msg_id", first_msg_id} });
        }
    }

    if (kafka_primary) {
        return;
    }

    for (const auto& msg : pending_messages) {
        if (!PostgresMgr::GetInstance()->SavePrivateMessage(msg)) {
            rtvalue["error"] = ErrorCodes::RPCFailed;
            rtvalue["status"] = "failed";
            return;
        }
        if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SavePrivateMessage(msg)) {
            std::cerr << "[MongoMgr] SavePrivateMessage dual-write failed for msg_id=" << msg.msg_id << std::endl;
        }
    }

    const auto route = ResolveOnlineRouteLocal(touid);
    if (route.kind == OnlineRouteKind::Offline || route.kind == OnlineRouteKind::Stale) {
        LogPrivateRouteLocal("chat.private.route", uid, touid, first_msg_id, route, "skipped", false);
        return;
    }
    if (route.kind == OnlineRouteKind::Local && route.session) {
        route.session->Send(JsonToCompactStringLocal(rtvalue), ID_NOTIFY_TEXT_CHAT_MSG_REQ);
        LogPrivateRouteLocal("chat.private.route", uid, touid, first_msg_id, route, "n/a", true);
        return;
    }

    const auto notify_rsp = ChatGrpcClient::GetInstance()->NotifyTextChatMsg(route.redis_server, text_msg_req, rtvalue);
    if (notify_rsp.error() != ErrorCodes::Success) {
        if (notify_rsp.error() == ErrorCodes::TargetOffline) {
            ClearTrackedOnlineRouteLocal(touid, route.redis_server);
        }
        LogPrivateRouteLocal("chat.private.route", uid, touid, first_msg_id, route, std::to_string(notify_rsp.error()), false);
        return;
    }
    LogPrivateRouteLocal("chat.private.route", uid, touid, first_msg_id, route, std::to_string(notify_rsp.error()), true);
}

void PrivateMessageService::HandleForwardPrivateMessage(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int from_uid = root.get("fromuid", root.get("uid", 0)).asInt();
    const int peer_uid = root.get("peer_uid", root.get("touid", 0)).asInt();
    const std::string source_msg_id = root.get("msgid", "").asString();
    std::string client_msg_id = root.get("client_msg_id", "").asString();

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = from_uid;
    rtvalue["peer_uid"] = peer_uid;
    rtvalue["touid"] = peer_uid;
    if (!client_msg_id.empty()) {
        rtvalue["client_msg_id"] = client_msg_id;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(rtvalue.toStyledString(), ID_FORWARD_PRIVATE_MSG_RSP);
    });

    if (from_uid <= 0 || peer_uid <= 0 || source_msg_id.empty()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (!PostgresMgr::GetInstance()->IsFriend(from_uid, peer_uid)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::shared_ptr<PrivateMessageInfo> source_msg;
    bool mongo_success = false;
    if (MongoMgr::GetInstance()->Enabled()) {
        mongo_success = MongoMgr::GetInstance()->GetPrivateMessageByMsgId(source_msg_id, source_msg) && source_msg;
    }
    if (!mongo_success) {
        if (!(PostgresMgr::GetInstance()->GetPrivateMessageByMsgId(source_msg_id, source_msg) && source_msg)) {
            rtvalue["error"] = ErrorCodes::GroupNotFound;
            return;
        }
    }

    const int conv_uid_min = std::min(from_uid, peer_uid);
    const int conv_uid_max = std::max(from_uid, peer_uid);
    if (source_msg->conv_uid_min != conv_uid_min || source_msg->conv_uid_max != conv_uid_max) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    const int64_t now_ms = NowMsLocal();
    if (client_msg_id.empty()) {
        client_msg_id = std::to_string(from_uid) + "_" + std::to_string(now_ms);
        rtvalue["client_msg_id"] = client_msg_id;
    }

    PrivateMessageInfo info;
    info.msg_id = client_msg_id;
    info.conv_uid_min = conv_uid_min;
    info.conv_uid_max = conv_uid_max;
    info.from_uid = from_uid;
    info.to_uid = peer_uid;
    info.content = source_msg->content;
    info.reply_to_server_msg_id = source_msg->reply_to_server_msg_id;
    info.created_at = now_ms;

    Json::Value forward_meta;
    forward_meta["forwarded_from_msgid"] = source_msg_id;
    forward_meta["source_from_uid"] = source_msg->from_uid;
    forward_meta["source_conv_uid_min"] = source_msg->conv_uid_min;
    forward_meta["source_conv_uid_max"] = source_msg->conv_uid_max;
    forward_meta["source_created_at"] = static_cast<Json::Int64>(source_msg->created_at);
    if (!source_msg->forward_meta_json.empty()) {
        Json::Reader prev_forward_reader;
        Json::Value prev_forward_meta;
        if (prev_forward_reader.parse(source_msg->forward_meta_json, prev_forward_meta)) {
            forward_meta["prev_forward_meta"] = prev_forward_meta;
        }
    }
    info.forward_meta_json = forward_meta.toStyledString();

    if (!PostgresMgr::GetInstance()->SavePrivateMessage(info)) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }
    if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SavePrivateMessage(info)) {
        std::cerr << "[MongoMgr] SavePrivateMessage dual-write failed for msg_id=" << info.msg_id << std::endl;
    }

    Json::Value msg_obj;
    msg_obj["msgid"] = info.msg_id;
    msg_obj["content"] = info.content;
    msg_obj["created_at"] = static_cast<Json::Int64>(now_ms);
    if (info.reply_to_server_msg_id > 0) {
        msg_obj["reply_to_server_msg_id"] = static_cast<Json::Int64>(info.reply_to_server_msg_id);
    }
    msg_obj["forward_meta"] = forward_meta;
    rtvalue["msg"] = msg_obj;
    rtvalue["text_array"].append(msg_obj);
    rtvalue["created_at"] = static_cast<Json::Int64>(now_ms);

    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(from_uid);
    text_msg_req.set_touid(peer_uid);
    auto* text_msg = text_msg_req.add_textmsgs();
    text_msg->set_msgid(info.msg_id);
    text_msg->set_msgcontent(info.content);

    const auto route = ResolveOnlineRouteLocal(peer_uid);
    if (route.kind == OnlineRouteKind::Offline || route.kind == OnlineRouteKind::Stale) {
        LogPrivateRouteLocal("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, "skipped", false);
        return;
    }
    if (route.kind == OnlineRouteKind::Local && route.session) {
        route.session->Send(rtvalue.toStyledString(), ID_NOTIFY_TEXT_CHAT_MSG_REQ);
        LogPrivateRouteLocal("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, "n/a", true);
        return;
    }

    const auto notify_rsp = ChatGrpcClient::GetInstance()->NotifyTextChatMsg(route.redis_server, text_msg_req, rtvalue);
    if (notify_rsp.error() != ErrorCodes::Success) {
        if (notify_rsp.error() == ErrorCodes::TargetOffline) {
            ClearTrackedOnlineRouteLocal(peer_uid, route.redis_server);
        }
        LogPrivateRouteLocal("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, std::to_string(notify_rsp.error()), false);
        return;
    }
    LogPrivateRouteLocal("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, std::to_string(notify_rsp.error()), true);
}

void PrivateMessageService::HandlePrivateReadAck(const std::shared_ptr<CSession>&, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int uid = root.get("fromuid", root.get("uid", 0)).asInt();
    const int peer_uid = root.get("peer_uid", 0).asInt();
    int64_t read_ts = root.get("read_ts", 0).asInt64();
    if (uid <= 0 || peer_uid <= 0) {
        return;
    }
    if (!PostgresMgr::GetInstance()->IsFriend(uid, peer_uid)) {
        return;
    }
    if (read_ts <= 0) {
        read_ts = NowMsLocal();
    }
    PostgresMgr::GetInstance()->UpsertPrivateReadState(uid, peer_uid, read_ts);

    Json::Value notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "private_read_ack";
    notify["fromuid"] = uid;
    notify["peer_uid"] = peer_uid;
    notify["read_ts"] = static_cast<Json::Int64>(read_ts);
    _logic.MessageDelivery().PushPayload({ peer_uid }, ID_NOTIFY_PRIVATE_READ_ACK_REQ, notify);
}

void PrivateMessageService::HandleEditPrivateMessage(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int peer_uid = root["peer_uid"].asInt();
    const std::string target_msg_id = root.get("msgid", "").asString();
    const std::string content = root.get("content", "").asString();
    const int64_t now_ms = NowMsLocal();

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = uid;
    rtvalue["peer_uid"] = peer_uid;
    rtvalue["msgid"] = target_msg_id;
    rtvalue["content"] = content;
    rtvalue["edited_at_ms"] = static_cast<Json::Int64>(now_ms);
    Defer defer([&rtvalue, session]() {
        session->Send(rtvalue.toStyledString(), ID_EDIT_PRIVATE_MSG_RSP);
    });

    if (uid <= 0 || peer_uid <= 0 || target_msg_id.empty() || content.empty() || content.size() > 4096) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (!PostgresMgr::GetInstance()->IsFriend(uid, peer_uid)) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    if (!PostgresMgr::GetInstance()->UpdatePrivateMessageContent(uid, peer_uid, target_msg_id, content, now_ms)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }
    if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->UpdatePrivateMessageContent(uid, peer_uid, target_msg_id, content, now_ms)) {
        std::cerr << "[MongoMgr] UpdatePrivateMessageContent sync failed for msg_id=" << target_msg_id << std::endl;
    }

    Json::Value notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "private_msg_edited";
    notify["fromuid"] = uid;
    notify["peer_uid"] = peer_uid;
    notify["msgid"] = target_msg_id;
    notify["content"] = content;
    notify["edited_at_ms"] = static_cast<Json::Int64>(now_ms);
    _logic.MessageDelivery().PushPayload({ uid, peer_uid }, ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ, notify);
}

void PrivateMessageService::HandleRevokePrivateMessage(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int peer_uid = root["peer_uid"].asInt();
    const std::string target_msg_id = root.get("msgid", "").asString();
    const int64_t now_ms = NowMsLocal();

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = uid;
    rtvalue["peer_uid"] = peer_uid;
    rtvalue["msgid"] = target_msg_id;
    rtvalue["content"] = "[消息已撤回]";
    rtvalue["deleted_at_ms"] = static_cast<Json::Int64>(now_ms);
    Defer defer([&rtvalue, session]() {
        session->Send(rtvalue.toStyledString(), ID_REVOKE_PRIVATE_MSG_RSP);
    });

    if (uid <= 0 || peer_uid <= 0 || target_msg_id.empty()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (!PostgresMgr::GetInstance()->IsFriend(uid, peer_uid)) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    if (!PostgresMgr::GetInstance()->RevokePrivateMessage(uid, peer_uid, target_msg_id, now_ms)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }
    if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->RevokePrivateMessage(uid, peer_uid, target_msg_id, now_ms)) {
        std::cerr << "[MongoMgr] RevokePrivateMessage sync failed for msg_id=" << target_msg_id << std::endl;
    }

    Json::Value notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "private_msg_revoked";
    notify["fromuid"] = uid;
    notify["peer_uid"] = peer_uid;
    notify["msgid"] = target_msg_id;
    notify["content"] = "[消息已撤回]";
    notify["deleted_at_ms"] = static_cast<Json::Int64>(now_ms);
    _logic.MessageDelivery().PushPayload({ uid, peer_uid }, ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ, notify);
}

void PrivateMessageService::HandlePrivateHistory(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int peer_uid = root["peer_uid"].asInt();
    const int64_t before_ts = root.get("before_ts", 0).asInt64();
    const std::string before_msg_id = root.get("before_msg_id", "").asString();
    const int limit = root.get("limit", 20).asInt();

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["peer_uid"] = peer_uid;
    rtvalue["has_more"] = false;
    Defer defer([&rtvalue, session]() {
        session->Send(rtvalue.toStyledString(), ID_PRIVATE_HISTORY_RSP);
    });

    if (uid <= 0 || peer_uid <= 0 || limit <= 0) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    std::vector<std::shared_ptr<PrivateMessageInfo>> messages;
    bool has_more = false;
    bool mongo_success = false;
    bool pg_success = false;

    if (MongoMgr::GetInstance()->Enabled()) {
        mongo_success = MongoMgr::GetInstance()->GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more);
    }
    if (!mongo_success || messages.empty()) {
        pg_success = PostgresMgr::GetInstance()->GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more);
    }
    if (!mongo_success && !pg_success) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }
    rtvalue["has_more"] = has_more;
    int64_t max_peer_read_ts = 0;
    for (const auto& one : messages) {
        if (!one) {
            continue;
        }
        Json::Value item;
        item["msgid"] = one->msg_id;
        item["content"] = one->content;
        item["fromuid"] = one->from_uid;
        item["touid"] = one->to_uid;
        item["created_at"] = static_cast<Json::Int64>(one->created_at);
        if (one->reply_to_server_msg_id > 0) {
            item["reply_to_server_msg_id"] = static_cast<Json::Int64>(one->reply_to_server_msg_id);
        }
        if (!one->forward_meta_json.empty()) {
            Json::Reader forward_reader;
            Json::Value forward_meta;
            if (forward_reader.parse(one->forward_meta_json, forward_meta)) {
                item["forward_meta"] = forward_meta;
            }
        }
        if (one->edited_at_ms > 0) {
            item["edited_at_ms"] = static_cast<Json::Int64>(one->edited_at_ms);
        }
        if (one->deleted_at_ms > 0) {
            item["deleted_at_ms"] = static_cast<Json::Int64>(one->deleted_at_ms);
        }
        rtvalue["messages"].append(item);
        if (one->from_uid == peer_uid && one->created_at > max_peer_read_ts) {
            max_peer_read_ts = one->created_at;
        }
    }
    if (max_peer_read_ts > 0) {
        PostgresMgr::GetInstance()->UpsertPrivateReadState(uid, peer_uid, max_peer_read_ts);
    }
}
