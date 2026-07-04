export module memochat.chat.message_grpc_client_algorithms;

export namespace memochat::chat::message_grpc_client::modules
{
const char* UnknownMethod()
{
    return "unknown";
}

const char* BuildGroupListMethod()
{
    return "BuildGroupList";
}

const char* SendTextChatMessageMethod()
{
    return "SendTextChatMessage";
}

const char* ForwardPrivateMessageMethod()
{
    return "ForwardPrivateMessage";
}

const char* PrivateReadAckMethod()
{
    return "PrivateReadAck";
}

const char* EditPrivateMessageMethod()
{
    return "EditPrivateMessage";
}

const char* RevokePrivateMessageMethod()
{
    return "RevokePrivateMessage";
}

const char* PrivateHistoryMethod()
{
    return "PrivateHistory";
}

const char* PrivateCommandMethodName(int rpc)
{
    switch (rpc)
    {
        case 0:
            return SendTextChatMessageMethod();
        case 1:
            return ForwardPrivateMessageMethod();
        case 2:
            return PrivateReadAckMethod();
        case 3:
            return EditPrivateMessageMethod();
        case 4:
            return RevokePrivateMessageMethod();
        case 5:
            return PrivateHistoryMethod();
    }
    return UnknownMethod();
}

const char* CreateGroupMethod()
{
    return "CreateGroup";
}

const char* GetGroupListMethod()
{
    return "GetGroupList";
}

const char* InviteGroupMemberMethod()
{
    return "InviteGroupMember";
}

const char* ApplyJoinGroupMethod()
{
    return "ApplyJoinGroup";
}

const char* ReviewGroupApplyMethod()
{
    return "ReviewGroupApply";
}

const char* GroupChatMessageMethod()
{
    return "GroupChatMessage";
}

const char* GroupHistoryMethod()
{
    return "GroupHistory";
}

const char* EditGroupMessageMethod()
{
    return "EditGroupMessage";
}

const char* RevokeGroupMessageMethod()
{
    return "RevokeGroupMessage";
}

const char* ForwardGroupMessageMethod()
{
    return "ForwardGroupMessage";
}

const char* GroupReadAckMethod()
{
    return "GroupReadAck";
}

const char* UpdateGroupAnnouncementMethod()
{
    return "UpdateGroupAnnouncement";
}

const char* UpdateGroupIconMethod()
{
    return "UpdateGroupIcon";
}

const char* SetGroupAdminMethod()
{
    return "SetGroupAdmin";
}

const char* MuteGroupMemberMethod()
{
    return "MuteGroupMember";
}

const char* KickGroupMemberMethod()
{
    return "KickGroupMember";
}

const char* QuitGroupMethod()
{
    return "QuitGroup";
}

const char* DissolveGroupMethod()
{
    return "DissolveGroup";
}

const char* GroupCommandMethodName(int rpc)
{
    switch (rpc)
    {
        case 0:
            return CreateGroupMethod();
        case 1:
            return GetGroupListMethod();
        case 2:
            return InviteGroupMemberMethod();
        case 3:
            return ApplyJoinGroupMethod();
        case 4:
            return ReviewGroupApplyMethod();
        case 5:
            return GroupChatMessageMethod();
        case 6:
            return GroupHistoryMethod();
        case 7:
            return EditGroupMessageMethod();
        case 8:
            return RevokeGroupMessageMethod();
        case 9:
            return ForwardGroupMessageMethod();
        case 10:
            return GroupReadAckMethod();
        case 11:
            return UpdateGroupAnnouncementMethod();
        case 12:
            return UpdateGroupIconMethod();
        case 13:
            return SetGroupAdminMethod();
        case 14:
            return MuteGroupMemberMethod();
        case 15:
            return KickGroupMemberMethod();
        case 16:
            return QuitGroupMethod();
        case 17:
            return DissolveGroupMethod();
    }
    return UnknownMethod();
}

const char* ErrorField()
{
    return "error";
}

const char* RemoteMethodField()
{
    return "message_remote_method";
}

const char* RemoteErrorField()
{
    return "message_remote_error";
}

const char* RemoteStatusCodeField()
{
    return "message_remote_status_code";
}

const char* RemoteErrorCodeField()
{
    return "message_remote_error_code";
}

const char* InvalidPayloadJsonMessage()
{
    return "invalid message payload json";
}

const char* PrivateStubNotConfiguredMessage()
{
    return "private message grpc stub is not configured";
}

const char* GroupStubNotConfiguredMessage()
{
    return "group message grpc stub is not configured";
}

const char* PrivateBusinessErrorMessage()
{
    return "private message grpc business error";
}

const char* GroupBusinessErrorMessage()
{
    return "group message grpc business error";
}

bool ShouldMergePayload(bool payload_empty)
{
    return !payload_empty;
}

bool ShouldReportInvalidPayload(bool parse_ok, bool is_object)
{
    return !parse_ok || !is_object;
}

bool ShouldReportMissingStub(bool has_stub)
{
    return !has_stub;
}

bool ShouldReportStatusError(bool status_ok)
{
    return !status_ok;
}

bool ShouldReportBusinessError(int response_error, int success_code)
{
    return response_error != success_code;
}
} // namespace memochat::chat::message_grpc_client::modules
