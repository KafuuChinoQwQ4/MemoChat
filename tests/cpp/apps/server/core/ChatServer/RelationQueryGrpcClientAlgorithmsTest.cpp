#include <gtest/gtest.h>

namespace memochat::tests::chat::relation_query_grpc_client
{
const char* UnknownMethod();
const char* AppendRelationBootstrapMethod();
const char* BuildDialogListMethod();
const char* QueryMethodName(int rpc);
const char* ErrorField();
const char* RemoteMethodField();
const char* RemoteErrorField();
const char* RemoteStatusCodeField();
const char* RemoteErrorCodeField();
const char* InvalidPayloadJsonMessage();
const char* StubNotConfiguredMessage();
bool ShouldMergePayload(bool payload_empty);
bool ShouldReportInvalidPayload(bool parse_ok, bool is_object);
bool ShouldReportMissingStub(bool has_stub);
bool ShouldReportStatusError(bool status_ok);
bool ShouldReportBusinessError(int response_error, int success_code);
} // namespace memochat::tests::chat::relation_query_grpc_client

TEST(RelationQueryGrpcClientAlgorithmsTest, SelectsStableRpcMethodNames)
{
    using namespace memochat::tests::chat::relation_query_grpc_client;

    EXPECT_STREQ(UnknownMethod(), "unknown");
    EXPECT_STREQ(AppendRelationBootstrapMethod(), "AppendRelationBootstrap");
    EXPECT_STREQ(BuildDialogListMethod(), "BuildDialogList");
    EXPECT_STREQ(QueryMethodName(0), "AppendRelationBootstrap");
    EXPECT_STREQ(QueryMethodName(1), "BuildDialogList");
    EXPECT_STREQ(QueryMethodName(2), "unknown");
}

TEST(RelationQueryGrpcClientAlgorithmsTest, KeepsRemoteErrorFieldNames)
{
    using namespace memochat::tests::chat::relation_query_grpc_client;

    EXPECT_STREQ(ErrorField(), "error");
    EXPECT_STREQ(RemoteMethodField(), "relation_query_remote_method");
    EXPECT_STREQ(RemoteErrorField(), "relation_query_remote_error");
    EXPECT_STREQ(RemoteStatusCodeField(), "relation_query_remote_status_code");
    EXPECT_STREQ(RemoteErrorCodeField(), "relation_query_remote_error_code");
}

TEST(RelationQueryGrpcClientAlgorithmsTest, KeepsRemoteErrorMessages)
{
    using namespace memochat::tests::chat::relation_query_grpc_client;

    EXPECT_STREQ(InvalidPayloadJsonMessage(), "invalid relation query payload json");
    EXPECT_STREQ(StubNotConfiguredMessage(), "relation query grpc stub is not configured");
}

TEST(RelationQueryGrpcClientAlgorithmsTest, ClassifiesPayloadGuards)
{
    using namespace memochat::tests::chat::relation_query_grpc_client;

    EXPECT_FALSE(ShouldMergePayload(true));
    EXPECT_TRUE(ShouldMergePayload(false));

    EXPECT_FALSE(ShouldReportInvalidPayload(true, true));
    EXPECT_TRUE(ShouldReportInvalidPayload(false, true));
    EXPECT_TRUE(ShouldReportInvalidPayload(true, false));
    EXPECT_TRUE(ShouldReportInvalidPayload(false, false));
}

TEST(RelationQueryGrpcClientAlgorithmsTest, ClassifiesGrpcAndBusinessErrorGuards)
{
    using namespace memochat::tests::chat::relation_query_grpc_client;

    EXPECT_FALSE(ShouldReportMissingStub(true));
    EXPECT_TRUE(ShouldReportMissingStub(false));

    EXPECT_FALSE(ShouldReportStatusError(true));
    EXPECT_TRUE(ShouldReportStatusError(false));

    EXPECT_FALSE(ShouldReportBusinessError(0, 0));
    EXPECT_TRUE(ShouldReportBusinessError(1, 0));
}
