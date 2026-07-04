#include "ChatRelationService.hpp"

#include "ChatRuntime.hpp"
#include "ChatRelationCommandDtos.hpp"
#include "ChatRelationGroupDtos.hpp"
#include "ChatUserSupport.hpp"
#include "const.hpp"
#include "delivery/MessageDeliveryTaskPayload.hpp"
#include "logging/Logger.hpp"

#include <chrono>
#include "json/GlazeCompat.hpp"
#include <unordered_map>
#include <unordered_set>

import memochat.chat.relation_service_algorithms;

namespace
{
namespace ChatOutput = memochat::chat::output;
namespace ChatRelationDtos = memochat::chat::relation;
namespace relation_service_modules = memochat::chat::relation_service::modules;

int64_t NowMsRelationLocal()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

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

void PublishRelationStateEventLocal(IEventPublisher* event_publisher,
                                    const std::string& event_type,
                                    int uid,
                                    int touid,
                                    const std::vector<std::string>& labels)
{
    const ChatRelationDtos::ChatRelationStateEventDto event_dto{.event_type = event_type,
                                                                .uid = uid,
                                                                .touid = touid,
                                                                .uids = {uid, touid},
                                                                .event_ts = NowMsRelationLocal(),
                                                                .labels = labels};
    Json::Value event_payload = ChatRelationDtos::ToJsonValue(event_dto);
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

    ChatOutput::ChatRelationBootstrapDto bootstrap;
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    if (chatusersupport::GetFriendApplyInfo(uid, apply_list))
    {
        for (auto& apply : apply_list)
        {
            ChatOutput::ChatRelationApplyRowDto row;
            row.name = apply->_name;
            row.uid = apply->_uid;
            row.user_id = apply->_user_id;
            row.icon = apply->_icon;
            row.nick = apply->_nick;
            row.sex = apply->_sex;
            row.desc = apply->_desc;
            row.status = apply->_status;
            row.labels = apply->_labels;
            bootstrap.apply_list.push_back(row);
        }
    }

    std::vector<std::shared_ptr<UserInfo>> friend_list;
    if (chatusersupport::GetFriendList(uid, friend_list))
    {
        for (auto& friend_ele : friend_list)
        {
            ChatOutput::ChatRelationFriendRowDto row;
            row.name = friend_ele->name;
            row.uid = friend_ele->uid;
            row.user_id = friend_ele->user_id;
            row.icon = friend_ele->icon;
            row.nick = friend_ele->nick;
            row.sex = friend_ele->sex;
            row.desc = friend_ele->desc;
            row.back = friend_ele->back;
            row.labels = friend_ele->labels;
            bootstrap.friend_list.push_back(row);
        }
    }

    Json::Value bootstrap_payload = ChatOutput::ToJsonValue(bootstrap);
    out["apply_list"] = bootstrap_payload["apply_list"].get<Json::Value>();
    out["friend_list"] = bootstrap_payload["friend_list"].get<Json::Value>();
    if (_relation_bootstrap_cache)
    {
        _relation_bootstrap_cache->Store(uid, bootstrap_payload);
    }
    memolog::LogInfo("chat.relation_bootstrap.cache_fill",
                     "relation bootstrap cache populated",
                     {{"uid", std::to_string(uid)},
                      {"relation_bootstrap_ms", std::to_string(NowMsRelationLocal() - bootstrap_start_ms)}});
}

void ChatRelationService::BuildDialogListJson(int uid, Json::Value& out)
{
    out["dialogs"] = Json::Value(Json::arrayValue);
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
            if (meta->dialog_type == relation_service_modules::PrivateDialogType() && meta->peer_uid > 0)
            {
                private_meta[meta->peer_uid] = meta;
                continue;
            }
            if (meta->dialog_type == relation_service_modules::GroupDialogType() && meta->group_id > 0)
            {
                group_meta[meta->group_id] = meta;
            }
        }
    }

    std::unordered_set<int> appended_private_uids;
    std::unordered_set<int64_t> appended_group_ids;

    std::vector<std::shared_ptr<UserInfo>> friend_list;
    chatusersupport::GetFriendList(uid, friend_list);
    for (const auto& peer : friend_list)
    {
        if (!peer)
        {
            continue;
        }
        const auto meta_it = private_meta.find(peer->uid);
        ChatOutput::ChatDialogRowDto row;
        row.dialog_id = std::string("u_") + std::to_string(peer->uid);
        row.dialog_type = relation_service_modules::PrivateDialogType();
        row.peer_uid = peer->uid;
        row.title = peer->nick.empty() ? peer->name : peer->nick;
        row.avatar = peer->icon;
        DialogRuntimeInfo runtime;
        if (!_relation_repository->GetPrivateDialogRuntime(uid, peer->uid, runtime))
        {
            runtime = DialogRuntimeInfo();
        }
        row.last_msg_preview = runtime.last_msg_preview;
        row.last_msg_ts = runtime.last_msg_ts;
        row.unread_count = runtime.unread_count;
        row.pinned_rank = (meta_it == private_meta.end()) ? 0 : meta_it->second->pinned_rank;
        row.draft_text = (meta_it == private_meta.end()) ? "" : meta_it->second->draft_text;
        row.mute_state = (meta_it == private_meta.end()) ? 0 : meta_it->second->mute_state;
        out["dialogs"].append(ChatOutput::ToJsonValue(row));
        appended_private_uids.insert(peer->uid);
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
        ChatOutput::ChatDialogRowDto row;
        row.dialog_id = std::string("g_") + std::to_string(group->group_id);
        row.dialog_type = relation_service_modules::GroupDialogType();
        row.group_id = group->group_id;
        row.title = group->name;
        row.avatar = group->icon;
        DialogRuntimeInfo runtime;
        if (!_relation_repository->GetGroupDialogRuntime(uid, group->group_id, runtime))
        {
            runtime = DialogRuntimeInfo();
        }
        row.last_msg_preview = runtime.last_msg_preview;
        row.last_msg_ts = runtime.last_msg_ts;
        row.unread_count = runtime.unread_count;
        row.pinned_rank = (meta_it == group_meta.end()) ? 0 : meta_it->second->pinned_rank;
        row.draft_text = (meta_it == group_meta.end()) ? "" : meta_it->second->draft_text;
        row.mute_state = (meta_it == group_meta.end()) ? 0 : meta_it->second->mute_state;
        out["dialogs"].append(ChatOutput::ToJsonValue(row));
        appended_group_ids.insert(group->group_id);
    }

    for (const auto& meta : meta_list)
    {
        if (!meta)
        {
            continue;
        }
        if (meta->dialog_type == relation_service_modules::PrivateDialogType() && meta->peer_uid > 0 &&
            appended_private_uids.find(meta->peer_uid) == appended_private_uids.end())
        {
            auto peer = _relation_repository->GetUserByUid(meta->peer_uid);
            ChatOutput::ChatDialogRowDto row;
            row.dialog_id = std::string("u_") + std::to_string(meta->peer_uid);
            row.dialog_type = relation_service_modules::PrivateDialogType();
            row.peer_uid = meta->peer_uid;
            if (peer)
            {
                row.title = peer->nick.empty() ? peer->name : peer->nick;
                row.avatar = peer->icon;
            }
            else
            {
                row.title = std::string("用户") + std::to_string(meta->peer_uid);
                row.avatar = "";
            }
            DialogRuntimeInfo runtime;
            if (!_relation_repository->GetPrivateDialogRuntime(uid, meta->peer_uid, runtime))
            {
                runtime = DialogRuntimeInfo();
            }
            row.last_msg_preview = runtime.last_msg_preview;
            row.last_msg_ts = runtime.last_msg_ts;
            row.unread_count = runtime.unread_count;
            row.pinned_rank = meta->pinned_rank;
            row.draft_text = meta->draft_text;
            row.mute_state = meta->mute_state;
            out["dialogs"].append(ChatOutput::ToJsonValue(row));
            appended_private_uids.insert(meta->peer_uid);
            continue;
        }
        if (meta->dialog_type == relation_service_modules::GroupDialogType() && meta->group_id > 0 &&
            appended_group_ids.find(meta->group_id) == appended_group_ids.end())
        {
            std::shared_ptr<GroupInfo> group;
            _relation_repository->GetGroupById(meta->group_id, group);
            ChatOutput::ChatDialogRowDto row;
            row.dialog_id = std::string("g_") + std::to_string(meta->group_id);
            row.dialog_type = relation_service_modules::GroupDialogType();
            row.group_id = meta->group_id;
            row.title = group ? group->name : (std::string("群聊") + std::to_string(meta->group_id));
            row.avatar = group ? group->icon : "";
            DialogRuntimeInfo runtime;
            if (!_relation_repository->GetGroupDialogRuntime(uid, meta->group_id, runtime))
            {
                runtime = DialogRuntimeInfo();
            }
            row.last_msg_preview = runtime.last_msg_preview;
            row.last_msg_ts = runtime.last_msg_ts;
            row.unread_count = runtime.unread_count;
            row.pinned_rank = meta->pinned_rank;
            row.draft_text = meta->draft_text;
            row.mute_state = meta->mute_state;
            out["dialogs"].append(ChatOutput::ToJsonValue(row));
            appended_group_ids.insert(meta->group_id);
        }
    }
}

