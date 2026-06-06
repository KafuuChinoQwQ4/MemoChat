#include "ChatRelationService.h"

#include "ChatGrpcClient.h"
#include "ChatRuntime.h"
#include "CSession.h"
#include "ChatUserSupport.h"
#include "logging/Logger.h"

#include <chrono>
#include "json/GlazeCompat.h"
#include <unordered_map>

namespace
{
int64_t NowMsRelationLocal()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

// Compact wire JSON for TCP/QUIC transport (Qt QJsonDocument is strict).
std::string JsonToWireString(const Json::Value& v)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, v);
}

RelationCommandResult BuildRelationCommandResult(short response_msg_id, const Json::Value& payload)
{
    RelationCommandResult result;
    result.response_msg_id = response_msg_id;
    result.payload_json = JsonToWireString(payload);
    return result;
}

RelationCommandRequest
BuildRelationCommandRequest(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data)
{
    RelationCommandRequest request;
    request.request_msg_id = msg_id;
    request.payload_json = msg_data;
    if (session)
    {
        request.session_uid = session->GetUserId();
        request.session_id = session->GetSessionId();
    }
    return request;
}

void SendRelationCommandResult(const std::shared_ptr<CSession>& session, const RelationCommandResult& result)
{
    if (!session)
    {
        return;
    }
    session->Send(result.payload_json, result.response_msg_id);
}

void PublishRelationStateEventLocal(IEventPublisher* event_publisher,
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
    for (const auto& label : labels)
    {
        event_payload["labels"].append(label);
    }
    std::string publish_error;
    if (!event_publisher ||
        !event_publisher->PublishEvent(memochat::chatruntime::TopicRelationState(), event_payload, &publish_error))
    {
        memolog::LogWarn("chat.relation_state.publish_failed",
                         "failed to publish relation state event",
                         {{"event_type", event_type},
                          {"uid", std::to_string(uid)},
                          {"touid", std::to_string(touid)},
                          {"error", publish_error}});
    }
}
} // namespace

ChatRelationService::ChatRelationService(IRelationRepository* relation_repository,
                                         IRelationBootstrapCache* relation_bootstrap_cache,
                                         IDeliveryGateway* delivery_gateway,
                                         IDeliveryTaskPublisher* task_publisher,
                                         IEventPublisher* event_publisher)
    : _relation_repository(relation_repository)
    , _relation_bootstrap_cache(relation_bootstrap_cache)
    , _delivery_gateway(delivery_gateway)
    , _task_publisher(task_publisher)
    , _event_publisher(event_publisher)
{
}

void ChatRelationService::AppendRelationBootstrapJson(int uid, Json::Value& out)
{
    const auto bootstrap_start_ms = NowMsRelationLocal();
    if (_relation_bootstrap_cache && _relation_bootstrap_cache->TryAppend(uid, out))
    {
        memolog::LogInfo("chat.relation_bootstrap.cache_hit",
                         "relation bootstrap served from cache",
                         {{"uid", std::to_string(uid)},
                          {"relation_bootstrap_ms", std::to_string(NowMsRelationLocal() - bootstrap_start_ms)}});
        return;
    }

    Json::Value cache_payload(Json::objectValue);
    out["apply_list"] = memochat::json::array_t{};
    out["friend_list"] = memochat::json::array_t{};
    cache_payload["apply_list"] = memochat::json::array_t{};
    cache_payload["friend_list"] = memochat::json::array_t{};
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    if (chatusersupport::GetFriendApplyInfo(uid, apply_list))
    {
        for (auto& apply : apply_list)
        {
            Json::Value obj;
            obj["name"] = apply->_name;
            obj["uid"] = apply->_uid;
            obj["user_id"] = apply->_user_id;
            obj["icon"] = apply->_icon;
            obj["nick"] = apply->_nick;
            obj["sex"] = apply->_sex;
            obj["desc"] = apply->_desc;
            obj["status"] = apply->_status;
            for (const auto& tag : apply->_labels)
            {
                obj["labels"].append(tag);
            }
            out["apply_list"].append(obj);
            cache_payload["apply_list"].append(obj);
        }
    }

    std::vector<std::shared_ptr<UserInfo>> friend_list;
    if (chatusersupport::GetFriendList(uid, friend_list))
    {
        for (auto& friend_ele : friend_list)
        {
            Json::Value obj;
            obj["name"] = friend_ele->name;
            obj["uid"] = friend_ele->uid;
            obj["user_id"] = friend_ele->user_id;
            obj["icon"] = friend_ele->icon;
            obj["nick"] = friend_ele->nick;
            obj["sex"] = friend_ele->sex;
            obj["desc"] = friend_ele->desc;
            obj["back"] = friend_ele->back;
            for (const auto& tag : friend_ele->labels)
            {
                obj["labels"].append(tag);
            }
            out["friend_list"].append(obj);
            cache_payload["friend_list"].append(obj);
        }
    }

    if (_relation_bootstrap_cache)
    {
        _relation_bootstrap_cache->Store(uid, cache_payload);
    }
    memolog::LogInfo("chat.relation_bootstrap.cache_fill",
                     "relation bootstrap cache populated",
                     {{"uid", std::to_string(uid)},
                      {"relation_bootstrap_ms", std::to_string(NowMsRelationLocal() - bootstrap_start_ms)}});
}

