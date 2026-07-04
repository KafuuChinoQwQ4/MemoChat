#include <gtest/gtest.h>

namespace memochat::tests::chat::message_internal_grpc_service
{
const char* DefaultPayloadJson();
const char* MissingGroupListBootstrapRequestMessage();
const char* GroupMessageCommandServiceNotConfiguredMessage();
const char* MissingPrivateMessageCommandRequestMessage();
const char* PrivateMessageCommandServiceNotConfiguredMessage();
const char* MissingGroupMessageCommandRequestMessage();
bool ShouldReportMissingRequestOrResponse(bool has_request, bool has_response);
bool ShouldReportMissingPrivateService(bool has_service);
bool ShouldReportMissingGroupService(bool has_service);
short TcpMessageId(int value);
} // namespace memochat::tests::chat::message_internal_grpc_service

TEST(ChatMessageInternalGrpcServiceAlgorithmsTest, KeepsStableGrpcErrorMessages)
{
    using namespace memochat::tests::chat::message_internal_grpc_service;

    EXPECT_STREQ(DefaultPayloadJson(), "{}");
    EXPECT_STREQ(MissingGroupListBootstrapRequestMessage(), "missing group list bootstrap request or response");
    EXPECT_STREQ(GroupMessageCommandServiceNotConfiguredMessage(), "group message command service is not configured");
    EXPECT_STREQ(MissingPrivateMessageCommandRequestMessage(), "missing private message command request or response");
    EXPECT_STREQ(PrivateMessageCommandServiceNotConfiguredMessage(),
                 "private message command service is not configured");
    EXPECT_STREQ(MissingGroupMessageCommandRequestMessage(), "missing group message command request or response");
}

TEST(ChatMessageInternalGrpcServiceAlgorithmsTest, ClassifiesMissingRequestOrResponse)
{
    using namespace memochat::tests::chat::message_internal_grpc_service;

    EXPECT_FALSE(ShouldReportMissingRequestOrResponse(true, true));
    EXPECT_TRUE(ShouldReportMissingRequestOrResponse(false, true));
    EXPECT_TRUE(ShouldReportMissingRequestOrResponse(true, false));
    EXPECT_TRUE(ShouldReportMissingRequestOrResponse(false, false));
}

TEST(ChatMessageInternalGrpcServiceAlgorithmsTest, ClassifiesMissingServices)
{
    using namespace memochat::tests::chat::message_internal_grpc_service;

    EXPECT_FALSE(ShouldReportMissingPrivateService(true));
    EXPECT_TRUE(ShouldReportMissingPrivateService(false));

    EXPECT_FALSE(ShouldReportMissingGroupService(true));
    EXPECT_TRUE(ShouldReportMissingGroupService(false));
}

TEST(ChatMessageInternalGrpcServiceAlgorithmsTest, ConvertsTcpMessageIdToWireShort)
{
    using namespace memochat::tests::chat::message_internal_grpc_service;

    EXPECT_EQ(TcpMessageId(0), 0);
    EXPECT_EQ(TcpMessageId(42), 42);
    EXPECT_EQ(TcpMessageId(32767), 32767);
}
