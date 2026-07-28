#include "clients/MomentsRelationClient.hpp"

#include "auth/RelationGrpcAuth.hpp"
#include "common/proto/chat_internal.grpc.pb.h"
#include "json/GlazeCompat.hpp"
#include "logging/Logger.hpp"
#include "logging/GrpcTrace.hpp"

#include <chrono>
#include <memory>
#include <utility>

#include <grpcpp/grpcpp.h>

namespace memochat::gate::services::moments
{

GateReadinessProbe
MomentsRelationReadinessProbe(std::string endpoint, std::string auth_token, std::chrono::milliseconds timeout)
{
    auto channel = endpoint.empty() ? std::shared_ptr<grpc::Channel>{}
                                    : grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
    return GateReadinessProbe{
        .name = "RelationQuery",
        .check =
            [endpoint = std::move(endpoint), auth_token = std::move(auth_token), channel = std::move(channel), timeout](
                std::string* error) -> bool
        {
            if (!channel || !memochat::auth::IsStrongRelationGrpcAuthToken(auth_token))
            {
                if (error != nullptr)
                {
                    *error = channel ? "relation query auth token is invalid" : "relation query endpoint is empty";
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
                *error = "relation query readiness failed at " + endpoint;
            }
            return false;
        },
    };
}

MomentsRelationClient::MomentsRelationClient(std::string endpoint, std::string auth_token)
    : _endpoint(std::move(endpoint))
    , _auth_token(std::move(auth_token))
{
}

std::vector<int> MomentsRelationClient::FilterFriendUids(int viewer_uid, const std::vector<int>& author_uids)
{
    std::vector<int> result;
    if (_endpoint.empty() || !memochat::auth::IsStrongRelationGrpcAuthToken(_auth_token) || viewer_uid <= 0 ||
        author_uids.empty())
    {
        return result;
    }

    // Build request payload: {"viewer_uid":N,"author_uids":[...]}.
    Json::Value payload(Json::objectValue);
    payload["viewer_uid"] = viewer_uid;
    Json::Value uids(Json::arrayValue);
    for (int uid : author_uids)
    {
        uids.append(uid);
    }
    payload["author_uids"] = uids;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";

    chatinternal::JsonPayloadRequest grpc_request;
    grpc_request.set_payload_json(Json::writeString(writer, payload));
    grpc_request.mutable_session()->set_uid(viewer_uid);

    auto channel = grpc::CreateChannel(_endpoint, grpc::InsecureChannelCredentials());
    auto stub = chatinternal::ChatRelationInternalService::NewStub(channel);

    grpc::ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    memochat::auth::InjectRelationGrpcAuth(context, _auth_token);
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(2));
    chatinternal::JsonPayloadResponse response;
    const grpc::Status status = stub->FilterFriendUids(&context, grpc_request, &response);
    if (!status.ok())
    {
        memolog::LogWarn("gate.moments.relation.rpc_failed",
                         "FilterFriendUids RPC failed",
                         {{"endpoint", _endpoint}, {"error", status.error_message()}});
        return result;
    }

    Json::CharReaderBuilder reader_builder;
    Json::Value root;
    std::string errors;
    const std::string& body = response.payload_json();
    std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
    if (!reader || !reader->parse(body.data(), body.data() + body.size(), &root, &errors))
    {
        memolog::LogWarn("gate.moments.relation.parse_failed", "FilterFriendUids parse failed", {{"error", errors}});
        return result;
    }
    if (root.isMember("friend_uids") && root["friend_uids"].isArray())
    {
        for (const auto& item : root["friend_uids"])
        {
            if (item.isNumber())
            {
                result.push_back(item.asInt());
            }
        }
    }
    return result;
}

} // namespace memochat::gate::services::moments