RelationCommandResult ChatRelationService::SearchUser(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const ChatRelationDtos::ChatSearchUserRequestDto request_dto =
        ChatRelationDtos::ChatSearchUserRequestFromJsonValue(root);
    const std::string user_id = request_dto.user_id;
    if (relation_service_modules::ShouldRejectSearchUserRequest(root.isMember("user_id"), user_id.empty()))
    {
        const ChatRelationDtos::ChatSearchUserResponseDto response{.error = ErrorCodes::Error_Json};
        return BuildRelationCommandResult(ID_SEARCH_USER_RSP, ChatRelationDtos::ToJsonValue(response));
    }
    int uid = 0;
    const bool found_user = _relation_repository->GetUidByUserId(user_id, uid);
    if (relation_service_modules::ShouldRejectSearchUserResult(found_user, uid))
    {
        const ChatRelationDtos::ChatSearchUserResponseDto response{.error = ErrorCodes::UidInvalid};
        return BuildRelationCommandResult(ID_SEARCH_USER_RSP, ChatRelationDtos::ToJsonValue(response));
    }
    Json::Value user_root;
    chatusersupport::GetUserByUid(std::to_string(uid), user_root);
    const ChatRelationDtos::ChatSearchUserResponseDto response =
        ChatRelationDtos::ChatSearchUserResponseFromJsonValue(user_root);
    return BuildRelationCommandResult(ID_SEARCH_USER_RSP, ChatRelationDtos::ToJsonValue(response));
}

