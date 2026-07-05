#include "GroupMessageHistoryService.hpp"

#include "ChatGroupCommandDtos.hpp"
#include "ChatHistoryCommandDtos.hpp"
#include "ChatMessageCommandDtos.hpp"
#include "GroupResponseFormatter.hpp"
#include "MessageServiceUtil.hpp"
#include "const.hpp"
#include "logging/Logger.hpp"
#include "ports/IDeliveryGateway.hpp"
#include "ports/IMessageRepository.hpp"
#include "ports/IRelationRepository.hpp"

#include "json/GlazeCompat.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace
{
using memochat::chat::message::JsonToWireString;
using memochat::chat::message::NowMs;
namespace GroupResponseFormatter = memochat::chat::message::GroupResponseFormatter;
namespace ChatGroupCommand = memochat::chat::group;
namespace ChatHistoryCommand = memochat::chat::history;
namespace ChatMessageCommand = memochat::chat::command;

constexpr int kMaxHistoryLimit = 200;

int AuthenticatedGroupRequestUidLocal(const MessageCommandRequest& request, int payload_uid)
{
    return request.session_uid > 0 ? request.session_uid : payload_uid;
}
} // namespace

GroupMessageHistoryService::GroupMessageHistoryService(IMessageRepository* message_repository,
                                                       IRelationRepository* relation_repository,
                                                       IDeliveryGateway* delivery_gateway)
    : _message_repository(message_repository)
    , _relation_repository(relation_repository)
    , _delivery_gateway(delivery_gateway)
{
}

MessageCommandResult GroupMessageHistoryService::GroupHistory(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatHistoryCommand::ChatGroupHistoryRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
    const int64_t group_id = command.group_id;
    const int64_t before_ts = command.before_ts;
    const int64_t before_seq = command.before_seq;
    const int requested_limit = command.limit;

    memochat::json::JsonValue rtvalue =
        ChatGroupCommand::ToJsonValue(ChatGroupCommand::ChatGroupHistoryResponseDto{.error = ErrorCodes::Success,
                                                                                    .groupid = group_id,
                                                                                    .has_more = false,
                                                                                    .next_before_seq = 0});
    rtvalue["messages"] = memochat::json::arrayValue;
    const auto result = [&rtvalue]()
    {
        return MessageCommandResult{ID_GROUP_HISTORY_RSP, JsonToWireString(rtvalue)};
    };

    if (uid <= 0 || group_id <= 0 || requested_limit <= 0 || !_relation_repository->IsGroupMember(group_id, uid))
    {
        rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
        return result();
    }
    const int limit = std::clamp(requested_limit, 1, kMaxHistoryLimit);

    std::vector<std::shared_ptr<GroupMessageInfo>> msgs;
    bool has_more = false;
    const bool repository_success =
        _message_repository->GetGroupHistory(group_id, before_ts, before_seq, limit, msgs, has_more);
    memolog::LogInfo("chat.group.history.query",
                     "group history query completed",
                     {{"uid", std::to_string(uid)},
                      {"group_id", std::to_string(group_id)},
                      {"before_ts", std::to_string(before_ts)},
                      {"before_seq", std::to_string(before_seq)},
                      {"limit", std::to_string(limit)},
                      {"repository_success", repository_success ? "true" : "false"},
                      {"final_count", std::to_string(msgs.size())},
                      {"has_more", has_more ? "true" : "false"}});
    if (!repository_success)
    {
        rtvalue["error"] = ErrorCodes::RPCFailed;
        return result();
    }
    rtvalue["has_more"] = has_more;
    std::shared_ptr<GroupInfo> group_info;
    if (_relation_repository->GetGroupById(group_id, group_info) && group_info)
    {
        rtvalue["group_code"] = group_info->group_code;
    }

    for (const auto& one : msgs)
    {
        if (!one)
        {
            continue;
        }
        memochat::json::JsonValue item = GroupResponseFormatter::SerializeHistoryMessage(*one);
        if ((one->from_name.empty() || one->from_nick.empty()) && one->from_uid > 0)
        {
            auto from_user = _relation_repository->GetUserByUid(one->from_uid);
            if (from_user)
            {
                item["from_name"] = from_user->name;
                item["from_nick"] = from_user->nick;
                item["from_icon"] = from_user->icon;
            }
        }
        append(rtvalue["messages"], item);
    }
    if (!msgs.empty() && msgs.back())
    {
        rtvalue["next_before_seq"] = static_cast<int64_t>(msgs.back()->group_seq);
    }
    int64_t read_ts = 0;
    if (!msgs.empty() && msgs.front())
    {
        read_ts = msgs.front()->created_at;
    }
    if (read_ts <= 0)
    {
        read_ts = NowMs();
    }
    _message_repository->UpsertGroupReadState(uid, group_id, read_ts);
    return result();
}

MessageCommandResult GroupMessageHistoryService::GroupReadAck(const MessageCommandRequest& request)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(request.payload_json, root);
    const auto command = ChatGroupCommand::ChatGroupReadAckRequestFromJsonValue(root);
    const int uid = AuthenticatedGroupRequestUidLocal(request, command.uid);
    const int64_t group_id = command.group_id;
    int64_t read_ts = command.read_ts;
    const auto result = []()
    {
        return MessageCommandResult{0, "{}"};
    };
    if (uid <= 0 || group_id <= 0)
    {
        return result();
    }
    if (!_relation_repository->IsGroupMember(group_id, uid))
    {
        return result();
    }
    if (read_ts <= 0)
    {
        read_ts = NowMs();
    }
    _message_repository->UpsertGroupReadState(uid, group_id, read_ts);

    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    _relation_repository->GetGroupMemberList(group_id, members);
    std::vector<int> recipients;
    for (const auto& member : members)
    {
        if (member && member->status == 1)
        {
            recipients.push_back(member->uid);
        }
    }
    const auto notify =
        ChatMessageCommand::ToJsonValue(ChatMessageCommand::ChatGroupReadAckEventDto{.error = ErrorCodes::Success,
                                                                                     .event = "group_read_ack",
                                                                                     .groupid = group_id,
                                                                                     .fromuid = uid,
                                                                                     .read_ts = read_ts});
    _delivery_gateway->PushPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, uid);
    return result();
}
