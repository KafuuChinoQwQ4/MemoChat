#include "ChatRelationService.h"

#include "ChatGrpcClient.h"
#include "ChatRuntime.h"
#include "ConfigMgr.h"
#include "CSession.h"
#include "ChatUserSupport.h"
#include "LogicSystem.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "logging/Logger.h"

#include <chrono>
#include <json/json.h>
#include <unordered_map>

namespace {
int64_t NowMsRelationLocal() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string RelationBootstrapCacheKeyLocal(int uid) {
    return std::string("relation_bootstrap_") + std::to_string(uid);
}

void InvalidateRelationBootstrapCacheLocal(int uid) {
    if (uid > 0) {
        RedisMgr::GetInstance()->Del(RelationBootstrapCacheKeyLocal(uid));
    }
}

// Compact wire JSON for TCP/QUIC transport (Qt QJsonDocument is strict).
std::string JsonToWireString(const Json::Value& v) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, v);
}

bool TryAppendCachedRelationBootstrapJsonLocal(int uid, Json::Value& out) {
    if (uid <= 0) {
        return false;
    }

    std::string payload;
    if (!RedisMgr::GetInstance()->Get(RelationBootstrapCacheKeyLocal(uid), payload) || payload.empty()) {
        return false;
    }

    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value cached;
    std::string errors;
    if (!reader->parse(payload.data(), payload.data() + payload.size(), &cached, &errors) || !cached.isObject()) {
        return false;
    }

    if (cached.isMember("apply_list")) {
        out["apply_list"] = cached["apply_list"];
    }
    if (cached.isMember("friend_list")) {
        out["friend_list"] = cached["friend_list"];
    }
    return true;
}

int RelationBootstrapCacheTtlSecLocal() {
    auto& cfg = ConfigMgr::Inst();
    const auto raw = cfg.GetValue("RelationBootstrapCache", "TtlSec");
    if (raw.empty()) {
        return 15;
    }
    try {
        return std::max(1, std::stoi(raw));
    } catch (...) {
        return 15;
    }
}

void CacheRelationBootstrapJsonLocal(int uid, const Json::Value& payload) {
    if (uid <= 0 || !payload.isObject()) {
        return;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const auto json = Json::writeString(builder, payload);
    if (json.empty()) {
        return;
    }

    RedisMgr::GetInstance()->SetEx(RelationBootstrapCacheKeyLocal(uid), json, RelationBootstrapCacheTtlSecLocal());
}

void PublishRelationStateEventLocal(LogicSystem& logic,
    const std::string& event_type,
    int uid,
    int touid,
    const std::vector<std::string>& labels)
{
    Json::Value event_payload(Json::objectValue);
    event_payload["event_type"] = event_type;
    event_payload["uid"] = uid;
    event_payload["touid"] = touid;
    event_payload["uids"].append(uid);
    event_payload["uids"].append(touid);
    event_payload["event_ts"] = static_cast<Json::Int64>(NowMsRelationLocal());
    for (const auto& label : labels) {
        event_payload["labels"].append(label);
    }
    std::string publish_error;
    if (!logic.PublishAsyncEvent(memochat::chatruntime::TopicRelationState(), event_payload, &publish_error)) {
        memolog::LogWarn("chat.relation_state.publish_failed", "failed to publish relation state event",
            {
                {"event_type", event_type},
                {"uid", std::to_string(uid)},
                {"touid", std::to_string(touid)},
                {"error", publish_error}
            });
    }
}
}

ChatRelationService::ChatRelationService(LogicSystem& logic)
    : _logic(logic) {
}

