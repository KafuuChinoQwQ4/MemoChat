#include "GroupMessageService.h"

#include "ChatRuntime.h"
#include "CSession.h"
#include "LogicSystem.h"
#include "MessageDeliveryService.h"
#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "logging/Logger.h"

#include <chrono>
#include <iostream>
#include "json/GlazeCompat.h"
#include <unordered_set>

namespace {
constexpr int64_t kPermChangeGroupInfoLocal = 1LL << 0;
constexpr int64_t kPermDeleteMessagesLocal = 1LL << 1;
constexpr int64_t kPermInviteUsersLocal = 1LL << 2;
constexpr int64_t kPermManageAdminsLocal = 1LL << 3;
constexpr int64_t kPermPinMessagesLocal = 1LL << 4;
constexpr int64_t kPermBanUsersLocal = 1LL << 5;
constexpr int64_t kPermManageTopicsLocal = 1LL << 6;
constexpr int64_t kDefaultAdminPermBitsLocal =
    kPermChangeGroupInfoLocal | kPermDeleteMessagesLocal | kPermInviteUsersLocal | kPermPinMessagesLocal | kPermBanUsersLocal;

int64_t NowMsGroupLocal() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

int64_t NowSecGroupLocal() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

bool ParseJsonObjectGroupLocal(const std::string& payload, memochat::json::JsonValue& root) {
    memochat::json::JsonCharReaderBuilder builder;
    std::unique_ptr<memochat::json::JsonCharReader> reader(builder.newCharReader());
    std::string errors;
    return reader->parse(payload.data(), payload.data() + payload.size(), &root, &errors) && root.is_object();
}

// Compact wire JSON for TCP/QUIC transport (Qt QJsonDocument is strict).
std::string JsonToWireString(const memochat::json::JsonValue& v) {
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, v);
}

bool KafkaBackendEnabledGroupLocal() {
    return memochat::chatruntime::AsyncEventBusBackend() == "kafka";
}
}

GroupMessageService::GroupMessageService(LogicSystem& logic)
    : _logic(logic) {
}

void GroupMessageService::BuildGroupListJson(int uid, memochat::json::JsonValue& out)
{
    out["group_list"] = memochat::json::arrayValue;
    out["pending_group_apply_list"] = memochat::json::arrayValue;

    std::vector<std::shared_ptr<GroupInfo>> groups;
    PostgresMgr::GetInstance()->GetUserGroupList(uid, groups);
    for (const auto& group : groups) {
        if (!group) {
            continue;
        }
        memochat::json::JsonValue one;
        one["groupid"] = static_cast<int64_t>(group->group_id);
        one["group_code"] = group->group_code;
        one["name"] = group->name;
        one["icon"] = group->icon;
        one["owner_uid"] = group->owner_uid;
        one["announcement"] = group->announcement;
        one["member_limit"] = group->member_limit;
        one["member_count"] = group->member_count;
        one["is_all_muted"] = group->is_all_muted;
        one["role"] = group->role;
        one["permission_bits"] = static_cast<int64_t>(group->permission_bits);
        one["can_change_group_info"] = (group->permission_bits & kPermChangeGroupInfoLocal) != 0;
        one["can_delete_messages"] = (group->permission_bits & kPermDeleteMessagesLocal) != 0;
        one["can_invite_users"] = (group->permission_bits & kPermInviteUsersLocal) != 0;
        one["can_manage_admins"] = (group->permission_bits & kPermManageAdminsLocal) != 0;
        one["can_pin_messages"] = (group->permission_bits & kPermPinMessagesLocal) != 0;
        one["can_ban_users"] = (group->permission_bits & kPermBanUsersLocal) != 0;
        one["can_manage_topics"] = (group->permission_bits & kPermManageTopicsLocal) != 0;
        out["group_list"].append(one);
    }

    std::vector<std::shared_ptr<GroupApplyInfo>> applies;
    PostgresMgr::GetInstance()->GetPendingGroupApplyForReviewer(uid, applies, 30);
    for (const auto& apply : applies) {
        if (!apply) {
            continue;
        }
        memochat::json::JsonValue one;
        one["apply_id"] = static_cast<int64_t>(apply->apply_id);
        one["groupid"] = static_cast<int64_t>(apply->group_id);
        std::shared_ptr<GroupInfo> group_info;
        if (PostgresMgr::GetInstance()->GetGroupById(apply->group_id, group_info) && group_info) {
            one["group_code"] = group_info->group_code;
        }
        one["applicant_uid"] = apply->applicant_uid;
        one["inviter_uid"] = apply->inviter_uid;
        auto applicant = PostgresMgr::GetInstance()->GetUser(apply->applicant_uid);
        if (applicant) {
            one["applicant_user_id"] = applicant->user_id;
        }
        if (apply->inviter_uid > 0) {
            auto inviter = PostgresMgr::GetInstance()->GetUser(apply->inviter_uid);
            if (inviter) {
                one["inviter_user_id"] = inviter->user_id;
            }
        }
        one["type"] = apply->type;
        one["status"] = apply->status;
        one["reason"] = apply->reason;
        out["pending_group_apply_list"].append(one);
    }
}