void ChatRelationService::BuildDialogListJson(int uid, Json::Value& out)
{
    _relation_repository->RefreshDialogsForOwner(uid);

    std::unordered_map<int, std::shared_ptr<DialogMetaInfo>> private_meta;
    std::unordered_map<int64_t, std::shared_ptr<DialogMetaInfo>> group_meta;
    std::vector<std::shared_ptr<DialogMetaInfo>> meta_list;
    if (_relation_repository->GetDialogMetaByOwner(uid, meta_list))
    {
        for (const auto& meta : meta_list)
        {
            if (!meta)
            {
                continue;
            }
            if (meta->dialog_type == "private" && meta->peer_uid > 0)
            {
                private_meta[meta->peer_uid] = meta;
                continue;
            }
            if (meta->dialog_type == "group" && meta->group_id > 0)
            {
                group_meta[meta->group_id] = meta;
            }
        }
    }

    std::vector<std::shared_ptr<UserInfo>> friend_list;
    chatusersupport::GetFriendList(uid, friend_list);
    for (const auto& peer : friend_list)
    {
        if (!peer)
        {
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
        if (!_relation_repository->GetPrivateDialogRuntime(uid, peer->uid, runtime))
        {
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
    _relation_repository->GetUserGroupList(uid, groups);
    for (const auto& group : groups)
    {
        if (!group)
        {
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
        if (!_relation_repository->GetGroupDialogRuntime(uid, group->group_id, runtime))
        {
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

RelationCommandResult ChatRelationService::SearchUser(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const std::string user_id = root["user_id"].asString();
    Json::Value rtvalue;
    if (!root.isMember("user_id") || user_id.empty())
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return BuildRelationCommandResult(ID_SEARCH_USER_RSP, rtvalue);
    }
    int uid = 0;
    if (!_relation_repository->GetUidByUserId(user_id, uid) || uid <= 0)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return BuildRelationCommandResult(ID_SEARCH_USER_RSP, rtvalue);
    }
    chatusersupport::GetUserByUid(std::to_string(uid), rtvalue);
    return BuildRelationCommandResult(ID_SEARCH_USER_RSP, rtvalue);
}

RelationCommandResult ChatRelationService::AddFriendApply(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    auto uid = root["uid"].asInt();
    auto applyname = root["applyname"].asString();
    auto touid = root["touid"].asInt();
    std::vector<std::string> labels;
    if (root.isMember("labels") && root["labels"].isArray())
    {
        for (const auto& one : root["labels"])
        {
            labels.push_back(one.asString());
        }
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;

    _relation_repository->AddFriendApply(uid, touid);
    _relation_repository->ReplaceApplyTags(uid, touid, labels);
    if (_relation_bootstrap_cache)
    {
        _relation_bootstrap_cache->Invalidate(touid);
    }
    auto apply_info = std::make_shared<UserInfo>();
    bool b_info = chatusersupport::GetBaseInfo(USER_BASE_INFO + std::to_string(uid), uid, apply_info);
    Json::Value notify;
    notify["error"] = ErrorCodes::Success;
    notify["applyuid"] = uid;
    notify["name"] = applyname;
    notify["desc"] = "";
    if (b_info)
    {
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
    const bool delivered =
        _delivery_gateway && _delivery_gateway->TryPushPayload({touid}, ID_NOTIFY_ADD_FRIEND_REQ, notify, 0, false);
    const bool published =
        delivered ||
        (_task_publisher && _task_publisher->PublishDeliveryTask("relation_notify",
                                                                 memochat::chatruntime::TaskRoutingRelationNotify(),
                                                                 task_payload,
                                                                 0,
                                                                 memochat::chatruntime::TaskMaxRetries(),
                                                                 &publish_error));
    if (!published)
    {
        memolog::LogWarn("chat.relation_notify.publish_failed",
                         "failed to publish add friend notification",
                         {{"uid", std::to_string(uid)}, {"touid", std::to_string(touid)}, {"error", publish_error}});
    }
    PublishRelationStateEventLocal(_event_publisher, "friend_apply_created", uid, touid, labels);
    return BuildRelationCommandResult(ID_ADD_FRIEND_RSP, rtvalue);
}

RelationCommandResult ChatRelationService::AuthFriendApply(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);

    auto uid = root["fromuid"].asInt();
    auto touid = root["touid"].asInt();
    auto back_name = root["back"].asString();
    std::vector<std::string> labels;
    if (root.isMember("labels") && root["labels"].isArray())
    {
        for (const auto& one : root["labels"])
        {
            labels.push_back(one.asString());
        }
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    auto user_info = std::make_shared<UserInfo>();
    bool b_info = chatusersupport::GetBaseInfo(USER_BASE_INFO + std::to_string(touid), touid, user_info);
    if (b_info)
    {
        rtvalue["name"] = user_info->name;
        rtvalue["nick"] = user_info->nick;
        rtvalue["icon"] = user_info->icon;
        rtvalue["sex"] = user_info->sex;
        rtvalue["uid"] = touid;
        rtvalue["user_id"] = user_info->user_id;
    }
    else
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
    }

    _relation_repository->AuthFriendApply(uid, touid);
    _relation_repository->AddFriend(uid, touid, back_name);
    _relation_repository->ReplaceFriendTags(uid, touid, labels);
    auto requesterApplyTags = _relation_repository->GetApplyTags(touid, uid);
    _relation_repository->ReplaceFriendTags(touid, uid, requesterApplyTags);
    if (_relation_bootstrap_cache)
    {
        _relation_bootstrap_cache->Invalidate(uid);
        _relation_bootstrap_cache->Invalidate(touid);
    }
    Json::Value notify;
    notify["error"] = ErrorCodes::Success;
    notify["fromuid"] = uid;
    notify["touid"] = touid;
    auto current_user = std::make_shared<UserInfo>();
    bool found = chatusersupport::GetBaseInfo(USER_BASE_INFO + std::to_string(uid), uid, current_user);
    if (found)
    {
        notify["name"] = current_user->name;
        notify["nick"] = current_user->nick;
        notify["icon"] = current_user->icon;
        notify["sex"] = current_user->sex;
        notify["user_id"] = current_user->user_id;
    }
    else
    {
        notify["error"] = ErrorCodes::UidInvalid;
    }
    Json::Value task_payload;
    task_payload["recipient_uid"] = touid;
    task_payload["msgid"] = ID_NOTIFY_AUTH_FRIEND_REQ;
    task_payload["exclude_uid"] = 0;
    task_payload["payload"] = notify;
    std::string publish_error;
    const bool delivered =
        _delivery_gateway && _delivery_gateway->TryPushPayload({touid}, ID_NOTIFY_AUTH_FRIEND_REQ, notify, 0, false);
    const bool published =
        delivered ||
        (_task_publisher && _task_publisher->PublishDeliveryTask("relation_notify",
                                                                 memochat::chatruntime::TaskRoutingRelationNotify(),
                                                                 task_payload,
                                                                 0,
                                                                 memochat::chatruntime::TaskMaxRetries(),
                                                                 &publish_error));
    if (!published)
    {
        memolog::LogWarn("chat.relation_notify.publish_failed",
                         "failed to publish auth friend notification",
                         {{"uid", std::to_string(uid)}, {"touid", std::to_string(touid)}, {"error", publish_error}});
    }
    PublishRelationStateEventLocal(_event_publisher, "friend_apply_approved", uid, touid, labels);
    return BuildRelationCommandResult(ID_AUTH_FRIEND_RSP, rtvalue);
}

RelationCommandResult ChatRelationService::DeleteFriend(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const int uid = root.isMember("fromuid") ? root["fromuid"].asInt() : root["uid"].asInt();
    const int friendUid = root.isMember("friend_uid") ? root["friend_uid"].asInt() : root["touid"].asInt();

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = uid;
    rtvalue["friend_uid"] = friendUid;

    if (uid <= 0 || friendUid <= 0 || uid == friendUid)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return BuildRelationCommandResult(ID_DELETE_FRIEND_RSP, rtvalue);
    }

    if (!_relation_repository->DeleteFriend(uid, friendUid))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return BuildRelationCommandResult(ID_DELETE_FRIEND_RSP, rtvalue);
    }

    PublishRelationStateEventLocal(_event_publisher, "friend_deleted", uid, friendUid, {});
    return BuildRelationCommandResult(ID_DELETE_FRIEND_RSP, rtvalue);
}

RelationCommandResult ChatRelationService::GetDialogList(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const int uid = root.isMember("fromuid") ? root["fromuid"].asInt() : root["uid"].asInt();

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;

    if (uid <= 0)
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return BuildRelationCommandResult(ID_GET_DIALOG_LIST_RSP, rtvalue);
    }

    BuildDialogListJson(uid, rtvalue);
    return BuildRelationCommandResult(ID_GET_DIALOG_LIST_RSP, rtvalue);
}

RelationCommandResult ChatRelationService::SyncDraft(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const int uid = root.isMember("fromuid") ? root["fromuid"].asInt() : root["uid"].asInt();
    const std::string dialog_type = root.get("dialog_type", "").asString();
    const int peer_uid = root.get("peer_uid", 0).asInt();
    const bool has_mute_state = root.isMember("mute_state");
    const int mute_state = root.get("mute_state", 0).asInt();
    std::string draft_text = root.get("draft_text", "").asString();
    if (draft_text.size() > 2000)
    {
        draft_text.resize(2000);
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;
    rtvalue["dialog_type"] = dialog_type;
    rtvalue["peer_uid"] = peer_uid;
    rtvalue["draft_text"] = draft_text;
    if (has_mute_state)
    {
        rtvalue["mute_state"] = mute_state > 0 ? 1 : 0;
    }

    if (uid <= 0 || (dialog_type != "private" && dialog_type != "group"))
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, rtvalue);
    }

    int normalized_peer_uid = 0;
    int64_t normalized_group_id = 0;
    if (dialog_type == "private")
    {
        normalized_peer_uid = peer_uid;
        if (normalized_peer_uid <= 0 || !_relation_repository->IsPrivateFriend(uid, normalized_peer_uid))
        {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, rtvalue);
        }
    }
    else
    {
        normalized_group_id = root.isMember("groupid") ? root["groupid"].asInt64() : root.get("group_id", 0).asInt64();
        rtvalue["group_id"] = static_cast<Json::Int64>(normalized_group_id);
        if (normalized_group_id <= 0 || !_relation_repository->IsGroupMember(normalized_group_id, uid))
        {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, rtvalue);
        }
    }

    if (!_relation_repository
             ->UpsertDialogDraft(uid, dialog_type, normalized_peer_uid, normalized_group_id, draft_text))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, rtvalue);
    }
    if (has_mute_state && !_relation_repository->UpsertDialogMuteState(uid,
                                                                       dialog_type,
                                                                       normalized_peer_uid,
                                                                       normalized_group_id,
                                                                       mute_state))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
    }
    return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, rtvalue);
}