void ChatRelationService::AppendRelationBootstrapJson(int uid, Json::Value& out)
{
    const auto bootstrap_start_ms = NowMsRelationLocal();
    if (TryAppendCachedRelationBootstrapJsonLocal(uid, out)) {
        memolog::LogInfo("chat.relation_bootstrap.cache_hit", "relation bootstrap served from cache",
            {{"uid", std::to_string(uid)},
             {"relation_bootstrap_ms", std::to_string(NowMsRelationLocal() - bootstrap_start_ms)}});
        return;
    }

    Json::Value cache_payload(Json::objectValue);
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    if (chatusersupport::GetFriendApplyInfo(uid, apply_list)) {
        for (auto& apply : apply_list) {
            Json::Value obj;
            obj["name"] = apply->_name;
            obj["uid"] = apply->_uid;
            obj["user_id"] = apply->_user_id;
            obj["icon"] = apply->_icon;
            obj["nick"] = apply->_nick;
            obj["sex"] = apply->_sex;
            obj["desc"] = apply->_desc;
            obj["status"] = apply->_status;
            for (const auto& tag : apply->_labels) {
                obj["labels"].append(tag);
            }
            out["apply_list"].append(obj);
            cache_payload["apply_list"].append(obj);
        }
    }

    std::vector<std::shared_ptr<UserInfo>> friend_list;
    if (chatusersupport::GetFriendList(uid, friend_list)) {
        for (auto& friend_ele : friend_list) {
            Json::Value obj;
            obj["name"] = friend_ele->name;
            obj["uid"] = friend_ele->uid;
            obj["user_id"] = friend_ele->user_id;
            obj["icon"] = friend_ele->icon;
            obj["nick"] = friend_ele->nick;
            obj["sex"] = friend_ele->sex;
            obj["desc"] = friend_ele->desc;
            obj["back"] = friend_ele->back;
            for (const auto& tag : friend_ele->labels) {
                obj["labels"].append(tag);
            }
            out["friend_list"].append(obj);
            cache_payload["friend_list"].append(obj);
        }
    }

    CacheRelationBootstrapJsonLocal(uid, cache_payload);
    memolog::LogInfo("chat.relation_bootstrap.cache_fill", "relation bootstrap cache populated",
        {{"uid", std::to_string(uid)},
         {"relation_bootstrap_ms", std::to_string(NowMsRelationLocal() - bootstrap_start_ms)}});
}

void ChatRelationService::BuildDialogListJson(int uid, Json::Value& out)
{
    PostgresMgr::GetInstance()->RefreshDialogsForOwner(uid);

    std::unordered_map<int, std::shared_ptr<DialogMetaInfo>> private_meta;
    std::unordered_map<int64_t, std::shared_ptr<DialogMetaInfo>> group_meta;
    std::vector<std::shared_ptr<DialogMetaInfo>> meta_list;
    if (PostgresMgr::GetInstance()->GetDialogMetaByOwner(uid, meta_list)) {
        for (const auto& meta : meta_list) {
            if (!meta) {
                continue;
            }
            if (meta->dialog_type == "private" && meta->peer_uid > 0) {
                private_meta[meta->peer_uid] = meta;
                continue;
            }
            if (meta->dialog_type == "group" && meta->group_id > 0) {
                group_meta[meta->group_id] = meta;
            }
        }
    }

    std::vector<std::shared_ptr<UserInfo>> friend_list;
    chatusersupport::GetFriendList(uid, friend_list);
    for (const auto& peer : friend_list) {
        if (!peer) {
            continue;
        }
        const auto meta_it = private_meta.find(peer->uid);
        Json::Value one;
        one["dialog_id"] = std::string("u_") + std::to_string(peer->uid);
        one["dialog_type"] = "private";
        one["peer_uid"] = peer->uid;
        one["title"] = peer->nick.empty() ? peer->name : peer->nick;
        one["avatar"] = peer->icon;
        DialogRuntimeInfo runtime;
        if (!PostgresMgr::GetInstance()->GetPrivateDialogRuntime(uid, peer->uid, runtime)) {
            runtime = DialogRuntimeInfo();
        }
        one["last_msg_preview"] = runtime.last_msg_preview;
        one["last_msg_ts"] = static_cast<Json::Int64>(runtime.last_msg_ts);
        one["unread_count"] = runtime.unread_count;
        one["pinned_rank"] = (meta_it == private_meta.end()) ? 0 : meta_it->second->pinned_rank;
        one["draft_text"] = (meta_it == private_meta.end()) ? "" : meta_it->second->draft_text;
        one["mute_state"] = (meta_it == private_meta.end()) ? 0 : meta_it->second->mute_state;
        out["dialogs"].append(one);
    }

    std::vector<std::shared_ptr<GroupInfo>> groups;
    PostgresMgr::GetInstance()->GetUserGroupList(uid, groups);
    for (const auto& group : groups) {
        if (!group) {
            continue;
        }
        const auto meta_it = group_meta.find(group->group_id);
        Json::Value one;
        one["dialog_id"] = std::string("g_") + std::to_string(group->group_id);
        one["dialog_type"] = "group";
        one["group_id"] = static_cast<Json::Int64>(group->group_id);
        one["title"] = group->name;
        one["avatar"] = group->icon;
        DialogRuntimeInfo runtime;
        if (!PostgresMgr::GetInstance()->GetGroupDialogRuntime(uid, group->group_id, runtime)) {
            runtime = DialogRuntimeInfo();
        }
        one["last_msg_preview"] = runtime.last_msg_preview;
        one["last_msg_ts"] = static_cast<Json::Int64>(runtime.last_msg_ts);
        one["unread_count"] = runtime.unread_count;
        one["pinned_rank"] = (meta_it == group_meta.end()) ? 0 : meta_it->second->pinned_rank;
        one["draft_text"] = (meta_it == group_meta.end()) ? "" : meta_it->second->draft_text;
        one["mute_state"] = (meta_it == group_meta.end()) ? 0 : meta_it->second->mute_state;
        out["dialogs"].append(one);
    }
}

