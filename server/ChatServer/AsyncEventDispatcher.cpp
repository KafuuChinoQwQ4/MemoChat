#include "AsyncEventDispatcher.h"

#include "ChatGrpcClient.h"
#include "ChatAsyncEvent.h"
#include "ChatRuntime.h"
#include "ConfigMgr.h"
#include "CSession.h"
#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "IAsyncEventBus.h"
#include "cluster/ChatClusterDiscovery.h"
#include "logging/Logger.h"

#include <algorithm>
#include <chrono>
#include <json/json.h>
#include <thread>
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

std::string TrimCopyAsync(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return std::string();
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string ServerOnlineUsersKeyAsync(const std::string& server_name) {
    return std::string(SERVER_ONLINE_USERS_PREFIX) + server_name;
}

std::vector<std::string> KnownChatServerNamesAsync() {
    std::vector<std::string> servers;
    auto& cfg = ConfigMgr::Inst();
    static const auto cluster = memochat::cluster::LoadStaticChatClusterConfig(
        [&cfg](const std::string& section, const std::string& key) {
            return cfg.GetValue(section, key);
        },
        TrimCopyAsync(cfg["SelfServer"]["Name"]));
    for (const auto& node : cluster.enabledNodes()) {
        servers.push_back(node.name);
    }
    return servers;
}

void RepairOnlineRouteStateAsync(int uid, const std::shared_ptr<CSession>& session, const std::string& server_name) {
    if (uid <= 0 || !session || server_name.empty()) {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RedisMgr::GetInstance()->Set(USERIPPREFIX + uid_str, server_name);
    RedisMgr::GetInstance()->Set(USER_SESSION_PREFIX + uid_str, session->GetSessionId());
    RedisMgr::GetInstance()->SAdd(ServerOnlineUsersKeyAsync(server_name), uid_str);
}

std::string ResolveServerFromOnlineSetsAsync(const std::string& uid_str) {
    if (uid_str.empty()) {
        return std::string();
    }

    for (const auto& server_name : KnownChatServerNamesAsync()) {
        std::vector<std::string> online_uids;
        RedisMgr::GetInstance()->SMembers(ServerOnlineUsersKeyAsync(server_name), online_uids);
        if (std::find(online_uids.begin(), online_uids.end(), uid_str) != online_uids.end()) {
            return server_name;
        }
    }
    return std::string();
}

void ClearTrackedOnlineRouteAsync(int uid, const std::string& server_name) {
    if (uid <= 0 || server_name.empty()) {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
    RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
    RedisMgr::GetInstance()->SRem(ServerOnlineUsersKeyAsync(server_name), uid_str);
}

OnlineRouteDecision ResolveOnlineRouteAsync(int uid) {
    OnlineRouteDecision route;
    if (uid <= 0) {
        return route;
    }

    const auto self_name = ConfigMgr::Inst()["SelfServer"]["Name"];
    auto session = UserMgr::GetInstance()->GetSession(uid);
    if (session) {
        route.kind = OnlineRouteKind::Local;
        route.session = session;
        route.redis_server = self_name;
        route.local_session_found = true;
        RepairOnlineRouteStateAsync(uid, session, self_name);
        return route;
    }

    const auto uid_str = std::to_string(uid);
    std::string redis_server;
    if (!RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, redis_server) || redis_server.empty()) {
        redis_server = ResolveServerFromOnlineSetsAsync(uid_str);
        if (redis_server.empty()) {
            return route;
        }
    }
    route.redis_server = redis_server;
    if (redis_server != self_name) {
        route.kind = OnlineRouteKind::Remote;
        return route;
    }

    std::string reloaded_server;
    if (RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, reloaded_server) && !reloaded_server.empty()) {
        route.redis_server = reloaded_server;
        if (reloaded_server != self_name) {
            route.kind = OnlineRouteKind::Remote;
            return route;
        }
    }

    route.kind = OnlineRouteKind::Stale;
    ClearTrackedOnlineRouteAsync(uid, self_name);
    return route;
}

const char* RouteResultNameAsync(OnlineRouteKind kind) {
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

int64_t NowMsAsync() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string JsonToCompactStringAsync(const Json::Value& value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

void InvalidateRelationBootstrapCacheAsync(int uid) {
    if (uid <= 0) {
        return;
    }
    RedisMgr::GetInstance()->Del(std::string("relation_bootstrap_") + std::to_string(uid));
}

void AppendUniqueUidAsync(std::vector<int>& values, int uid) {
    if (uid <= 0) {
        return;
    }
    if (std::find(values.begin(), values.end(), uid) == values.end()) {
        values.push_back(uid);
    }
}

bool ParseJsonObjectAsync(const std::string& payload, Json::Value& root) {
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errors;
    return reader->parse(payload.data(), payload.data() + payload.size(), &root, &errors) && root.isObject();
}

void LogPrivateRouteAsync(const std::string& event,
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
        {"route_result", RouteResultNameAsync(route.kind)},
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
}

AsyncEventDispatcher::AsyncEventDispatcher(std::shared_ptr<IAsyncEventBus> event_bus,
    StopRequested stop_requested,
    PushGroupPayloadFn push_group_payload)
    : _event_bus(std::move(event_bus)),
      _stop_requested(std::move(stop_requested)),
      _push_group_payload(std::move(push_group_payload)) {
}

bool AsyncEventDispatcher::PublishAsyncEvent(const std::string& topic, const Json::Value& payload, std::string* error)
{
    if (!_event_bus) {
        if (error) {
            *error = "event_bus_unavailable";
        }
        return false;
    }
    return _event_bus->Publish(topic, payload, error);
}

void AsyncEventDispatcher::DealAsyncEvents()
{
    while (!_stop_requested()) {
        if (!memochat::chatruntime::IsWorkerEnabled()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        AsyncConsumedEvent event;
        std::string consume_error;
        const bool handled = _event_bus &&
            _event_bus->ConsumeOnce({
                memochat::chatruntime::TopicPrivate(),
                memochat::chatruntime::TopicGroup(),
                memochat::chatruntime::TopicDialogSync(),
                memochat::chatruntime::TopicRelationState()
            }, event, &consume_error);
        if (handled) {
            if (!event.parsed) {
                memolog::LogWarn("chat.async.invalid_event", "async chat event parse failed",
                    { {"topic", event.envelope.topic}, {"error", consume_error} });
                _event_bus->AckLastConsumed();
            } else if (event.envelope.topic == memochat::chatruntime::TopicPrivate()) {
                HandlePrivateAsyncEvent(event.envelope.payload());
                _event_bus->AckLastConsumed();
            } else if (event.envelope.topic == memochat::chatruntime::TopicGroup()) {
                HandleGroupAsyncEvent(event.envelope.payload());
                _event_bus->AckLastConsumed();
            } else if (event.envelope.topic == memochat::chatruntime::TopicDialogSync()) {
                HandleDialogSyncEvent(event.envelope.payload());
                _event_bus->AckLastConsumed();
            } else if (event.envelope.topic == memochat::chatruntime::TopicRelationState()) {
                HandleRelationStateEvent(event.envelope.payload());
                _event_bus->AckLastConsumed();
            } else {
                memolog::LogWarn("chat.async.unknown_topic", "async chat event topic is not registered", { {"topic", event.envelope.topic} });
                _event_bus->AckLastConsumed();
            }
        }
        if (!handled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void AsyncEventDispatcher::NotifyMessageStatus(const Json::Value& payload)
{
    const int uid = payload.get("fromuid", 0).asInt();
    if (uid <= 0) {
        return;
    }
    const auto route = ResolveOnlineRouteAsync(uid);
    const auto serialized = JsonToCompactStringAsync(payload);
    if (route.kind == OnlineRouteKind::Local && route.session) {
        route.session->Send(serialized, ID_NOTIFY_MSG_STATUS_REQ);
        return;
    }
    if (route.kind == OnlineRouteKind::Remote) {
        GroupMessageNotifyReq req;
        req.add_touids(uid);
        req.set_tcp_msgid(ID_NOTIFY_MSG_STATUS_REQ);
        req.set_payload_json(serialized);
        ChatGrpcClient::GetInstance()->NotifyGroupMessage(route.redis_server, req);
    }
}

void AsyncEventDispatcher::HandlePrivateAsyncEvent(const Json::Value& root)
{
    if (!root.isObject()) {
        return;
    }
    const int from_uid = root.get("fromuid", 0).asInt();
    const int to_uid = root.get("touid", 0).asInt();
    const Json::Value text_array = root["text_array"];
    if (from_uid <= 0 || to_uid <= 0 || !text_array.isArray() || text_array.empty()) {
        return;
    }

    const int conv_uid_min = std::min(from_uid, to_uid);
    const int conv_uid_max = std::max(from_uid, to_uid);
    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(from_uid);
    text_msg_req.set_touid(to_uid);
    Json::Value notify_payload;
    notify_payload["error"] = ErrorCodes::Success;
    notify_payload["fromuid"] = from_uid;
    notify_payload["touid"] = to_uid;
    notify_payload["text_array"] = Json::arrayValue;
    std::string first_msg_id;

    for (const auto& txt_obj : text_array) {
        PrivateMessageInfo msg;
        msg.msg_id = txt_obj.get("msgid", "").asString();
        msg.content = txt_obj.get("content", "").asString();
        msg.conv_uid_min = conv_uid_min;
        msg.conv_uid_max = conv_uid_max;
        msg.from_uid = from_uid;
        msg.to_uid = to_uid;
        msg.created_at = txt_obj.get("created_at", 0).asInt64();
        msg.reply_to_server_msg_id = txt_obj.get("reply_to_server_msg_id", 0).asInt64();
        msg.edited_at_ms = txt_obj.get("edited_at_ms", 0).asInt64();
        msg.deleted_at_ms = txt_obj.get("deleted_at_ms", 0).asInt64();
        if (msg.msg_id.empty() || msg.content.empty()) {
            continue;
        }
        if (msg.created_at <= 0) {
            msg.created_at = NowMsAsync();
        }
        if (txt_obj.isMember("forward_meta")) {
            msg.forward_meta_json = JsonToCompactStringAsync(txt_obj["forward_meta"]);
        }
        if (!PostgresMgr::GetInstance()->SavePrivateMessage(msg)) {
            continue;
        }
        if (MongoMgr::GetInstance()->Enabled()) {
            MongoMgr::GetInstance()->SavePrivateMessage(msg);
        }
        if (first_msg_id.empty()) {
            first_msg_id = msg.msg_id;
        }
        auto* text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(msg.msg_id);
        text_msg->set_msgcontent(msg.content);

        Json::Value one;
        one["msgid"] = msg.msg_id;
        one["content"] = msg.content;
        one["created_at"] = static_cast<Json::Int64>(msg.created_at);
        if (msg.reply_to_server_msg_id > 0) {
            one["reply_to_server_msg_id"] = static_cast<Json::Int64>(msg.reply_to_server_msg_id);
        }
        notify_payload["text_array"].append(one);
    }

    const auto route = ResolveOnlineRouteAsync(to_uid);
    if (route.kind == OnlineRouteKind::Local && route.session) {
        route.session->Send(JsonToCompactStringAsync(notify_payload), ID_NOTIFY_TEXT_CHAT_MSG_REQ);
        LogPrivateRouteAsync("chat.private.route.async", from_uid, to_uid, first_msg_id, route, "n/a", true);
    } else if (route.kind == OnlineRouteKind::Remote) {
        const auto notify_rsp = ChatGrpcClient::GetInstance()->NotifyTextChatMsg(route.redis_server, text_msg_req, notify_payload);
        LogPrivateRouteAsync("chat.private.route.async", from_uid, to_uid, first_msg_id, route, std::to_string(notify_rsp.error()), notify_rsp.error() == ErrorCodes::Success);
    } else {
        LogPrivateRouteAsync("chat.private.route.async", from_uid, to_uid, first_msg_id, route, "skipped", false);
    }

    Json::Value status_payload;
    status_payload["error"] = ErrorCodes::Success;
    status_payload["scope"] = "private";
    status_payload["fromuid"] = from_uid;
    status_payload["touid"] = to_uid;
    status_payload["peer_uid"] = to_uid;
    status_payload["client_msg_id"] = first_msg_id;
    status_payload["status"] = "persisted";
    status_payload["persist_ts"] = static_cast<Json::Int64>(NowMsAsync());
    status_payload["text_array"] = notify_payload["text_array"];
    NotifyMessageStatus(status_payload);

    Json::Value dialog_sync(Json::objectValue);
    dialog_sync["scope"] = "private";
    dialog_sync["fromuid"] = from_uid;
    dialog_sync["touid"] = to_uid;
    dialog_sync["client_msg_id"] = first_msg_id;
    dialog_sync["event_ts"] = static_cast<Json::Int64>(NowMsAsync());
    dialog_sync["text_array"] = notify_payload["text_array"];
    std::string publish_error;
    if (!first_msg_id.empty() && !PublishAsyncEvent(memochat::chatruntime::TopicDialogSync(), dialog_sync, &publish_error)) {
        memolog::LogWarn("chat.dialog_sync.publish_failed", "failed to publish private dialog sync event",
            {
                {"from_uid", std::to_string(from_uid)},
                {"to_uid", std::to_string(to_uid)},
                {"client_msg_id", first_msg_id},
                {"error", publish_error}
            });
    }
}

void AsyncEventDispatcher::HandleGroupAsyncEvent(const Json::Value& root)
{
    if (!root.isObject()) {
        return;
    }
    const int from_uid = root.get("fromuid", 0).asInt();
    const int64_t group_id = root.get("groupid", 0).asInt64();
    Json::Value msg = root["msg"];
    if (from_uid <= 0 || group_id <= 0 || !msg.isObject()) {
        return;
    }

    GroupMessageInfo info;
    info.msg_id = msg.get("msgid", "").asString();
    info.group_id = group_id;
    info.from_uid = from_uid;
    info.msg_type = msg.get("msgtype", "text").asString();
    info.content = msg.get("content", "").asString();
    info.mentions_json = msg.get("mentions", Json::arrayValue).toStyledString();
    info.file_name = msg.get("file_name", "").asString();
    info.mime = msg.get("mime", "").asString();
    info.size = msg.get("size", 0).asInt();
    info.reply_to_server_msg_id = msg.get("reply_to_server_msg_id", 0).asInt64();
    info.edited_at_ms = msg.get("edited_at_ms", 0).asInt64();
    info.deleted_at_ms = msg.get("deleted_at_ms", 0).asInt64();
    info.created_at = NowMsAsync();
    if (msg.isMember("forward_meta")) {
        info.forward_meta_json = JsonToCompactStringAsync(msg["forward_meta"]);
    }
    if (info.msg_id.empty() || info.content.empty()) {
        return;
    }

    int64_t server_msg_id = 0;
    int64_t group_seq = 0;
    if (!PostgresMgr::GetInstance()->SaveGroupMessage(info, &server_msg_id, &group_seq)) {
        return;
    }
    info.server_msg_id = server_msg_id;
    info.group_seq = group_seq;

    auto sender_info = PostgresMgr::GetInstance()->GetUser(from_uid);
    if (sender_info) {
        info.from_name = sender_info->name;
        info.from_nick = sender_info->nick;
        info.from_icon = sender_info->icon;
    }
    if (MongoMgr::GetInstance()->Enabled()) {
        MongoMgr::GetInstance()->SaveGroupMessage(info);
    }

    Json::Value notify_payload;
    notify_payload["error"] = ErrorCodes::Success;
    notify_payload["fromuid"] = from_uid;
    notify_payload["groupid"] = static_cast<Json::Int64>(group_id);
    notify_payload["client_msg_id"] = info.msg_id;
    notify_payload["created_at"] = static_cast<Json::Int64>(info.created_at);
    notify_payload["server_msg_id"] = static_cast<Json::Int64>(info.server_msg_id);
    notify_payload["group_seq"] = static_cast<Json::Int64>(info.group_seq);
    notify_payload["msg"] = msg;
    notify_payload["msg"]["created_at"] = static_cast<Json::Int64>(info.created_at);
    notify_payload["msg"]["server_msg_id"] = static_cast<Json::Int64>(info.server_msg_id);
    notify_payload["msg"]["group_seq"] = static_cast<Json::Int64>(info.group_seq);
    if (sender_info) {
        notify_payload["from_name"] = sender_info->name;
        notify_payload["from_nick"] = sender_info->nick;
        notify_payload["from_icon"] = sender_info->icon;
        notify_payload["from_user_id"] = sender_info->user_id;
    }
    std::shared_ptr<GroupInfo> group_info;
    if (PostgresMgr::GetInstance()->GetGroupById(group_id, group_info) && group_info) {
        notify_payload["group_code"] = group_info->group_code;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members) {
        if (member && member->status == 1) {
            recipients.push_back(member->uid);
        }
    }
    _push_group_payload(recipients, ID_NOTIFY_GROUP_CHAT_MSG_REQ, notify_payload, from_uid);

    Json::Value status_payload;
    status_payload["error"] = ErrorCodes::Success;
    status_payload["scope"] = "group";
    status_payload["fromuid"] = from_uid;
    status_payload["groupid"] = static_cast<Json::Int64>(group_id);
    status_payload["client_msg_id"] = info.msg_id;
    status_payload["server_msg_id"] = static_cast<Json::Int64>(info.server_msg_id);
    status_payload["group_seq"] = static_cast<Json::Int64>(info.group_seq);
    status_payload["status"] = "persisted";
    status_payload["persist_ts"] = static_cast<Json::Int64>(NowMsAsync());
    status_payload["msg"] = notify_payload["msg"];
    if (sender_info) {
        status_payload["from_name"] = sender_info->name;
        status_payload["from_nick"] = sender_info->nick;
        status_payload["from_icon"] = sender_info->icon;
    }
    NotifyMessageStatus(status_payload);

    Json::Value dialog_sync(Json::objectValue);
    dialog_sync["scope"] = "group";
    dialog_sync["fromuid"] = from_uid;
    dialog_sync["groupid"] = static_cast<Json::Int64>(group_id);
    dialog_sync["client_msg_id"] = info.msg_id;
    dialog_sync["server_msg_id"] = static_cast<Json::Int64>(info.server_msg_id);
    dialog_sync["group_seq"] = static_cast<Json::Int64>(info.group_seq);
    dialog_sync["event_ts"] = static_cast<Json::Int64>(NowMsAsync());
    dialog_sync["msg"] = notify_payload["msg"];
    std::string publish_error;
    if (!info.msg_id.empty() && !PublishAsyncEvent(memochat::chatruntime::TopicDialogSync(), dialog_sync, &publish_error)) {
        memolog::LogWarn("chat.dialog_sync.publish_failed", "failed to publish group dialog sync event",
            {
                {"from_uid", std::to_string(from_uid)},
                {"group_id", std::to_string(group_id)},
                {"client_msg_id", info.msg_id},
                {"error", publish_error}
            });
    }
}

void AsyncEventDispatcher::HandleDialogSyncEvent(const Json::Value& root)
{
    if (!root.isObject()) {
        return;
    }

    std::vector<int> owner_uids;
    const std::string scope = root.get("scope", "").asString();
    if (scope == "private") {
        AppendUniqueUidAsync(owner_uids, root.get("fromuid", 0).asInt());
        AppendUniqueUidAsync(owner_uids, root.get("touid", 0).asInt());
    } else if (scope == "group") {
        const int from_uid = root.get("fromuid", 0).asInt();
        const int64_t group_id = root.get("groupid", 0).asInt64();
        AppendUniqueUidAsync(owner_uids, from_uid);
        if (group_id > 0) {
            std::vector<std::shared_ptr<GroupMemberInfo>> members;
            PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
            for (const auto& member : members) {
                if (member && member->status == 1) {
                    AppendUniqueUidAsync(owner_uids, member->uid);
                }
            }
        }
    }

    for (const auto owner_uid : owner_uids) {
        if (!PostgresMgr::GetInstance()->RefreshDialogsForOwner(owner_uid)) {
            memolog::LogWarn("chat.dialog_sync.refresh_failed", "failed to refresh dialog runtime",
                {
                    {"scope", scope},
                    {"owner_uid", std::to_string(owner_uid)},
                    {"client_msg_id", root.get("client_msg_id", "").asString()}
                });
        }
    }
}

void AsyncEventDispatcher::HandleRelationStateEvent(const Json::Value& root)
{
    if (!root.isObject()) {
        return;
    }

    std::vector<int> affected_uids;
    if (root.isMember("uids") && root["uids"].isArray()) {
        for (const auto& uid_value : root["uids"]) {
            AppendUniqueUidAsync(affected_uids, uid_value.asInt());
        }
    } else {
        AppendUniqueUidAsync(affected_uids, root.get("uid", 0).asInt());
        AppendUniqueUidAsync(affected_uids, root.get("touid", 0).asInt());
        AppendUniqueUidAsync(affected_uids, root.get("recipient_uid", 0).asInt());
    }

    for (const auto uid : affected_uids) {
        InvalidateRelationBootstrapCacheAsync(uid);
        if (!PostgresMgr::GetInstance()->RefreshDialogsForOwner(uid)) {
            memolog::LogWarn("chat.relation_state.refresh_failed", "failed to refresh dialogs after relation event",
                {
                    {"uid", std::to_string(uid)},
                    {"event_type", root.get("event_type", "").asString()}
                });
        }
    }
}
