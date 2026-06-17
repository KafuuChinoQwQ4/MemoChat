#include "clients/MomentsRelationClient.h"

#include "common/proto/chat_internal.grpc.pb.h"
#include "json/GlazeCompat.h"
#include "logging/Logger.h"

#include <chrono>
#include <memory>

#include <grpcpp/grpcpp.h>

namespace memochat::gate::services::moments
{

MomentsRelationClient::MomentsRelationClient(std::string endpoint)
    : _endpoint(std::move(endpoint))
{
}

std::vector<int> MomentsRelationClient::FilterFriendUids(int viewer_uid, const std::vector<int>& author_uids)
{
    std::vector<int> result;
    if (_endpoint.empty() || viewer_uid <= 0 || author_uids.empty())
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

    try
    {
        auto channel = grpc::CreateChannel(_endpoint, grpc::InsecureChannelCredentials());
        auto stub = chatinternal::ChatRelationInternalService::NewStub(channel);

        grpc::ClientContext context;
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
        if (!reader->parse(body.data(), body.data() + body.size(), &root, &errors))
        {
            memolog::LogWarn("gate.moments.relation.parse_failed", "FilterFriendUids parse failed", {{"error", errors}});
            return result;
        }
        if (root.isMember("friend_uids") && root["friend_uids"].isArray())
        {
            for (const auto& item : root["friend_uids"])
            {
                result.push_back(item.asInt());
            }
        }
    }
    catch (const std::exception& e)
    {
        memolog::LogWarn("gate.moments.relation.exception", "FilterFriendUids exception", {{"error", e.what()}});
        result.clear();
    }
    return result;
}

} // namespace memochat::gate::services::moments