void GroupMessageService::HandleCreateGroup(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);

    const int owner_uid = root["fromuid"].asInt();
    const std::string group_name = root["name"].asString();
    const std::string announcement = root.get("announcement", "").asString();
    const int member_limit = root.get("member_limit", 200).asInt();
    std::vector<int> members;
    std::unordered_set<int> member_set;
    bool invalid_member_user_id = false;
    if (isMember(root, "member_user_ids") && root["member_user_ids"].is_array()) {
        for (const auto& one : root["member_user_ids"]) {
            const std::string member_user_id = one.asString();
            int uid = 0;
            if (!PostgresMgr::GetInstance()->GetUidByUserId(member_user_id, uid) || uid <= 0) {
                invalid_member_user_id = true;
                break;
            }
            if (uid == owner_uid) {
                continue;
            }
            member_set.insert(uid);
        }
    }
    for (int uid : member_set) {
        members.push_back(uid);
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_CREATE_GROUP_RSP);
    });

    if (owner_uid <= 0 || group_name.empty() || group_name.size() > 64 || invalid_member_user_id) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    for (int uid : members) {
        if (!PostgresMgr::GetInstance()->IsFriend(owner_uid, uid)) {
            rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
            return;
        }
    }

    int64_t group_id = 0;
    std::string group_code;
    if (!PostgresMgr::GetInstance()->CreateGroup(owner_uid, group_name, announcement, member_limit, members, group_id, group_code) || group_id <= 0) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }

    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["group_code"] = group_code;
    rtvalue["name"] = group_name;
    rtvalue["announcement"] = announcement;
    BuildGroupListJson(owner_uid, rtvalue);

    if (!members.empty()) {
        memochat::json::JsonValue notify;
        notify["error"] = ErrorCodes::Success;
        notify["event"] = "group_invite";
        notify["groupid"] = static_cast<int64_t>(group_id);
        notify["group_code"] = group_code;
        notify["name"] = group_name;
        notify["operator_uid"] = owner_uid;
        _logic.MessageDelivery().PushPayload(members, ID_NOTIFY_GROUP_INVITE_REQ, notify);
    }
}

void GroupMessageService::HandleGetGroupList(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    if (uid <= 0) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        session->Send(JsonToWireString(rtvalue), ID_GET_GROUP_LIST_RSP);
        return;
    }

    BuildGroupListJson(uid, rtvalue);
    session->Send(JsonToWireString(rtvalue), ID_GET_GROUP_LIST_RSP);
}

void GroupMessageService::HandleInviteGroupMember(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int from_uid = root["fromuid"].asInt();
    const std::string target_user_id = root.get("target_user_id", "").asString();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string reason = root.get("reason", "").asString();
    int to_uid = 0;
    if (!PostgresMgr::GetInstance()->GetUidByUserId(target_user_id, to_uid)) {
        to_uid = 0;
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["touid"] = to_uid;
    rtvalue["target_user_id"] = target_user_id;
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_INVITE_GROUP_MEMBER_RSP);
    });

    if (from_uid <= 0 || to_uid <= 0 || group_id <= 0 || target_user_id.empty()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (!PostgresMgr::GetInstance()->InviteGroupMember(group_id, from_uid, to_uid, reason)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::shared_ptr<GroupInfo> group_info;
    PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_invite";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["name"] = group_info ? group_info->name : "";
    notify["operator_uid"] = from_uid;
    notify["target_user_id"] = target_user_id;
    notify["reason"] = reason;
    _logic.MessageDelivery().PushPayload({ to_uid }, ID_NOTIFY_GROUP_INVITE_REQ, notify);
}

void GroupMessageService::HandleApplyJoinGroup(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int from_uid = root["fromuid"].asInt();
    const std::string group_code = root.get("group_code", "").asString();
    int64_t group_id = 0;
    if (!PostgresMgr::GetInstance()->GetGroupIdByCode(group_code, group_id)) {
        group_id = 0;
    }
    const std::string reason = root.get("reason", "").asString();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["group_code"] = group_code;
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_APPLY_JOIN_GROUP_RSP);
    });

    if (from_uid <= 0 || group_id <= 0 || group_code.empty()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (!PostgresMgr::GetInstance()->ApplyJoinGroup(group_id, from_uid, reason)) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> admins;
    for (const auto& one : members) {
        if (one && one->role >= 2) {
            admins.push_back(one->uid);
        }
    }
    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_apply";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_code;
    notify["applicant_uid"] = from_uid;
    auto applicant = PostgresMgr::GetInstance()->GetUser(from_uid);
    if (applicant) {
        notify["applicant_user_id"] = applicant->user_id;
    }
    notify["reason"] = reason;
    _logic.MessageDelivery().PushPayload(admins, ID_NOTIFY_GROUP_APPLY_REQ, notify);
}

