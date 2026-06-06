#pragma once

#include "common/proto/chat_internal.grpc.pb.h"
#include "ports/IRelationQueryService.h"

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

class RelationQueryGrpcClient final : public IRelationQueryService
{
public:
    explicit RelationQueryGrpcClient(const std::string& endpoint,
                                     std::chrono::milliseconds timeout = std::chrono::seconds(2));
    explicit RelationQueryGrpcClient(std::shared_ptr<grpc::Channel> channel,
                                     std::chrono::milliseconds timeout = std::chrono::seconds(2));

    void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override;
    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override;

private:
    enum class QueryRpc
    {
        AppendRelationBootstrap,
        BuildDialogList,
    };

    void Call(QueryRpc rpc, int uid, memochat::json::JsonValue& out);

    std::unique_ptr<chatinternal::ChatRelationInternalService::Stub> _stub;
    std::chrono::milliseconds _timeout;
};
