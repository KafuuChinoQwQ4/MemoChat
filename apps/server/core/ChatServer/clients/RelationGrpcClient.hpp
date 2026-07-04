#pragma once

#include "common/proto/chat_internal.grpc.pb.h"
#include "ports/IRelationCommandService.hpp"
#include "ports/IRelationQueryService.hpp"

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

class RelationGrpcClient final
    : public IRelationQueryService
    , public IRelationCommandService
{
public:
    enum class QueryRpc
    {
        AppendRelationBootstrap,
        BuildDialogList,
    };

    enum class CommandRpc
    {
        SearchUser,
        AddFriendApply,
        AuthFriendApply,
        DeleteFriend,
        GetDialogList,
        SyncDraft,
        PinDialog,
        FilterFriendUids,
    };

    explicit RelationGrpcClient(const std::string& endpoint,
                                std::chrono::milliseconds timeout = std::chrono::seconds(2));
    explicit RelationGrpcClient(std::shared_ptr<grpc::Channel> channel,
                                std::chrono::milliseconds timeout = std::chrono::seconds(2));

    void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override;
    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override;

    RelationCommandResult SearchUser(const RelationCommandRequest& request) override;
    RelationCommandResult AddFriendApply(const RelationCommandRequest& request) override;
    RelationCommandResult AuthFriendApply(const RelationCommandRequest& request) override;
    RelationCommandResult DeleteFriend(const RelationCommandRequest& request) override;
    RelationCommandResult GetDialogList(const RelationCommandRequest& request) override;
    RelationCommandResult SyncDraft(const RelationCommandRequest& request) override;
    RelationCommandResult PinDialog(const RelationCommandRequest& request) override;
    RelationCommandResult FilterFriendUids(const RelationCommandRequest& request) override;

private:
    void CallQuery(QueryRpc rpc, int uid, memochat::json::JsonValue& out);
    RelationCommandResult CallCommand(CommandRpc rpc, const RelationCommandRequest& request);

    std::unique_ptr<chatinternal::ChatRelationInternalService::Stub> _stub;
    std::chrono::milliseconds _timeout;
};