void ChatRelationService::HandleSearchUser(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const std::string user_id = root["user_id"].asString();
    Json::Value rtvalue;
    Defer defer([&rtvalue, session]() { session->Send(JsonToWireString(rtvalue), ID_SEARCH_USER_RSP); });
    if (!root.isMember("user_id") || user_id.empty()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    int uid = 0;
    if (!PostgresMgr::GetInstance()->GetUidByUserId(user_id, uid) || uid <= 0) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    chatusersupport::GetUserByUid(std::to_string(uid), rtvalue);
}

void ChatRelationService::HandleAddFriendApply(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto applyname = root["applyname"].asString();
    auto touid = root["touid"].asInt();
    std::vector<std::string> labels;
    if (root.isMember("labels") && root["labels"].isArray()) {
        for (const auto& one : root["labels"]) {
            labels.push_back(one.asString());
        }
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([&rtvalue, session]() { session->Send(JsonToWireString(rtvalue), ID_ADD_FRIEND_RSP); });

    PostgresMgr::GetInstance()->AddFriendApply(uid, touid);
    PostgresMgr::GetInstance()->ReplaceApplyTags(uid, touid, labels);
    InvalidateRelationBootstrapCacheLocal(touid);
    auto apply_info = std::make_shared<UserInfo>();
    bool b_info = chatusersupport::GetBaseInfo(USER_BASE_INFO + std::to_string(uid), uid, apply_info);
    Json::Value notify;
    notify["error"] = ErrorCodes::Success;
    notify["applyuid"] = uid;
    notify["name"] = applyname;
    notify["desc"] = "";
    if (b_info) {
        notify["icon"] = apply_info->icon;
        notify["sex"] = apply_info->sex;
        notify["nick"] = apply_info->nick;
        notify["user_id"] = apply_info->user_id;
    }
    Json::Value task_payload;
    task_payload["recipient_uid"] = touid;
    task_payload["msgid"] = ID_NOTIFY_ADD_FRIEND_REQ;
    task_payload["exclude_uid"] = 0;
    task_payload["payload"] = notify;
    std::string publish_error;
    _logic.PublishTask("relation_notify",
        memochat::chatruntime::TaskRoutingRelationNotify(),
        task_payload,
        0,
        memochat::chatruntime::TaskMaxRetries(),
        &publish_error);
    PublishRelationStateEventLocal(_logic, "friend_apply_created", uid, touid, labels);
}

void ChatRelationService::HandleAuthFriendApply(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    auto uid = root["fromuid"].asInt();
    auto touid = root["touid"].asInt();
    auto back_name = root["back"].asString();
    std::vector<std::string> labels;
    if (root.isMember("labels") && root["labels"].isArray()) {
        for (const auto& one : root["labels"]) {
            labels.push_back(one.asString());
        }
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    auto user_info = std::make_shared<UserInfo>();
    bool b_info = chatusersupport::GetBaseInfo(USER_BASE_INFO + std::to_string(touid), touid, user_info);
    if (b_info) {
        rtvalue["name"] = user_info->name;
        rtvalue["nick"] = user_info->nick;
        rtvalue["icon"] = user_info->icon;
        rtvalue["sex"] = user_info->sex;
        rtvalue["uid"] = touid;
        rtvalue["user_id"] = user_info->user_id;
    } else {
        rtvalue["error"] = ErrorCodes::UidInvalid;
    }
    Defer defer([&rtvalue, session]() { session->Send(JsonToWireString(rtvalue), ID_AUTH_FRIEND_RSP); });

    PostgresMgr::GetInstance()->AuthFriendApply(uid, touid);
    PostgresMgr::GetInstance()->AddFriend(uid, touid, back_name);
    PostgresMgr::GetInstance()->ReplaceFriendTags(uid, touid, labels);
    auto requesterApplyTags = PostgresMgr::GetInstance()->GetApplyTags(touid, uid);
    PostgresMgr::GetInstance()->ReplaceFriendTags(touid, uid, requesterApplyTags);
    InvalidateRelationBootstrapCacheLocal(uid);
    InvalidateRelationBootstrapCacheLocal(touid);
    Json::Value notify;
    notify["error"] = ErrorCodes::Success;
    notify["fromuid"] = uid;
    notify["touid"] = touid;
    auto current_user = std::make_shared<UserInfo>();
    bool found = chatusersupport::GetBaseInfo(USER_BASE_INFO + std::to_string(uid), uid, current_user);
    if (found) {
        notify["name"] = current_user->name;
        notify["nick"] = current_user->nick;
        notify["icon"] = current_user->icon;
        notify["sex"] = current_user->sex;
        notify["user_id"] = current_user->user_id;
    } else {
        notify["error"] = ErrorCodes::UidInvalid;
    }
    Json::Value task_payload;
    task_payload["recipient_uid"] = touid;
    task_payload["msgid"] = ID_NOTIFY_AUTH_FRIEND_REQ;
    task_payload["exclude_uid"] = 0;
    task_payload["payload"] = notify;
    std::string publish_error;
    _logic.PublishTask("relation_notify",
        memochat::chatruntime::TaskRoutingRelationNotify(),
        task_payload,
        0,
        memochat::chatruntime::TaskMaxRetries(),
        &publish_error);
    PublishRelationStateEventLocal(_logic, "friend_apply_approved", uid, touid, labels);
}

void ChatRelationService::HandleGetDialogList(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int uid = root.get("fromuid", root.get("uid", 0)).asInt();

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_GET_DIALOG_LIST_RSP);
    });

    if (uid <= 0) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    BuildDialogListJson(uid, rtvalue);
}

