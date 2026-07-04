#include <gtest/gtest.h>

namespace memochat::tests::chat::relation_grpc_client
{
const char* UnknownMethod();
const char* QueryMethodName(int rpc);
const char* CommandMethodName(int rpc);
const char* ErrorField();
const char* RemoteMethodField();
const char* RemoteErrorField();
const char* RemoteStatusCodeField();
const char* RemoteErrorCodeField();
const char* InvalidPayloadJsonMessage();
const char* StubNotConfiguredMessage();
const char* BusinessErrorMessage();
bool ShouldMergePayload(bool payload_empty);
bool ShouldReportInvalidPayload(bool parse_ok, bool is_object);
bool ShouldReportMissingStub(bool has_stub);
bool ShouldReportStatusError(bool status_ok);
bool ShouldReportBusinessError(int response_error, int success_code);
} // namespace memochat::tests::chat::relation_grpc_client

TEST(RelationGrpcClientAlgorithmsTest, SelectsStableQueryMethodNames)
{
    using namespace memochat::tests::chat::relation_grpc_client;

    EXPECT_STREQ(UnknownMethod(), "unknown");
    EXPECT_STREQ(QueryMethodName(0), "AppendRelationBootstrap");
    EXPECT_STREQ(QueryMethodName(1), "BuildDialogList");
    EXPECT_STREQ(QueryMethodName(2), "unknown");
}

TEST(RelationGrpcClientAlgorithmsTest, SelectsStableCommandMethodNames)
{
    using namespace memochat::tests::chat::relation_grpc_client;

    EXPECT_STREQ(CommandMethodName(0), "SearchUser");
    EXPECT_STREQ(CommandMethodName(1), "AddFriendApply");
    EXPECT_STREQ(CommandMethodName(2), "AuthFriendApply");
    EXPECT_STREQ(CommandMethodName(3), "DeleteFriend");
    EXPECT_STREQ(CommandMethodName(4), "GetDialogList");
    EXPECT_STREQ(CommandMethodName(5), "SyncDraft");
    EXPECT_STREQ(CommandMethodName(6), "PinDialog");
    EXPECT_STREQ(CommandMethodName(7), "FilterFriendUids");
    EXPECT_STREQ(CommandMethodName(8), "unknown");
}

TEST(RelationGrpcClientAlgorithmsTest, KeepsRemoteErrorFieldNamesAndMessages)
{
    using namespace memochat::tests::chat::relation_grpc_client;

    EXPECT_STREQ(ErrorField(), "error");
    EXPECT_STREQ(RemoteMethodField(), "relation_remote_method");
    EXPECT_STREQ(RemoteErrorField(), "relation_remote_error");
    EXPECT_STREQ(RemoteStatusCodeField(), "relation_remote_status_code");
    EXPECT_STREQ(RemoteErrorCodeField(), "relation_remote_error_code");
    EXPECT_STREQ(InvalidPayloadJsonMessage(), "invalid relation payload json");
    EXPECT_STREQ(StubNotConfiguredMessage(), "relation grpc stub is not configured");
    EXPECT_STREQ(BusinessErrorMessage(), "relation grpc business error");
}

TEST(RelationGrpcClientAlgorithmsTest, ClassifiesPayloadGuards)
{
    using namespace memochat::tests::chat::relation_grpc_client;

    EXPECT_FALSE(ShouldMergePayload(true));
    EXPECT_TRUE(ShouldMergePayload(false));

    EXPECT_FALSE(ShouldReportInvalidPayload(true, true));
    EXPECT_TRUE(ShouldReportInvalidPayload(false, true));
    EXPECT_TRUE(ShouldReportInvalidPayload(true, false));
    EXPECT_TRUE(ShouldReportInvalidPayload(false, false));
}

TEST(RelationGrpcClientAlgorithmsTest, ClassifiesGrpcAndBusinessErrorGuards)
{
    using namespace memochat::tests::chat::relation_grpc_client;

    EXPECT_FALSE(ShouldReportMissingStub(true));
    EXPECT_TRUE(ShouldReportMissingStub(false));

    EXPECT_FALSE(ShouldReportStatusError(true));
    EXPECT_TRUE(ShouldReportStatusError(false));

    EXPECT_FALSE(ShouldReportBusinessError(0, 0));
    EXPECT_TRUE(ShouldReportBusinessError(1, 0));
}
