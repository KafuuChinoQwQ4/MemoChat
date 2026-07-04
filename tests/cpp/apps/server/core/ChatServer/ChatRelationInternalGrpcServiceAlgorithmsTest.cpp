#include <gtest/gtest.h>

namespace memochat::tests::chat::relation_internal_grpc_service
{
const char* DefaultPayloadJson();
const char* MissingBootstrapRequestMessage();
const char* RelationServiceNotConfiguredMessage();
const char* UidMustBePositiveMessage();
const char* MissingRelationCommandRequestMessage();
bool ShouldReportMissingRequestOrResponse(bool has_request, bool has_response);
bool ShouldReportMissingRelationService(bool has_service);
bool ShouldReportInvalidUid(int uid);
short TcpMessageId(int value);
} // namespace memochat::tests::chat::relation_internal_grpc_service

TEST(ChatRelationInternalGrpcServiceAlgorithmsTest, KeepsStableGrpcErrorMessages)
{
    using namespace memochat::tests::chat::relation_internal_grpc_service;

    EXPECT_STREQ(DefaultPayloadJson(), "{}");
    EXPECT_STREQ(MissingBootstrapRequestMessage(), "missing bootstrap request or response");
    EXPECT_STREQ(RelationServiceNotConfiguredMessage(), "relation service is not configured");
    EXPECT_STREQ(UidMustBePositiveMessage(), "uid must be positive");
    EXPECT_STREQ(MissingRelationCommandRequestMessage(), "missing relation command request or response");
}

TEST(ChatRelationInternalGrpcServiceAlgorithmsTest, ClassifiesMissingRequestOrResponse)
{
    using namespace memochat::tests::chat::relation_internal_grpc_service;

    EXPECT_FALSE(ShouldReportMissingRequestOrResponse(true, true));
    EXPECT_TRUE(ShouldReportMissingRequestOrResponse(false, true));
    EXPECT_TRUE(ShouldReportMissingRequestOrResponse(true, false));
    EXPECT_TRUE(ShouldReportMissingRequestOrResponse(false, false));
}

TEST(ChatRelationInternalGrpcServiceAlgorithmsTest, ClassifiesMissingServiceAndInvalidUid)
{
    using namespace memochat::tests::chat::relation_internal_grpc_service;

    EXPECT_FALSE(ShouldReportMissingRelationService(true));
    EXPECT_TRUE(ShouldReportMissingRelationService(false));

    EXPECT_TRUE(ShouldReportInvalidUid(-1));
    EXPECT_TRUE(ShouldReportInvalidUid(0));
    EXPECT_FALSE(ShouldReportInvalidUid(1));
}

TEST(ChatRelationInternalGrpcServiceAlgorithmsTest, ConvertsTcpMessageIdToWireShort)
{
    using namespace memochat::tests::chat::relation_internal_grpc_service;

    EXPECT_EQ(TcpMessageId(0), 0);
    EXPECT_EQ(TcpMessageId(42), 42);
    EXPECT_EQ(TcpMessageId(32767), 32767);
}
