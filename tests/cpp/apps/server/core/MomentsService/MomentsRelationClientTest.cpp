#include <gtest/gtest.h>

#include "auth/RelationGrpcAuth.hpp"
#include "clients/MomentsRelationClient.hpp"
#include "common/proto/chat_internal.grpc.pb.h"

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

namespace
{
std::string TestMomentsAuthToken()
{
    return std::string(32, 'm');
}

class MomentsRelationGrpcService final : public chatinternal::ChatRelationInternalService::Service
{
public:
    grpc::Status CheckHealth(grpc::ServerContext* context,
                             const chatinternal::RelationHealthRequest*,
                             chatinternal::RelationHealthResponse* response) override
    {
        if (context == nullptr || !memochat::auth::HasValidRelationGrpcAuth(*context, TestMomentsAuthToken()))
        {
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "bad moments test auth token");
        }
        response->set_ready(true);
        return grpc::Status::OK;
    }

    grpc::Status FilterFriendUids(grpc::ServerContext* context,
                                  const chatinternal::JsonPayloadRequest*,
                                  chatinternal::JsonPayloadResponse* response) override
    {
        if (context == nullptr || !memochat::auth::HasValidRelationGrpcAuth(*context, TestMomentsAuthToken()))
        {
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "bad moments test auth token");
        }
        response->set_payload_json(R"({"error":0,"friend_uids":[84]})");
        return grpc::Status::OK;
    }
};

struct RunningGrpcServer
{
    int port = 0;
    std::unique_ptr<grpc::Server> server;

    std::string Endpoint() const
    {
        return "127.0.0.1:" + std::to_string(port);
    }
};

RunningGrpcServer StartServer(chatinternal::ChatRelationInternalService::Service* service)
{
    RunningGrpcServer running;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &running.port);
    builder.RegisterService(service);
    running.server = builder.BuildAndStart();
    return running;
}
} // namespace

TEST(MomentsRelationClientTest, SendsMomentsRoleTokenAndParsesFriendUids)
{
    MomentsRelationGrpcService service;
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);

    memochat::gate::services::moments::MomentsRelationClient client(running.Endpoint(), TestMomentsAuthToken());
    EXPECT_EQ(client.FilterFriendUids(42, {84}), std::vector<int>({84}));
    running.server->Shutdown();
}

TEST(MomentsRelationClientTest, WrongTokenFailsClosed)
{
    MomentsRelationGrpcService service;
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);

    memochat::gate::services::moments::MomentsRelationClient client(running.Endpoint(), std::string(32, 'x'));
    EXPECT_TRUE(client.FilterFriendUids(42, {84}).empty());
    running.server->Shutdown();
}