void GroupMessageService::HandleReviewGroupApply(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int reviewer_uid = root["fromuid"].asInt();
    const int64_t apply_id = root["apply_id"].asInt64();
    const bool agree = root.get("agree", false).asBool();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["apply_id"] = static_cast<int64_t>(apply_id);
    rtvalue["agree"] = agree;
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_REVIEW_GROUP_APPLY_RSP);
    });

    if (reviewer_uid <= 0 || apply_id <= 0) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    std::shared_ptr<GroupApplyInfo> apply_info;
    if (!PostgresMgr::GetInstance()->ReviewGroupApply(apply_id, reviewer_uid, agree, apply_info) || !apply_info) {
        rtvalue["error"] = ErrorCodes::GroupApplyNotFound;
        return;
    }

    rtvalue["groupid"] = static_cast<int64_t>(apply_info->group_id);
    rtvalue["applicant_uid"] = apply_info->applicant_uid;
    std::shared_ptr<GroupInfo> apply_group;
    if (PostgresMgr::GetInstance()->GetGroupById(apply_info->group_id, apply_group) && apply_group) {
        rtvalue["group_code"] = apply_group->group_code;
    }
    auto applicant = PostgresMgr::GetInstance()->GetUser(apply_info->applicant_uid);
    if (applicant) {
        rtvalue["applicant_user_id"] = applicant->user_id;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(apply_info->group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members) {
        if (one) {
            recipients.push_back(one->uid);
        }
    }
    recipients.push_back(apply_info->applicant_uid);

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_member_changed";
    notify["groupid"] = static_cast<int64_t>(apply_info->group_id);
    notify["group_code"] = apply_group ? apply_group->group_code : "";
    notify["applicant_uid"] = apply_info->applicant_uid;
    if (applicant) {
        notify["applicant_user_id"] = applicant->user_id;
    }
    notify["agree"] = agree;
    notify["operator_uid"] = reviewer_uid;
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void GroupMessageService::HandleGroupChatMessage(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonValue root;
    ParseJsonObjectGroupLocal(msg_data, root);
    const int from_uid = memochat::json::glaze_safe_get<int>(root, "fromuid", 0);
    const int64_t group_id = memochat::json::glaze_safe_get<int64_t>(root, "groupid", 0);
    const auto msg = root.get("msg");
    const std::string client_msg_id = memochat::json::glaze_safe_get<std::string>(msg, "msgid", "");
    const bool kafka_backend = KafkaBackendEnabledGroupLocal();
    const bool kafka_primary = kafka_backend && memochat::chatruntime::FeatureEnabled("chat_group_kafka_primary");
    const bool kafka_shadow = kafka_backend && !kafka_primary && memochat::chatruntime::FeatureEnabled("chat_group_kafka_shadow");

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = from_uid;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    if (!client_msg_id.empty()) {
        rtvalue["client_msg_id"] = client_msg_id;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_GROUP_CHAT_MSG_RSP);
    });

    if (from_uid <= 0 || group_id <= 0 || !msg.isObject()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    int role = 0;
    if (!PostgresMgr::GetInstance()->GetUserRoleInGroup(group_id, from_uid, role)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    const auto now_sec = NowSecGroupLocal();
    const auto now_ms = NowMsGroupLocal();
    for (const auto& member : members) {
        if (member && member->uid == from_uid && member->mute_until > now_sec) {
            rtvalue["error"] = ErrorCodes::GroupMuted;
            return;
        }
    }

    const bool mention_all = memochat::json::glaze_safe_get<bool>(msg, "mention_all", false);
    if (mention_all && role < 2) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    GroupMessageInfo info;
    info.msg_id = client_msg_id;
    info.group_id = group_id;
    info.from_uid = from_uid;
    info.msg_type = memochat::json::glaze_safe_get<std::string>(msg, "msgtype", "text");
    info.content = memochat::json::glaze_safe_get<std::string>(msg, "content", "");
    if (memochat::json::isMember(msg, "mentions")) {
        info.mentions_json = msg["mentions"].toStyledString();
    } else {
        info.mentions_json = "[]";
    }
    info.file_name = memochat::json::glaze_safe_get<std::string>(msg, "file_name", "");
    info.mime = memochat::json::glaze_safe_get<std::string>(msg, "mime", "");
    info.size = memochat::json::glaze_safe_get<int>(msg, "size", 0);
    info.reply_to_server_msg_id = memochat::json::glaze_safe_get<int64_t>(msg, "reply_to_server_msg_id", 0);
    if (memochat::json::isMember(msg, "forward_meta")) {
        info.forward_meta_json = msg["forward_meta"].toStyledString();
    }
    info.edited_at_ms = memochat::json::glaze_safe_get<int64_t>(msg, "edited_at_ms", 0);
    info.deleted_at_ms = memochat::json::glaze_safe_get<int64_t>(msg, "deleted_at_ms", 0);
    info.created_at = now_ms;
    if (info.msg_id.empty() || info.content.empty()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }

    const auto accept_ts = NowMsGroupLocal();
    rtvalue["accept_node"] = memochat::chatruntime::SelfServerName();
    rtvalue["accept_ts"] = static_cast<int64_t>(accept_ts);
    rtvalue["status"] = kafka_primary ? "accepted" : "persisted";

    auto sender_info = PostgresMgr::GetInstance()->GetUser(from_uid);
    std::shared_ptr<GroupInfo> group_info;
    PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
    if (sender_info) {
        rtvalue["from_name"] = sender_info->name;
        rtvalue["from_nick"] = sender_info->nick;
        rtvalue["from_icon"] = sender_info->icon;
        rtvalue["from_user_id"] = sender_info->user_id;
    }
    if (group_info) {
        rtvalue["group_code"] = group_info->group_code;
    }

    memochat::json::JsonValue event_payload;
    event_payload["fromuid"] = from_uid;
    event_payload["groupid"] = static_cast<int64_t>(group_id);
    event_payload["trace_id"] = root.get("trace_id", "").asString();
    event_payload["request_id"] = root.get("request_id", "").asString();
    event_payload["span_id"] = root.get("span_id", "").asString();
    event_payload["event_id"] = client_msg_id;
    event_payload["accept_node"] = memochat::chatruntime::SelfServerName();
    event_payload["accept_ts"] = static_cast<int64_t>(accept_ts);
    event_payload["msg"] = msg;
    if (sender_info) {
        event_payload["from_name"] = sender_info->name;
        event_payload["from_nick"] = sender_info->nick;
        event_payload["from_icon"] = sender_info->icon;
        event_payload["from_user_id"] = sender_info->user_id;
    }
    if (group_info) {
        event_payload["group_code"] = group_info->group_code;
    }

    if (kafka_primary || kafka_shadow) {
        std::string publish_error;
        if (!_logic.PublishAsyncEvent(memochat::chatruntime::TopicGroup(), event_payload, &publish_error)) {
            if (kafka_primary) {
                rtvalue["error"] = ErrorCodes::RPCFailed;
                rtvalue["status"] = "failed";
                return;
            }
            memolog::LogWarn("chat.group.shadow_publish_failed", "group shadow publish failed",
                { {"error", publish_error}, {"client_msg_id", client_msg_id} });
        }
    }

    if (kafka_primary) {
        return;
    }

    int64_t server_msg_id = 0;
    int64_t group_seq = 0;
    if (!PostgresMgr::GetInstance()->SaveGroupMessage(info, &server_msg_id, &group_seq)) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        rtvalue["status"] = "failed";
        return;
    }
    info.server_msg_id = server_msg_id;
    info.group_seq = group_seq;
    if (sender_info) {
        info.from_name = sender_info->name;
        info.from_nick = sender_info->nick;
        info.from_icon = sender_info->icon;
    }
    if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SaveGroupMessage(info)) {
        std::cerr << "[MongoMgr] SaveGroupMessage dual-write failed for msg_id=" << info.msg_id << std::endl;
    }

    memochat::json::JsonValue msg_out = msg;
    msg_out["created_at"] = static_cast<int64_t>(now_ms);
    msg_out["server_msg_id"] = static_cast<int64_t>(server_msg_id);
    msg_out["group_seq"] = static_cast<int64_t>(group_seq);
    rtvalue["msg"] = msg_out;
    rtvalue["created_at"] = static_cast<int64_t>(now_ms);
    rtvalue["server_msg_id"] = static_cast<int64_t>(server_msg_id);
    rtvalue["group_seq"] = static_cast<int64_t>(group_seq);

    std::vector<int> recipients;
    for (const auto& member : members) {
        if (member && member->status == 1) {
            recipients.push_back(member->uid);
        }
    }
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_CHAT_MSG_REQ, rtvalue, from_uid);
}

