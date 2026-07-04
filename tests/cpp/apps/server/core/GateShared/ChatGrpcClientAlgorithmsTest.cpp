#include <gtest/gtest.h>

namespace memochat::tests::gate::chat_grpc_client
{
int DefaultConnectionPoolSize();
const char* NotifyCallEventSpanName();
const char* ClientSpanKind();
const char* RpcSystemAttribute();
const char* RpcSystemValue();
const char* RpcServiceAttribute();
const char* ChatServiceName();
const char* RpcMethodAttribute();
const char* NotifyGroupEventMethod();
const char* PeerServiceAttribute();
bool ShouldWakeConnectionWait(bool stop_requested, bool has_connection);
bool ShouldReturnNullConnection(bool stop_requested);
bool ShouldAcceptReturnedConnection(bool stop_requested);
bool ShouldReportMissingServer(bool pool_found);
bool ShouldReportUnavailableStub(bool has_stub);
bool ShouldReportFailedStatus(bool status_ok);
} // namespace memochat::tests::gate::chat_grpc_client

TEST(ChatGrpcClientAlgorithmsTest, ExposesStablePoolAndSpanMetadata)
{
    using namespace memochat::tests::gate::chat_grpc_client;

    EXPECT_EQ(DefaultConnectionPoolSize(), 5);
    EXPECT_STREQ(NotifyCallEventSpanName(), "ChatService.NotifyCallEvent");
    EXPECT_STREQ(ClientSpanKind(), "CLIENT");
    EXPECT_STREQ(RpcSystemAttribute(), "rpc.system");
    EXPECT_STREQ(RpcSystemValue(), "grpc");
    EXPECT_STREQ(RpcServiceAttribute(), "rpc.service");
    EXPECT_STREQ(ChatServiceName(), "ChatService");
    EXPECT_STREQ(RpcMethodAttribute(), "rpc.method");
    EXPECT_STREQ(NotifyGroupEventMethod(), "NotifyGroupEvent");
    EXPECT_STREQ(PeerServiceAttribute(), "peer_service");
}

TEST(ChatGrpcClientAlgorithmsTest, ClassifiesConnectionPoolWaitAndReturnGuards)
{
    using namespace memochat::tests::gate::chat_grpc_client;

    EXPECT_FALSE(ShouldWakeConnectionWait(false, false));
    EXPECT_TRUE(ShouldWakeConnectionWait(false, true));
    EXPECT_TRUE(ShouldWakeConnectionWait(true, false));
    EXPECT_TRUE(ShouldWakeConnectionWait(true, true));

    EXPECT_FALSE(ShouldReturnNullConnection(false));
    EXPECT_TRUE(ShouldReturnNullConnection(true));
    EXPECT_TRUE(ShouldAcceptReturnedConnection(false));
    EXPECT_FALSE(ShouldAcceptReturnedConnection(true));
}

TEST(ChatGrpcClientAlgorithmsTest, ClassifiesRpcFailureGuards)
{
    using namespace memochat::tests::gate::chat_grpc_client;

    EXPECT_FALSE(ShouldReportMissingServer(true));
    EXPECT_TRUE(ShouldReportMissingServer(false));
    EXPECT_FALSE(ShouldReportUnavailableStub(true));
    EXPECT_TRUE(ShouldReportUnavailableStub(false));
    EXPECT_FALSE(ShouldReportFailedStatus(true));
    EXPECT_TRUE(ShouldReportFailedStatus(false));
}