void ChatRelationService::HandleSyncDraft(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int uid = root.get("fromuid", root.get("uid", 0)).asInt();
    const std::string dialog_type = root.get("dialog_type", "").asString();
    const int peer_uid = root.get("peer_uid", 0).asInt();
    const int64_t group_id = root.get("groupid", root.get("group_id", 0)).asInt64();
    const bool has_mute_state = root.isMember("mute_state");
    const int mute_state = root.get("mute_state", 0).asInt();
    std::string draft_text = root.get("draft_text", "").asString();
    if (draft_text.size() > 2000) {
        draft_text.resize(2000);
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;
    rtvalue["dialog_type"] = dialog_type;
    rtvalue["peer_uid"] = peer_uid;
    rtvalue["group_id"] = static_cast<Json::Int64>(group_id);
    rtvalue["draft_text"] = draft_text;
    if (has_mute_state) {
        rtvalue["mute_state"] = mute_state > 0 ? 1 : 0;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_SYNC_DRAFT_RSP);
    });

    if (uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    int normalized_peer_uid = 0;
    int64_t normalized_group_id = 0;
    if (dialog_type == "private") {
        normalized_peer_uid = peer_uid;
        if (normalized_peer_uid <= 0 || !PostgresMgr::GetInstance()->IsFriend(uid, normalized_peer_uid)) {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return;
        }
    } else {
        normalized_group_id = group_id;
        if (normalized_group_id <= 0 || !PostgresMgr::GetInstance()->IsUserInGroup(normalized_group_id, uid)) {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return;
        }
    }

    if (!PostgresMgr::GetInstance()->UpsertDialogDraft(uid, dialog_type, normalized_peer_uid, normalized_group_id, draft_text)) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }
    if (has_mute_state &&
        !PostgresMgr::GetInstance()->UpsertDialogMuteState(uid, dialog_type, normalized_peer_uid, normalized_group_id, mute_state)) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
    }
}

void ChatRelationService::HandlePinDialog(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    const int uid = root.get("fromuid", root.get("uid", 0)).asInt();
    const std::string dialog_type = root.get("dialog_type", "").asString();
    const int peer_uid = root.get("peer_uid", 0).asInt();
    const int64_t group_id = root.get("groupid", root.get("group_id", 0)).asInt64();
    int pinned_rank = root.get("pinned_rank", 0).asInt();
    if (pinned_rank < 0) {
        pinned_rank = 0;
    }
    if (pinned_rank > 1000000000) {
        pinned_rank = 1000000000;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;
    rtvalue["dialog_type"] = dialog_type;
    rtvalue["peer_uid"] = peer_uid;
    rtvalue["group_id"] = static_cast<Json::Int64>(group_id);
    rtvalue["pinned_rank"] = pinned_rank;
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_PIN_DIALOG_RSP);
    });

    if (uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    int normalized_peer_uid = 0;
    int64_t normalized_group_id = 0;
    if (dialog_type == "private") {
        normalized_peer_uid = peer_uid;
        if (normalized_peer_uid <= 0 || !PostgresMgr::GetInstance()->IsFriend(uid, normalized_peer_uid)) {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return;
        }
    } else {
        normalized_group_id = group_id;
        if (normalized_group_id <= 0 || !PostgresMgr::GetInstance()->IsUserInGroup(normalized_group_id, uid)) {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return;
        }
    }

    if (!PostgresMgr::GetInstance()->UpsertDialogPinned(uid, dialog_type, normalized_peer_uid, normalized_group_id, pinned_rank)) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
    }
}
