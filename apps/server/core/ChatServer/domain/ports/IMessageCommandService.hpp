#pragma once

#include "json/GlazeCompat.hpp"

#include <string>

struct MessageCommandRequest
{
    short request_msg_id = 0;
    std::string payload_json;
    int session_uid = 0;
    std::string session_id;
    std::string server_name;
    std::string trace_id;
};

struct MessageCommandResult
{
    short response_msg_id = 0;
    std::string payload_json;
};

class IPrivateMessageCommandService
{
public:
    virtual ~IPrivateMessageCommandService() = default;

    virtual MessageCommandResult TextChatMessage(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult ForwardPrivateMessage(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult PrivateReadAck(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult EditPrivateMessage(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult RevokePrivateMessage(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult PrivateHistory(const MessageCommandRequest& request) = 0;
};

class IGroupMessageCommandService
{
public:
    virtual ~IGroupMessageCommandService() = default;

    virtual MessageCommandResult GroupChatMessage(const MessageCommandRequest& request) = 0;
    virtual void BuildGroupListJson(int uid, memochat::json::JsonValue& out) = 0;
    virtual MessageCommandResult CreateGroup(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult GetGroupList(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult InviteGroupMember(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult ApplyJoinGroup(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult ReviewGroupApply(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult GroupHistory(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult GroupReadAck(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult EditGroupMessage(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult RevokeGroupMessage(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult ForwardGroupMessage(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult UpdateGroupAnnouncement(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult UpdateGroupIcon(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult SetGroupAdmin(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult MuteGroupMember(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult KickGroupMember(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult QuitGroup(const MessageCommandRequest& request) = 0;
    virtual MessageCommandResult DissolveGroup(const MessageCommandRequest& request) = 0;
};
