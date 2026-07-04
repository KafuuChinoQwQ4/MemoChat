export module memochat.chat.relation_grpc_client_algorithms;

export namespace memochat::chat::relation_grpc_client::modules
{
const char* UnknownMethod()
{
    return "unknown";
}

const char* AppendRelationBootstrapMethod()
{
    return "AppendRelationBootstrap";
}

const char* BuildDialogListMethod()
{
    return "BuildDialogList";
}

const char* QueryMethodName(int rpc)
{
    switch (rpc)
    {
        case 0:
            return AppendRelationBootstrapMethod();
        case 1:
            return BuildDialogListMethod();
    }
    return UnknownMethod();
}

const char* SearchUserMethod()
{
    return "SearchUser";
}

const char* AddFriendApplyMethod()
{
    return "AddFriendApply";
}

const char* AuthFriendApplyMethod()
{
    return "AuthFriendApply";
}

const char* DeleteFriendMethod()
{
    return "DeleteFriend";
}

const char* GetDialogListMethod()
{
    return "GetDialogList";
}

const char* SyncDraftMethod()
{
    return "SyncDraft";
}

const char* PinDialogMethod()
{
    return "PinDialog";
}

const char* FilterFriendUidsMethod()
{
    return "FilterFriendUids";
}

const char* CommandMethodName(int rpc)
{
    switch (rpc)
    {
        case 0:
            return SearchUserMethod();
        case 1:
            return AddFriendApplyMethod();
        case 2:
            return AuthFriendApplyMethod();
        case 3:
            return DeleteFriendMethod();
        case 4:
            return GetDialogListMethod();
        case 5:
            return SyncDraftMethod();
        case 6:
            return PinDialogMethod();
        case 7:
            return FilterFriendUidsMethod();
    }
    return UnknownMethod();
}

const char* ErrorField()
{
    return "error";
}

const char* RemoteMethodField()
{
    return "relation_remote_method";
}

const char* RemoteErrorField()
{
    return "relation_remote_error";
}

const char* RemoteStatusCodeField()
{
    return "relation_remote_status_code";
}

const char* RemoteErrorCodeField()
{
    return "relation_remote_error_code";
}

const char* InvalidPayloadJsonMessage()
{
    return "invalid relation payload json";
}

const char* StubNotConfiguredMessage()
{
    return "relation grpc stub is not configured";
}

const char* BusinessErrorMessage()
{
    return "relation grpc business error";
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
} // namespace memochat::chat::relation_grpc_client::modules