RelationCommandResult ChatRelationService::FilterFriendUids(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const ChatRelationDtos::ChatFilterFriendUidsRequestDto request_dto =
        ChatRelationDtos::ChatFilterFriendUidsRequestFromJsonValue(root);

    ChatRelationDtos::ChatFilterFriendUidsResponseDto response{.error = ErrorCodes::Success};
    if (relation_service_modules::ShouldFilterFriendUids(request_dto.viewer_uid, !request_dto.author_uids.empty()))
    {
        for (int friend_uid : _relation_repository->FilterFriendUids(request_dto.viewer_uid, request_dto.author_uids))
        {
            response.friend_uids.push_back(friend_uid);
        }
    }
    // Internal read RPC: no client-facing TCP message id, so 0.
    return BuildRelationCommandResult(relation_service_modules::InternalReadResponseMessageId(),
                                      ChatRelationDtos::ToJsonValue(response));
}

RelationCommandResult ChatRelationService::AddFriendApply(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const ChatRelationDtos::ChatAddFriendApplyRequestDto request_dto =
        ChatRelationDtos::ChatAddFriendApplyRequestFromJsonValue(root);
    const int uid = request_dto.uid;
    const std::string& applyname = request_dto.applyname;
    const int touid = request_dto.touid;
    const std::vector<std::string>& labels = request_dto.labels;

    const ChatRelationDtos::ChatAddFriendApplyResponseDto response{.error = ErrorCodes::Success};

    _relation_repository->AddFriendApply(uid, touid);
    _relation_repository->ReplaceApplyTags(uid, touid, labels);
    if (_relation_bootstrap_cache)
    {
        _relation_bootstrap_cache->Invalidate(touid);
    }
    auto apply_info = std::make_shared<UserInfo>();
    bool b_info = chatusersupport::GetBaseInfo(USER_BASE_INFO + std::to_string(uid), uid, apply_info);
    ChatRelationDtos::ChatAddFriendApplyNotifyDto notify_dto{.error = ErrorCodes::Success,
                                                             .applyuid = uid,
                                                             .name = applyname,
                                                             .desc = ""};
    if (b_info)
    {
        notify_dto.icon = apply_info->icon;
        notify_dto.sex = apply_info->sex;
        notify_dto.nick = apply_info->nick;
        notify_dto.user_id = apply_info->user_id;
    }
    Json::Value notify = ChatRelationDtos::ToJsonValue(notify_dto);
    Json::Value task_payload =
        memochat::chat::delivery::BuildDeliveryTaskPayloadJson(touid, ID_NOTIFY_ADD_FRIEND_REQ, notify, 0, "");
    std::string publish_error;
    const bool delivered =
        _delivery_gateway && _delivery_gateway->TryPushPayload({touid}, ID_NOTIFY_ADD_FRIEND_REQ, notify, 0, false);
    const bool published =
        delivered ||
        (_task_publisher && _task_publisher->PublishDeliveryTask(relation_service_modules::RelationNotifyTaskName(),
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
    PublishRelationStateEventLocal(_event_publisher,
                                   relation_service_modules::FriendApplyCreatedEvent(),
                                   uid,
                                   touid,
                                   labels);
    return BuildRelationCommandResult(ID_ADD_FRIEND_RSP, ChatRelationDtos::ToJsonValue(response));
}

RelationCommandResult ChatRelationService::AuthFriendApply(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const ChatRelationDtos::ChatAuthFriendApplyRequestDto request_dto =
        ChatRelationDtos::ChatAuthFriendApplyRequestFromJsonValue(root);
    const int uid = request_dto.fromuid;
    const int touid = request_dto.touid;
    const std::string& back_name = request_dto.back;
    const std::vector<std::string>& labels = request_dto.labels;

    ChatRelationDtos::ChatAuthFriendApplyResponseDto response{.error = ErrorCodes::Success};
    auto user_info = std::make_shared<UserInfo>();
    bool b_info = chatusersupport::GetBaseInfo(USER_BASE_INFO + std::to_string(touid), touid, user_info);
    if (b_info)
    {
        response.name = user_info->name;
        response.nick = user_info->nick;
        response.icon = user_info->icon;
        response.sex = user_info->sex;
        response.uid = touid;
        response.user_id = user_info->user_id;
    }
    else
    {
        response.error = ErrorCodes::UidInvalid;
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
    ChatRelationDtos::ChatAuthFriendApplyNotifyDto notify_dto{.error = ErrorCodes::Success,
                                                              .fromuid = uid,
                                                              .touid = touid};
    auto current_user = std::make_shared<UserInfo>();
    bool found = chatusersupport::GetBaseInfo(USER_BASE_INFO + std::to_string(uid), uid, current_user);
    if (found)
    {
        notify_dto.name = current_user->name;
        notify_dto.nick = current_user->nick;
        notify_dto.icon = current_user->icon;
        notify_dto.sex = current_user->sex;
        notify_dto.user_id = current_user->user_id;
    }
    else
    {
        notify_dto.error = ErrorCodes::UidInvalid;
    }
    Json::Value notify = ChatRelationDtos::ToJsonValue(notify_dto);
    Json::Value task_payload =
        memochat::chat::delivery::BuildDeliveryTaskPayloadJson(touid, ID_NOTIFY_AUTH_FRIEND_REQ, notify, 0, "");
    std::string publish_error;
    const bool delivered =
        _delivery_gateway && _delivery_gateway->TryPushPayload({touid}, ID_NOTIFY_AUTH_FRIEND_REQ, notify, 0, false);
    const bool published =
        delivered ||
        (_task_publisher && _task_publisher->PublishDeliveryTask(relation_service_modules::RelationNotifyTaskName(),
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
    PublishRelationStateEventLocal(_event_publisher,
                                   relation_service_modules::FriendApplyApprovedEvent(),
                                   uid,
                                   touid,
                                   labels);
    return BuildRelationCommandResult(ID_AUTH_FRIEND_RSP, ChatRelationDtos::ToJsonValue(response));
}

RelationCommandResult ChatRelationService::DeleteFriend(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const ChatRelationDtos::ChatDeleteFriendRequestDto request_dto =
        ChatRelationDtos::ChatDeleteFriendRequestFromJsonValue(root);
    const int uid = request_dto.uid;
    const int friendUid = request_dto.friend_uid;

    ChatRelationDtos::ChatDeleteFriendResponseDto response{.error = ErrorCodes::Success,
                                                           .fromuid = uid,
                                                           .friend_uid = friendUid};

    if (relation_service_modules::ShouldRejectDeleteFriend(uid, friendUid))
    {
        response.error = ErrorCodes::Error_Json;
        return BuildRelationCommandResult(ID_DELETE_FRIEND_RSP, ChatRelationDtos::ToJsonValue(response));
    }

    if (!_relation_repository->DeleteFriend(uid, friendUid))
    {
        response.error = ErrorCodes::RPCFailed;
        return BuildRelationCommandResult(ID_DELETE_FRIEND_RSP, ChatRelationDtos::ToJsonValue(response));
    }

    PublishRelationStateEventLocal(_event_publisher,
                                   relation_service_modules::FriendDeletedEvent(),
                                   uid,
                                   friendUid,
                                   {});
    return BuildRelationCommandResult(ID_DELETE_FRIEND_RSP, ChatRelationDtos::ToJsonValue(response));
}

RelationCommandResult ChatRelationService::GetDialogList(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const ChatRelationDtos::ChatDialogListRequestDto request_dto =
        ChatRelationDtos::ChatDialogListRequestFromJsonValue(root);
    const int uid = request_dto.uid;

    ChatRelationDtos::ChatDialogListResponseDto response{.error = ErrorCodes::Success, .uid = uid};

    if (relation_service_modules::ShouldRejectPositiveUid(uid))
    {
        response.error = ErrorCodes::Error_Json;
        return BuildRelationCommandResult(ID_GET_DIALOG_LIST_RSP, ChatRelationDtos::ToJsonValue(response));
    }

    Json::Value dialog_root;
    BuildDialogListJson(uid, dialog_root);
    response.dialogs = dialog_root["dialogs"];
    return BuildRelationCommandResult(ID_GET_DIALOG_LIST_RSP, ChatRelationDtos::ToJsonValue(response));
}

RelationCommandResult ChatRelationService::SyncDraft(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const ChatRelationDtos::ChatSyncDraftRequestDto request_dto =
        ChatRelationDtos::ChatSyncDraftRequestFromJsonValue(root);
    const int uid = request_dto.uid;
    const std::string& dialog_type = request_dto.dialog_type;
    const int peer_uid = request_dto.peer_uid;
    const bool has_mute_state = request_dto.has_mute_state;
    const int mute_state = request_dto.mute_state;
    const std::string& draft_text = request_dto.draft_text;

    ChatRelationDtos::ChatSyncDraftResponseDto response{.error = ErrorCodes::Success,
                                                        .uid = uid,
                                                        .dialog_type = dialog_type,
                                                        .peer_uid = peer_uid,
                                                        .draft_text = draft_text};
    if (has_mute_state)
    {
        response.mute_state = relation_service_modules::NormalizeMuteState(mute_state);
    }

    const bool sync_is_private = dialog_type == relation_service_modules::PrivateDialogType();
    const bool sync_is_group = dialog_type == relation_service_modules::GroupDialogType();
    if (relation_service_modules::ShouldRejectPositiveUid(uid) ||
        relation_service_modules::ShouldRejectDialogType(sync_is_private, sync_is_group))
    {
        response.error = ErrorCodes::Error_Json;
        return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, ChatRelationDtos::ToJsonValue(response));
    }

    int normalized_peer_uid = 0;
    int64_t normalized_group_id = 0;
    if (sync_is_private)
    {
        normalized_peer_uid = peer_uid;
        const bool is_private_friend =
            normalized_peer_uid > 0 && _relation_repository->IsPrivateFriend(uid, normalized_peer_uid);
        if (relation_service_modules::ShouldRejectPrivateDialogTarget(normalized_peer_uid, is_private_friend))
        {
            response.error = ErrorCodes::GroupPermissionDenied;
            return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, ChatRelationDtos::ToJsonValue(response));
        }
    }
    else
    {
        normalized_group_id = request_dto.group_id;
        response.group_id = normalized_group_id;
        const bool is_group_member =
            normalized_group_id > 0 && _relation_repository->IsGroupMember(normalized_group_id, uid);
        if (relation_service_modules::ShouldRejectGroupDialogTarget(normalized_group_id, is_group_member))
        {
            response.error = ErrorCodes::GroupPermissionDenied;
            return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, ChatRelationDtos::ToJsonValue(response));
        }
    }

    if (!_relation_repository
             ->UpsertDialogDraft(uid, dialog_type, normalized_peer_uid, normalized_group_id, draft_text))
    {
        response.error = ErrorCodes::RPCFailed;
        return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, ChatRelationDtos::ToJsonValue(response));
    }
    if (has_mute_state && !_relation_repository->UpsertDialogMuteState(uid,
                                                                       dialog_type,
                                                                       normalized_peer_uid,
                                                                       normalized_group_id,
                                                                       mute_state))
    {
        response.error = ErrorCodes::RPCFailed;
    }
    return BuildRelationCommandResult(ID_SYNC_DRAFT_RSP, ChatRelationDtos::ToJsonValue(response));
}

RelationCommandResult ChatRelationService::PinDialog(const RelationCommandRequest& request)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(request.payload_json, root);
    const ChatRelationDtos::ChatPinDialogRequestDto request_dto =
        ChatRelationDtos::ChatPinDialogRequestFromJsonValue(root);
    const int uid = request_dto.uid;
    const std::string& dialog_type = request_dto.dialog_type;
    const int peer_uid = request_dto.peer_uid;
    const int pinned_rank = request_dto.pinned_rank;

    ChatRelationDtos::ChatPinDialogResponseDto response{.error = ErrorCodes::Success,
                                                        .uid = uid,
                                                        .dialog_type = dialog_type,
                                                        .peer_uid = peer_uid,
                                                        .pinned_rank = pinned_rank};

    const bool pin_is_private = dialog_type == relation_service_modules::PrivateDialogType();
    const bool pin_is_group = dialog_type == relation_service_modules::GroupDialogType();
    if (relation_service_modules::ShouldRejectPositiveUid(uid) ||
        relation_service_modules::ShouldRejectDialogType(pin_is_private, pin_is_group))
    {
        response.error = ErrorCodes::Error_Json;
        return BuildRelationCommandResult(ID_PIN_DIALOG_RSP, ChatRelationDtos::ToJsonValue(response));
    }

    int normalized_peer_uid = 0;
    int64_t normalized_group_id = 0;
    if (pin_is_private)
    {
        normalized_peer_uid = peer_uid;
        const bool is_private_friend =
            normalized_peer_uid > 0 && _relation_repository->IsPrivateFriend(uid, normalized_peer_uid);
        if (relation_service_modules::ShouldRejectPrivateDialogTarget(normalized_peer_uid, is_private_friend))
        {
            response.error = ErrorCodes::GroupPermissionDenied;
            return BuildRelationCommandResult(ID_PIN_DIALOG_RSP, ChatRelationDtos::ToJsonValue(response));
        }
    }
    else
    {
        normalized_group_id = request_dto.group_id;
        response.group_id = normalized_group_id;
        const bool is_group_member =
            normalized_group_id > 0 && _relation_repository->IsGroupMember(normalized_group_id, uid);
        if (relation_service_modules::ShouldRejectGroupDialogTarget(normalized_group_id, is_group_member))
        {
            response.error = ErrorCodes::GroupPermissionDenied;
            return BuildRelationCommandResult(ID_PIN_DIALOG_RSP, ChatRelationDtos::ToJsonValue(response));
        }
    }

    if (!_relation_repository
             ->UpsertDialogPinned(uid, dialog_type, normalized_peer_uid, normalized_group_id, pinned_rank))
    {
        response.error = ErrorCodes::RPCFailed;
    }
    return BuildRelationCommandResult(ID_PIN_DIALOG_RSP, ChatRelationDtos::ToJsonValue(response));
}