void GroupMessageService::HandleGroupHistory(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const int64_t before_ts = root.get("before_ts", 0).asInt64();
    const int64_t before_seq = root.get("before_seq", 0).asInt64();
    const int limit = root.get("limit", 20).asInt();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["has_more"] = false;
    rtvalue["next_before_seq"] = static_cast<int64_t>(0);
    rtvalue["messages"] = memochat::json::arrayValue;
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_GROUP_HISTORY_RSP);
    });

    if (uid <= 0 || group_id <= 0 || !PostgresMgr::GetInstance()->IsUserInGroup(group_id, uid)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::vector<std::shared_ptr<GroupMessageInfo>> msgs;
    bool has_more = false;
    bool mongo_success = false;
    bool pg_success = false;

    if (MongoMgr::GetInstance()->Enabled()) {
        mongo_success = MongoMgr::GetInstance()->GetGroupHistory(group_id, before_ts, before_seq, limit, msgs, has_more);
    }
    const size_t mongo_count = msgs.size();
    if (!mongo_success || msgs.empty()) {
        pg_success = PostgresMgr::GetInstance()->GetGroupHistory(group_id, before_ts, before_seq, limit, msgs, has_more);
    }
    memolog::LogInfo("chat.group.history.query",
        "group history query completed",
        { {"uid", std::to_string(uid)},
          {"group_id", std::to_string(group_id)},
          {"before_ts", std::to_string(before_ts)},
          {"before_seq", std::to_string(before_seq)},
          {"limit", std::to_string(limit)},
          {"mongo_enabled", MongoMgr::GetInstance()->Enabled() ? "true" : "false"},
          {"mongo_success", mongo_success ? "true" : "false"},
          {"mongo_count", std::to_string(mongo_count)},
          {"pg_success", pg_success ? "true" : "false"},
          {"final_count", std::to_string(msgs.size())},
          {"has_more", has_more ? "true" : "false"} });
    if (!mongo_success && !pg_success) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }
    rtvalue["has_more"] = has_more;
    std::shared_ptr<GroupInfo> group_info;
    if (PostgresMgr::GetInstance()->GetGroupById(group_id, group_info) && group_info) {
        rtvalue["group_code"] = group_info->group_code;
    }

    for (const auto& one : msgs) {
        if (!one) {
            continue;
        }
        memochat::json::JsonValue item;
        item["msgid"] = one->msg_id;
        item["groupid"] = static_cast<int64_t>(one->group_id);
        item["fromuid"] = one->from_uid;
        item["msgtype"] = one->msg_type;
        item["content"] = one->content;
        memochat::json::JsonValue mentions(memochat::json::array_t{});
        if (!one->mentions_json.empty()) {
            memochat::json::JsonReader mentions_reader;
            memochat::json::JsonValue parsed_mentions;
            if (mentions_reader.parse(one->mentions_json, parsed_mentions) && parsed_mentions.is_array()) {
                mentions = parsed_mentions;
            }
        }
        item["mentions"] = mentions;
        if (!one->file_name.empty()) {
            item["file_name"] = one->file_name;
        }
        if (!one->mime.empty()) {
            item["mime"] = one->mime;
        }
        if (one->size > 0) {
            item["size"] = one->size;
        }
        item["created_at"] = static_cast<int64_t>(one->created_at);
        item["server_msg_id"] = static_cast<int64_t>(one->server_msg_id);
        item["group_seq"] = static_cast<int64_t>(one->group_seq);
        if (one->reply_to_server_msg_id > 0) {
            item["reply_to_server_msg_id"] = static_cast<int64_t>(one->reply_to_server_msg_id);
        }
        if (!one->forward_meta_json.empty()) {
            memochat::json::JsonReader forward_reader;
            memochat::json::JsonValue forward_meta;
            if (forward_reader.parse(one->forward_meta_json, forward_meta)) {
                item["forward_meta"] = forward_meta;
            }
        }
        if (one->edited_at_ms > 0) {
            item["edited_at_ms"] = static_cast<int64_t>(one->edited_at_ms);
        }
        if (one->deleted_at_ms > 0) {
            item["deleted_at_ms"] = static_cast<int64_t>(one->deleted_at_ms);
        }
        item["from_name"] = one->from_name;
        item["from_nick"] = one->from_nick;
        item["from_icon"] = one->from_icon;
        if ((one->from_name.empty() || one->from_nick.empty()) && one->from_uid > 0) {
            auto from_user = PostgresMgr::GetInstance()->GetUser(one->from_uid);
            if (from_user) {
                item["from_name"] = from_user->name;
                item["from_nick"] = from_user->nick;
                item["from_icon"] = from_user->icon;
            }
        }
        append(rtvalue["messages"], item);
    }
    if (!msgs.empty() && msgs.back()) {
        rtvalue["next_before_seq"] = static_cast<int64_t>(msgs.back()->group_seq);
    }
    int64_t read_ts = 0;
    if (!msgs.empty() && msgs.front()) {
        read_ts = msgs.front()->created_at;
    }
    if (read_ts <= 0) {
        read_ts = NowMsGroupLocal();
    }
    PostgresMgr::GetInstance()->UpsertGroupReadState(uid, group_id, read_ts);
}