RelationCommandResult ChatRelationService::PinDialog(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const int uid = root.isMember("fromuid") ? root["fromuid"].asInt() : root["uid"].asInt();
    const std::string dialog_type = root.get("dialog_type", "").asString();
    const int peer_uid = root.get("peer_uid", 0).asInt();
    int pinned_rank = root.get("pinned_rank", 0).asInt();
    if (pinned_rank < 0)
    {
        pinned_rank = 0;
    }
    if (pinned_rank > 1000000000)
    {
        pinned_rank = 1000000000;
    }

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;
    rtvalue["dialog_type"] = dialog_type;
    rtvalue["peer_uid"] = peer_uid;
    rtvalue["pinned_rank"] = pinned_rank;

    if (uid <= 0 || (dialog_type != "private" && dialog_type != "group"))
    {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return BuildRelationCommandResult(ID_PIN_DIALOG_RSP, rtvalue);
    }

    int normalized_peer_uid = 0;
    int64_t normalized_group_id = 0;
    if (dialog_type == "private")
    {
        normalized_peer_uid = peer_uid;
        if (normalized_peer_uid <= 0 || !_relation_repository->IsPrivateFriend(uid, normalized_peer_uid))
        {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return BuildRelationCommandResult(ID_PIN_DIALOG_RSP, rtvalue);
        }
    }
    else
    {
        normalized_group_id = root.isMember("groupid") ? root["groupid"].asInt64() : root.get("group_id", 0).asInt64();
        rtvalue["group_id"] = static_cast<Json::Int64>(normalized_group_id);
        if (normalized_group_id <= 0 || !_relation_repository->IsGroupMember(normalized_group_id, uid))
        {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return BuildRelationCommandResult(ID_PIN_DIALOG_RSP, rtvalue);
        }
    }

    if (!_relation_repository
             ->UpsertDialogPinned(uid, dialog_type, normalized_peer_uid, normalized_group_id, pinned_rank))
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
    }
    return BuildRelationCommandResult(ID_PIN_DIALOG_RSP, rtvalue);
}

