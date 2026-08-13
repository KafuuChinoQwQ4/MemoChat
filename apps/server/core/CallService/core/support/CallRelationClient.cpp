#include "CallRelationClient.hpp"

#include "auth/RelationGrpcAuth.hpp"
#include "common/proto/chat_internal.grpc.pb.h"
#include "logging/GrpcTrace.hpp"
#include "logging/Logger.hpp"

#include <chrono>
#include <memory>
#include <utility>

#include <grpcpp/grpcpp.h>

namespace memochat::gate::services::call
{
CallRelationClient::CallRelationClient(std::string endpoint, std::string auth_token, std::chrono::milliseconds timeout)
    : endpoint_(std::move(endpoint))
    , auth_token_(std::move(auth_token))
    , timeout_(timeout)
{
}

bool CallRelationClient::AreUsersFriends(int uid, int peer_uid) const
{
    if (endpoint_.empty() || !memochat::auth::IsStrongRelationGrpcAuthToken(auth_token_) || uid <= 0 || peer_uid <= 0 ||
        uid == peer_uid)
    {
        return false;
    }

    chatinternal::FriendshipRequest request;
    request.set_uid(uid);
    request.set_peer_uid(peer_uid);

    auto channel = grpc::CreateChannel(endpoint_, grpc::InsecureChannelCredentials());
    auto stub = chatinternal::ChatRelationInternalService::NewStub(channel);
    grpc::ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    memochat::auth::InjectRelationGrpcAuth(context, auth_token_);
    context.set_deadline(std::chrono::system_clock::now() + timeout_);

    chatinternal::FriendshipResponse response;
    const grpc::Status status = stub->CheckFriendship(&context, request, &response);
    if (!status.ok())
    {
        memolog::LogWarn("gate.call.relation.rpc_failed",
                         "CheckFriendship RPC failed",
                         {{"endpoint", endpoint_}, {"error", status.error_message()}});
        return false;
    }
    return response.are_friends();
}

GateReadinessProbe
CallRelationReadinessProbe(std::string endpoint, std::string auth_token, std::chrono::milliseconds timeout)
{
    auto channel = endpoint.empty() ? std::shared_ptr<grpc::Channel>{}
                                    : grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
    return GateReadinessProbe{
        .name = "RelationQuery",
        .check =
            [endpoint = std::move(endpoint), auth_token = std::move(auth_token), channel = std::move(channel), timeout](
                std::string* error) -> bool
        {
            if (!channel)
            {
                if (error != nullptr)
                {
                    *error = "relation query endpoint is empty";
                }
                return false;
            }
            if (!memochat::auth::IsStrongRelationGrpcAuthToken(auth_token))
            {
                if (error != nullptr)
                {
                    *error = "relation query auth token is invalid";
                }
                return false;
            }

            auto stub = chatinternal::ChatRelationInternalService::NewStub(channel);
            chatinternal::RelationHealthRequest request;
            chatinternal::RelationHealthResponse response;
            grpc::ClientContext context;
            memolog::InjectGrpcTraceMetadata(context);
            memochat::auth::InjectRelationGrpcAuth(context, auth_token);
            context.set_deadline(std::chrono::system_clock::now() + timeout);
            const auto status = stub->CheckHealth(&context, request, &response);
            if (status.ok() && response.ready())
            {
                return true;
            }
            if (error != nullptr)
            {
                *error = "relation query readiness failed at " + endpoint + ": " + status.error_message();
            }
            return false;
        },
    };
}

} // namespace memochat::gate::services::call