void GroupMessageService::HandleEditGroupMessage(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string target_msg_id = root.get("msgid", "").asString();
    const std::string content = root.get("content", "").asString();
    const int64_t now_ms = NowMsGroupLocal();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["msgid"] = target_msg_id;
    rtvalue["content"] = content;
    rtvalue["edited_at_ms"] = static_cast<int64_t>(now_ms);
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_EDIT_GROUP_MSG_RSP);
    });

    if (uid <= 0 || group_id <= 0 || target_msg_id.empty() || content.empty() || content.size() > 4096) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (!PostgresMgr::GetInstance()->IsUserInGroup(group_id, uid)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }
    if (!PostgresMgr::GetInstance()->UpdateGroupMessageContent(group_id, uid, target_msg_id, content, now_ms)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }
    if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->UpdateGroupMessageContent(group_id, uid, target_msg_id, content, now_ms)) {
        std::cerr << "[MongoMgr] UpdateGroupMessageContent sync failed for msg_id=" << target_msg_id << std::endl;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members) {
        if (member && member->status == 1) {
            recipients.push_back(member->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_msg_edited";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["msgid"] = target_msg_id;
    notify["content"] = content;
    notify["edited_at_ms"] = static_cast<int64_t>(now_ms);
    notify["operator_uid"] = uid;
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, 0);
}

void GroupMessageService::HandleRevokeGroupMessage(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string target_msg_id = root.get("msgid", "").asString();
    const int64_t now_ms = NowMsGroupLocal();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["msgid"] = target_msg_id;
    rtvalue["content"] = "[消息已撤回]";
    rtvalue["deleted_at_ms"] = static_cast<int64_t>(now_ms);
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_REVOKE_GROUP_MSG_RSP);
    });

    if (uid <= 0 || group_id <= 0 || target_msg_id.empty()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (!PostgresMgr::GetInstance()->IsUserInGroup(group_id, uid)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }
    if (!PostgresMgr::GetInstance()->RevokeGroupMessage(group_id, uid, target_msg_id, now_ms)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }
    if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->RevokeGroupMessage(group_id, uid, target_msg_id, now_ms)) {
        std::cerr << "[MongoMgr] RevokeGroupMessage sync failed for msg_id=" << target_msg_id << std::endl;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members) {
        if (member && member->status == 1) {
            recipients.push_back(member->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_msg_revoked";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["msgid"] = target_msg_id;
    notify["content"] = "[消息已撤回]";
    notify["deleted_at_ms"] = static_cast<int64_t>(now_ms);
    notify["operator_uid"] = uid;
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, 0);
}

void GroupMessageService::HandleForwardGroupMessage(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int from_uid = root.isMember("fromuid") ? root["fromuid"].asInt() : root["uid"].asInt();
    const int64_t group_id = root.get("groupid", 0).asInt64();
    const std::string source_msg_id = root.get("msgid", "").asString();
    std::string client_msg_id = root.get("client_msg_id", "").asString();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = from_uid;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    if (!client_msg_id.empty()) {
        rtvalue["client_msg_id"] = client_msg_id;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_FORWARD_GROUP_MSG_RSP);
    });

    if (from_uid <= 0 || group_id <= 0 || source_msg_id.empty()) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (!PostgresMgr::GetInstance()->IsUserInGroup(group_id, from_uid)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::shared_ptr<GroupMessageInfo> source_msg;
    bool mongo_success = false;
    if (MongoMgr::GetInstance()->Enabled()) {
        mongo_success = MongoMgr::GetInstance()->GetGroupMessageByMsgId(group_id, source_msg_id, source_msg) && source_msg;
    }
    if (!mongo_success) {
        if (!(PostgresMgr::GetInstance()->GetGroupMessageByMsgId(group_id, source_msg_id, source_msg) && source_msg)) {
            rtvalue["error"] = ErrorCodes::GroupNotFound;
            return;
        }
    }

    const int64_t now_ms = NowMsGroupLocal();
    if (client_msg_id.empty()) {
        client_msg_id = std::to_string(from_uid) + "_" + std::to_string(now_ms);
        rtvalue["client_msg_id"] = client_msg_id;
    }

    GroupMessageInfo info;
    info.msg_id = client_msg_id;
    info.group_id = group_id;
    info.from_uid = from_uid;
    info.msg_type = source_msg->msg_type;
    info.content = source_msg->content;
    info.mentions_json = source_msg->mentions_json;
    info.file_name = source_msg->file_name;
    info.mime = source_msg->mime;
    info.size = source_msg->size;
    info.reply_to_server_msg_id = source_msg->reply_to_server_msg_id;
    memochat::json::JsonValue forward_meta;
    forward_meta["forwarded_from_msgid"] = source_msg_id;
    forward_meta["source_server_msg_id"] = static_cast<int64_t>(source_msg->server_msg_id);
    forward_meta["source_group_seq"] = static_cast<int64_t>(source_msg->group_seq);
    forward_meta["source_from_uid"] = source_msg->from_uid;
    if (!source_msg->forward_meta_json.empty()) {
        memochat::json::JsonReader prev_forward_reader;
        memochat::json::JsonValue prev_forward_meta;
        if (prev_forward_reader.parse(source_msg->forward_meta_json, prev_forward_meta)) {
            forward_meta["prev_forward_meta"] = prev_forward_meta;
        }
    }
    info.forward_meta_json = forward_meta.and_then([](auto&& v){ return glz::write_json(v); }).value_or("{}");
    info.created_at = now_ms;

    int64_t server_msg_id = 0;
    int64_t group_seq = 0;
    if (!PostgresMgr::GetInstance()->SaveGroupMessage(info, &server_msg_id, &group_seq)) {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return;
    }
    info.server_msg_id = server_msg_id;
    info.group_seq = group_seq;
    auto sender_info_for_group_mongo = PostgresMgr::GetInstance()->GetUser(from_uid);
    if (sender_info_for_group_mongo) {
        info.from_name = sender_info_for_group_mongo->name;
        info.from_nick = sender_info_for_group_mongo->nick;
        info.from_icon = sender_info_for_group_mongo->icon;
    }
    if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SaveGroupMessage(info)) {
        std::cerr << "[MongoMgr] SaveGroupMessage dual-write failed for msg_id=" << info.msg_id << std::endl;
    }

    memochat::json::JsonValue forwarded_msg;
    forwarded_msg["msgid"] = info.msg_id;
    forwarded_msg["msgtype"] = info.msg_type;
    forwarded_msg["content"] = info.content;
    memochat::json::JsonValue mentions(memochat::json::array_t{});
    if (!info.mentions_json.empty()) {
        memochat::json::JsonReader mentions_reader;
        memochat::json::JsonValue parsed_mentions;
        if (mentions_reader.parse(info.mentions_json, parsed_mentions) && parsed_mentions.is_array()) {
            mentions = parsed_mentions;
        }
    }
    forwarded_msg["mentions"] = mentions;
    if (!info.file_name.empty()) {
        forwarded_msg["file_name"] = info.file_name;
    }
    if (!info.mime.empty()) {
        forwarded_msg["mime"] = info.mime;
    }
    if (info.size > 0) {
        forwarded_msg["size"] = info.size;
    }
    forwarded_msg["forwarded_from_msgid"] = source_msg_id;
    forwarded_msg["created_at"] = static_cast<int64_t>(now_ms);
    forwarded_msg["server_msg_id"] = static_cast<int64_t>(server_msg_id);
    forwarded_msg["group_seq"] = static_cast<int64_t>(group_seq);
    if (info.reply_to_server_msg_id > 0) {
        forwarded_msg["reply_to_server_msg_id"] = static_cast<int64_t>(info.reply_to_server_msg_id);
    }
    {
        memochat::json::JsonReader forward_reader;
        memochat::json::JsonValue parsed_forward_meta;
        if (forward_reader.parse(info.forward_meta_json, parsed_forward_meta)) {
            forwarded_msg["forward_meta"] = parsed_forward_meta;
        }
    }

    rtvalue["msg"] = forwarded_msg;
    rtvalue["created_at"] = static_cast<int64_t>(now_ms);
    rtvalue["server_msg_id"] = static_cast<int64_t>(server_msg_id);
    rtvalue["group_seq"] = static_cast<int64_t>(group_seq);
    if (info.reply_to_server_msg_id > 0) {
        rtvalue["reply_to_server_msg_id"] = static_cast<int64_t>(info.reply_to_server_msg_id);
    }
    rtvalue["forward_meta"] = forward_meta;

    auto sender_info = PostgresMgr::GetInstance()->GetUser(from_uid);
    if (sender_info) {
        rtvalue["from_name"] = sender_info->name;
        rtvalue["from_nick"] = sender_info->nick;
        rtvalue["from_icon"] = sender_info->icon;
        rtvalue["from_user_id"] = sender_info->user_id;
    }
    std::shared_ptr<GroupInfo> group_info;
    if (PostgresMgr::GetInstance()->GetGroupById(group_id, group_info) && group_info) {
        rtvalue["group_code"] = group_info->group_code;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members) {
        if (member && member->status == 1) {
            recipients.push_back(member->uid);
        }
    }
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_CHAT_MSG_REQ, rtvalue, from_uid);
}