void ChatRelationService::HandleSearchUser(const std::shared_ptr<CSession>& session,
                                           short msg_id,
                                           const std::string& msg_data)
{
    SendRelationCommandResult(session, SearchUser(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationService::HandleAddFriendApply(const std::shared_ptr<CSession>& session,
                                               short msg_id,
                                               const std::string& msg_data)
{
    SendRelationCommandResult(session, AddFriendApply(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationService::HandleAuthFriendApply(const std::shared_ptr<CSession>& session,
                                                short msg_id,
                                                const std::string& msg_data)
{
    SendRelationCommandResult(session, AuthFriendApply(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationService::HandleDeleteFriend(const std::shared_ptr<CSession>& session,
                                             short msg_id,
                                             const std::string& msg_data)
{
    SendRelationCommandResult(session, DeleteFriend(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationService::HandleGetDialogList(const std::shared_ptr<CSession>& session,
                                              short msg_id,
                                              const std::string& msg_data)
{
    SendRelationCommandResult(session, GetDialogList(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationService::HandleSyncDraft(const std::shared_ptr<CSession>& session,
                                          short msg_id,
                                          const std::string& msg_data)
{
    SendRelationCommandResult(session, SyncDraft(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationService::HandlePinDialog(const std::shared_ptr<CSession>& session,
                                          short msg_id,
                                          const std::string& msg_data)
{
    SendRelationCommandResult(session, PinDialog(BuildRelationCommandRequest(session, msg_id, msg_data)));
}
