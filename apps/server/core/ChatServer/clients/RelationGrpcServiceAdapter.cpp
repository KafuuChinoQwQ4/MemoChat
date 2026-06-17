#include "RelationGrpcServiceAdapter.h"

#include <utility>

RelationGrpcServiceAdapter::RelationGrpcServiceAdapter(const std::string& endpoint, std::chrono::milliseconds timeout)
    : _client(endpoint, timeout)
{
}

RelationGrpcServiceAdapter::RelationGrpcServiceAdapter(std::shared_ptr<grpc::Channel> channel,
                                                       std::chrono::milliseconds timeout)
    : _client(std::move(channel), timeout)
{
}

void RelationGrpcServiceAdapter::AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out)
{
    _client.AppendRelationBootstrapJson(uid, out);
}

void RelationGrpcServiceAdapter::BuildDialogListJson(int uid, memochat::json::JsonValue& out)
{
    _client.BuildDialogListJson(uid, out);
}

RelationCommandResult RelationGrpcServiceAdapter::SearchUser(const RelationCommandRequest& request)
{
    return _client.SearchUser(request);
}

RelationCommandResult RelationGrpcServiceAdapter::AddFriendApply(const RelationCommandRequest& request)
{
    return _client.AddFriendApply(request);
}

RelationCommandResult RelationGrpcServiceAdapter::AuthFriendApply(const RelationCommandRequest& request)
{
    return _client.AuthFriendApply(request);
}

RelationCommandResult RelationGrpcServiceAdapter::DeleteFriend(const RelationCommandRequest& request)
{
    return _client.DeleteFriend(request);
}

RelationCommandResult RelationGrpcServiceAdapter::GetDialogList(const RelationCommandRequest& request)
{
    return _client.GetDialogList(request);
}

RelationCommandResult RelationGrpcServiceAdapter::SyncDraft(const RelationCommandRequest& request)
{
    return _client.SyncDraft(request);
}

RelationCommandResult RelationGrpcServiceAdapter::PinDialog(const RelationCommandRequest& request)
{
    return _client.PinDialog(request);
}

RelationCommandResult RelationGrpcServiceAdapter::FilterFriendUids(const RelationCommandRequest& request)
{
    return _client.FilterFriendUids(request);
}
