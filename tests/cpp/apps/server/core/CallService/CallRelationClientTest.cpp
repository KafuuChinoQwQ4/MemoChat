#include <gtest/gtest.h>

#include "CallRelationClient.hpp"
#include "auth/RelationGrpcAuth.hpp"
#include "common/proto/chat_internal.grpc.pb.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

namespace
{
std::string TestCallAuthToken()
{
    return std::string(32, 'a');
}

class FriendshipGrpcService final : public chatinternal::ChatRelationInternalService::Service
{
public:
    grpc::Status CheckHealth(grpc::ServerContext* context,
                             const chatinternal::RelationHealthRequest*,
                             chatinternal::RelationHealthResponse* response) override
    {
        if (context == nullptr || !memochat::auth::HasValidRelationGrpcAuth(*context, expected_auth_token))
        {
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "bad test auth token");
        }
        if (force_failure)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "relation unavailable");
        }
        response->set_ready(true);
        return grpc::Status::OK;
    }

    grpc::Status CheckFriendship(grpc::ServerContext* context,
                                 const chatinternal::FriendshipRequest* request,
                                 chatinternal::FriendshipResponse* response) override
    {
        call_count.fetch_add(1, std::memory_order_relaxed);
        saw_request_id.store(context != nullptr && context->client_metadata().contains("x-request-id"),
                             std::memory_order_relaxed);
        if (context == nullptr || !memochat::auth::HasValidRelationGrpcAuth(*context, expected_auth_token))
        {
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "bad test auth token");
        }
        if (delay.count() > 0)
        {
            std::this_thread::sleep_for(delay);
        }
        if (force_failure)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "relation unavailable");
        }
        response->set_are_friends(request != nullptr && request->uid() == friend_uid &&
                                  request->peer_uid() == friend_peer_uid);
        return grpc::Status::OK;
    }

    int friend_uid = 42;
    int friend_peer_uid = 84;
    bool force_failure = false;
    std::chrono::milliseconds delay{0};
    std::atomic<int> call_count{0};
    std::atomic<bool> saw_request_id{false};
    std::string expected_auth_token = TestCallAuthToken();
};

struct RunningGrpcServer
{
    int port = 0;
    std::unique_ptr<grpc::Server> server;

    std::string Endpoint() const
    {
        return std::string("127.0.0.1:") + std::to_string(port);
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

TEST(CallRelationClientTest, AuthorizesOnlyTypedCurrentFriendResponse)
{
    FriendshipGrpcService service;
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);

    memochat::gate::services::call::CallRelationClient client(running.Endpoint(),
                                                              TestCallAuthToken(),
                                                              std::chrono::milliseconds(500));

    EXPECT_TRUE(client.AreUsersFriends(42, 84));
    EXPECT_FALSE(client.AreUsersFriends(42, 85));
    EXPECT_TRUE(service.saw_request_id.load(std::memory_order_relaxed));
    running.server->Shutdown();
}

TEST(CallRelationClientTest, InvalidIdentityIsRejectedWithoutRpc)
{
    FriendshipGrpcService service;
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);

    memochat::gate::services::call::CallRelationClient client(running.Endpoint(),
                                                              TestCallAuthToken(),
                                                              std::chrono::milliseconds(500));

    EXPECT_FALSE(client.AreUsersFriends(0, 84));
    EXPECT_FALSE(client.AreUsersFriends(42, 42));
    EXPECT_EQ(service.call_count.load(std::memory_order_relaxed), 0);
    running.server->Shutdown();
}

TEST(CallRelationClientTest, RpcFailureAndDeadlineFailClosed)
{
    FriendshipGrpcService service;
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);

    service.force_failure = true;
    memochat::gate::services::call::CallRelationClient failure_client(running.Endpoint(),
                                                                      TestCallAuthToken(),
                                                                      std::chrono::milliseconds(500));
    EXPECT_FALSE(failure_client.AreUsersFriends(42, 84));

    service.force_failure = false;
    service.delay = std::chrono::milliseconds(100);
    memochat::gate::services::call::CallRelationClient timeout_client(running.Endpoint(),
                                                                      TestCallAuthToken(),
                                                                      std::chrono::milliseconds(10));
    EXPECT_FALSE(timeout_client.AreUsersFriends(42, 84));
    running.server->Shutdown();
}

TEST(CallRelationClientTest, ReadinessRequiresConfiguredReachableEndpoint)
{
    std::string error;
    auto missing = memochat::gate::services::call::CallRelationReadinessProbe("", TestCallAuthToken());
    EXPECT_FALSE(missing.check(&error));
    EXPECT_NE(error.find("empty"), std::string::npos);

    FriendshipGrpcService service;
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);
    auto reachable = memochat::gate::services::call::CallRelationReadinessProbe(running.Endpoint(),
                                                                                TestCallAuthToken(),
                                                                                std::chrono::milliseconds(500));
    error.clear();
    EXPECT_TRUE(reachable.check(&error)) << error;

    auto wrong_token = memochat::gate::services::call::CallRelationReadinessProbe(running.Endpoint(),
                                                                                  std::string(32, 'x'),
                                                                                  std::chrono::milliseconds(500));
    error.clear();
    EXPECT_FALSE(wrong_token.check(&error));
    EXPECT_NE(error.find("bad test auth token"), std::string::npos);
    running.server->Shutdown();
}