void GroupMessageService::HandleGroupReadAck(const std::shared_ptr<CSession>&, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root.isMember("fromuid") ? root["fromuid"].asInt() : root["uid"].asInt();
    const int64_t group_id = root.get("groupid", 0).asInt64();
    int64_t read_ts = root.get("read_ts", 0).asInt64();
    if (uid <= 0 || group_id <= 0) {
        return;
    }
    if (!PostgresMgr::GetInstance()->IsUserInGroup(group_id, uid)) {
        return;
    }
    if (read_ts <= 0) {
        read_ts = NowMsGroupLocal();
    }
    PostgresMgr::GetInstance()->UpsertGroupReadState(uid, group_id, read_ts);

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members) {
        if (member && member->status == 1) {
            recipients.push_back(member->uid);
        }
    }
    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_read_ack";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["fromuid"] = uid;
    notify["read_ts"] = static_cast<int64_t>(read_ts);
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, uid);
}

void GroupMessageService::HandleUpdateGroupAnnouncement(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string announcement = root.get("announcement", "").asString();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["announcement"] = announcement;
    std::shared_ptr<GroupInfo> group_info;
    PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
    if (group_info) {
        rtvalue["group_code"] = group_info->group_code;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_UPDATE_GROUP_ANNOUNCEMENT_RSP);
    });

    if (!PostgresMgr::GetInstance()->UpdateGroupAnnouncement(group_id, uid, announcement)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members) {
        if (one) {
            recipients.push_back(one->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_announcement_updated";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["announcement"] = announcement;
    notify["operator_uid"] = uid;
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void GroupMessageService::HandleUpdateGroupIcon(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();
    const std::string icon = root.get("icon", "").asString();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["icon"] = icon;
    std::shared_ptr<GroupInfo> group_info;
    PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
    if (group_info) {
        rtvalue["group_code"] = group_info->group_code;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_UPDATE_GROUP_ICON_RSP);
    });

    if (uid <= 0 || group_id <= 0 || icon.empty() || icon.size() > 512) {
        rtvalue["error"] = ErrorCodes::Error_Json;
        return;
    }
    if (!PostgresMgr::GetInstance()->UpdateGroupIcon(group_id, uid, icon)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members) {
        if (one) {
            recipients.push_back(one->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_icon_updated";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["icon"] = icon;
    notify["operator_uid"] = uid;
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void GroupMessageService::HandleSetGroupAdmin(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const std::string target_user_id = root.get("target_user_id", "").asString();
    const int64_t group_id = root["groupid"].asInt64();
    const bool is_admin = root.get("is_admin", true).asBool();
    bool has_permission_bits = isMember(root, "permission_bits");
    int64_t permission_bits = root.get("permission_bits", 0).asInt64();
    auto merge_perm_flag = [&](const char* key, int64_t bit) {
        if (!root.isMember(key)) {
            return;
        }
        has_permission_bits = true;
        if (root.get(key, false).asBool()) {
            permission_bits |= bit;
        } else {
            permission_bits &= ~bit;
        }
    };
    merge_perm_flag("can_change_group_info", kPermChangeGroupInfoLocal);
    merge_perm_flag("can_delete_messages", kPermDeleteMessagesLocal);
    merge_perm_flag("can_invite_users", kPermInviteUsersLocal);
    merge_perm_flag("can_manage_admins", kPermManageAdminsLocal);
    merge_perm_flag("can_pin_messages", kPermPinMessagesLocal);
    merge_perm_flag("can_ban_users", kPermBanUsersLocal);
    merge_perm_flag("can_manage_topics", kPermManageTopicsLocal);
    if (!is_admin) {
        permission_bits = 0;
        has_permission_bits = true;
    }
    int64_t requested_permission_bits = has_permission_bits ? permission_bits : 0;
    if (is_admin && !has_permission_bits && requested_permission_bits <= 0) {
        requested_permission_bits = kDefaultAdminPermBitsLocal;
    }
    int target_uid = 0;
    if (!PostgresMgr::GetInstance()->GetUidByUserId(target_user_id, target_uid)) {
        target_uid = 0;
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["touid"] = target_uid;
    rtvalue["target_user_id"] = target_user_id;
    rtvalue["is_admin"] = is_admin;
    rtvalue["permission_bits"] = static_cast<int64_t>(requested_permission_bits);
    rtvalue["can_change_group_info"] = (requested_permission_bits & kPermChangeGroupInfoLocal) != 0;
    rtvalue["can_delete_messages"] = (requested_permission_bits & kPermDeleteMessagesLocal) != 0;
    rtvalue["can_invite_users"] = (requested_permission_bits & kPermInviteUsersLocal) != 0;
    rtvalue["can_manage_admins"] = (requested_permission_bits & kPermManageAdminsLocal) != 0;
    rtvalue["can_pin_messages"] = (requested_permission_bits & kPermPinMessagesLocal) != 0;
    rtvalue["can_ban_users"] = (requested_permission_bits & kPermBanUsersLocal) != 0;
    rtvalue["can_manage_topics"] = (requested_permission_bits & kPermManageTopicsLocal) != 0;
    std::shared_ptr<GroupInfo> group_info;
    PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
    if (group_info) {
        rtvalue["group_code"] = group_info->group_code;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_SET_GROUP_ADMIN_RSP);
    });

    if (target_uid <= 0 || target_user_id.empty() ||
        !PostgresMgr::GetInstance()->SetGroupAdmin(group_id, uid, target_uid, is_admin, requested_permission_bits)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members) {
        if (one) {
            recipients.push_back(one->uid);
        }
    }
    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_admin_changed";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["operator_uid"] = uid;
    notify["target_uid"] = target_uid;
    notify["target_user_id"] = target_user_id;
    notify["is_admin"] = is_admin;
    notify["permission_bits"] = static_cast<int64_t>(requested_permission_bits);
    notify["can_change_group_info"] = (requested_permission_bits & kPermChangeGroupInfoLocal) != 0;
    notify["can_delete_messages"] = (requested_permission_bits & kPermDeleteMessagesLocal) != 0;
    notify["can_invite_users"] = (requested_permission_bits & kPermInviteUsersLocal) != 0;
    notify["can_manage_admins"] = (requested_permission_bits & kPermManageAdminsLocal) != 0;
    notify["can_pin_messages"] = (requested_permission_bits & kPermPinMessagesLocal) != 0;
    notify["can_ban_users"] = (requested_permission_bits & kPermBanUsersLocal) != 0;
    notify["can_manage_topics"] = (requested_permission_bits & kPermManageTopicsLocal) != 0;
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void GroupMessageService::HandleMuteGroupMember(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const std::string target_user_id = root.get("target_user_id", "").asString();
    const int64_t group_id = root["groupid"].asInt64();
    const int mute_seconds = root.get("mute_seconds", 0).asInt();
    int target_uid = 0;
    if (!PostgresMgr::GetInstance()->GetUidByUserId(target_user_id, target_uid)) {
        target_uid = 0;
    }
    const int64_t mute_until = (mute_seconds > 0) ? NowSecGroupLocal() + mute_seconds : 0;

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["touid"] = target_uid;
    rtvalue["target_user_id"] = target_user_id;
    rtvalue["mute_until"] = static_cast<int64_t>(mute_until);
    std::shared_ptr<GroupInfo> group_info;
    PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
    if (group_info) {
        rtvalue["group_code"] = group_info->group_code;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_MUTE_GROUP_MEMBER_RSP);
    });

    if (target_uid <= 0 || target_user_id.empty() ||
        !PostgresMgr::GetInstance()->MuteGroupMember(group_id, uid, target_uid, mute_until)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members) {
        if (one) {
            recipients.push_back(one->uid);
        }
    }
    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_mute_changed";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["operator_uid"] = uid;
    notify["target_uid"] = target_uid;
    notify["target_user_id"] = target_user_id;
    notify["mute_until"] = static_cast<int64_t>(mute_until);
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void GroupMessageService::HandleKickGroupMember(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const std::string target_user_id = root.get("target_user_id", "").asString();
    const int64_t group_id = root["groupid"].asInt64();
    int target_uid = 0;
    if (!PostgresMgr::GetInstance()->GetUidByUserId(target_user_id, target_uid)) {
        target_uid = 0;
    }

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    rtvalue["touid"] = target_uid;
    rtvalue["target_user_id"] = target_user_id;
    std::shared_ptr<GroupInfo> group_info;
    PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
    if (group_info) {
        rtvalue["group_code"] = group_info->group_code;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_KICK_GROUP_MEMBER_RSP);
    });

    if (target_uid <= 0 || target_user_id.empty() ||
        !PostgresMgr::GetInstance()->KickGroupMember(group_id, uid, target_uid)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members) {
        if (one) {
            recipients.push_back(one->uid);
        }
    }
    recipients.push_back(target_uid);

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_member_kicked";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["operator_uid"] = uid;
    notify["target_uid"] = target_uid;
    notify["target_user_id"] = target_user_id;
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void GroupMessageService::HandleQuitGroup(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);
    std::shared_ptr<GroupInfo> group_info;
    PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
    if (group_info) {
        rtvalue["group_code"] = group_info->group_code;
    }
    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_QUIT_GROUP_RSP);
    });

    if (!PostgresMgr::GetInstance()->QuitGroup(group_id, uid)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members) {
        if (one) {
            recipients.push_back(one->uid);
        }
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_member_quit";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["target_uid"] = uid;
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void GroupMessageService::HandleDissolveGroup(const std::shared_ptr<CSession>& session, short, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    const int uid = root["fromuid"].asInt();
    const int64_t group_id = root["groupid"].asInt64();

    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["groupid"] = static_cast<int64_t>(group_id);

    std::shared_ptr<GroupInfo> group_info;
    PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
    if (group_info) {
        rtvalue["group_code"] = group_info->group_code;
        rtvalue["name"] = group_info->name;
    }

    Defer defer([&rtvalue, session]() {
        session->Send(JsonToWireString(rtvalue), ID_DISSOLVE_GROUP_RSP);
    });

    if (uid <= 0 || group_id <= 0 || !group_info || group_info->status != 1) {
        rtvalue["error"] = ErrorCodes::GroupNotFound;
        return;
    }

    if (group_info->owner_uid != uid) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& one : members) {
        if (one && one->status == 1) {
            recipients.push_back(one->uid);
        }
    }

    if (!PostgresMgr::GetInstance()->DissolveGroup(group_id, uid)) {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return;
    }

    memochat::json::JsonValue notify;
    notify["error"] = ErrorCodes::Success;
    notify["event"] = "group_dissolved";
    notify["groupid"] = static_cast<int64_t>(group_id);
    notify["group_code"] = group_info ? group_info->group_code : "";
    notify["name"] = group_info ? group_info->name : "";
    notify["operator_uid"] = uid;
    _logic.MessageDelivery().PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}
