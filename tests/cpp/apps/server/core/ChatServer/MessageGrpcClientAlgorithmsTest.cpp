#include <gtest/gtest.h>

namespace memochat::tests::chat::message_grpc_client
{
const char* UnknownMethod();
const char* BuildGroupListMethod();
const char* PrivateCommandMethodName(int rpc);
const char* GroupCommandMethodName(int rpc);
const char* ErrorField();
const char* RemoteMethodField();
const char* RemoteErrorField();
const char* RemoteStatusCodeField();
const char* RemoteErrorCodeField();
const char* InvalidPayloadJsonMessage();
const char* PrivateStubNotConfiguredMessage();
const char* GroupStubNotConfiguredMessage();
const char* PrivateBusinessErrorMessage();
const char* GroupBusinessErrorMessage();
bool ShouldMergePayload(bool payload_empty);
bool ShouldReportInvalidPayload(bool parse_ok, bool is_object);
bool ShouldReportMissingStub(bool has_stub);
bool ShouldReportStatusError(bool status_ok);
bool ShouldReportBusinessError(int response_error, int success_code);
} // namespace memochat::tests::chat::message_grpc_client

TEST(MessageGrpcClientAlgorithmsTest, SelectsStablePrivateCommandMethodNames)
{
    using namespace memochat::tests::chat::message_grpc_client;

    EXPECT_STREQ(UnknownMethod(), "unknown");
    EXPECT_STREQ(PrivateCommandMethodName(0), "SendTextChatMessage");
    EXPECT_STREQ(PrivateCommandMethodName(1), "ForwardPrivateMessage");
    EXPECT_STREQ(PrivateCommandMethodName(2), "PrivateReadAck");
    EXPECT_STREQ(PrivateCommandMethodName(3), "EditPrivateMessage");
    EXPECT_STREQ(PrivateCommandMethodName(4), "RevokePrivateMessage");
    EXPECT_STREQ(PrivateCommandMethodName(5), "PrivateHistory");
    EXPECT_STREQ(PrivateCommandMethodName(6), "unknown");
}

TEST(MessageGrpcClientAlgorithmsTest, SelectsStableGroupCommandMethodNames)
{
    using namespace memochat::tests::chat::message_grpc_client;

    EXPECT_STREQ(BuildGroupListMethod(), "BuildGroupList");
    EXPECT_STREQ(GroupCommandMethodName(0), "CreateGroup");
    EXPECT_STREQ(GroupCommandMethodName(1), "GetGroupList");
    EXPECT_STREQ(GroupCommandMethodName(2), "InviteGroupMember");
    EXPECT_STREQ(GroupCommandMethodName(3), "ApplyJoinGroup");
    EXPECT_STREQ(GroupCommandMethodName(4), "ReviewGroupApply");
    EXPECT_STREQ(GroupCommandMethodName(5), "GroupChatMessage");
    EXPECT_STREQ(GroupCommandMethodName(6), "GroupHistory");
    EXPECT_STREQ(GroupCommandMethodName(7), "EditGroupMessage");
    EXPECT_STREQ(GroupCommandMethodName(8), "RevokeGroupMessage");
    EXPECT_STREQ(GroupCommandMethodName(9), "ForwardGroupMessage");
    EXPECT_STREQ(GroupCommandMethodName(10), "GroupReadAck");
    EXPECT_STREQ(GroupCommandMethodName(11), "UpdateGroupAnnouncement");
    EXPECT_STREQ(GroupCommandMethodName(12), "UpdateGroupIcon");
    EXPECT_STREQ(GroupCommandMethodName(13), "SetGroupAdmin");
    EXPECT_STREQ(GroupCommandMethodName(14), "MuteGroupMember");
    EXPECT_STREQ(GroupCommandMethodName(15), "KickGroupMember");
    EXPECT_STREQ(GroupCommandMethodName(16), "QuitGroup");
    EXPECT_STREQ(GroupCommandMethodName(17), "DissolveGroup");
    EXPECT_STREQ(GroupCommandMethodName(18), "unknown");
}

TEST(MessageGrpcClientAlgorithmsTest, KeepsRemoteErrorFieldNamesAndMessages)
{
    using namespace memochat::tests::chat::message_grpc_client;

    EXPECT_STREQ(ErrorField(), "error");
    EXPECT_STREQ(RemoteMethodField(), "message_remote_method");
    EXPECT_STREQ(RemoteErrorField(), "message_remote_error");
    EXPECT_STREQ(RemoteStatusCodeField(), "message_remote_status_code");
    EXPECT_STREQ(RemoteErrorCodeField(), "message_remote_error_code");
    EXPECT_STREQ(InvalidPayloadJsonMessage(), "invalid message payload json");
    EXPECT_STREQ(PrivateStubNotConfiguredMessage(), "private message grpc stub is not configured");
    EXPECT_STREQ(GroupStubNotConfiguredMessage(), "group message grpc stub is not configured");
    EXPECT_STREQ(PrivateBusinessErrorMessage(), "private message grpc business error");
    EXPECT_STREQ(GroupBusinessErrorMessage(), "group message grpc business error");
}

TEST(MessageGrpcClientAlgorithmsTest, ClassifiesPayloadGuards)
{
    using namespace memochat::tests::chat::message_grpc_client;

    EXPECT_FALSE(ShouldMergePayload(true));
    EXPECT_TRUE(ShouldMergePayload(false));

    EXPECT_FALSE(ShouldReportInvalidPayload(true, true));
    EXPECT_TRUE(ShouldReportInvalidPayload(false, true));
    EXPECT_TRUE(ShouldReportInvalidPayload(true, false));
    EXPECT_TRUE(ShouldReportInvalidPayload(false, false));
}

TEST(MessageGrpcClientAlgorithmsTest, ClassifiesGrpcAndBusinessErrorGuards)
{
    using namespace memochat::tests::chat::message_grpc_client;

    EXPECT_FALSE(ShouldReportMissingStub(true));
    EXPECT_TRUE(ShouldReportMissingStub(false));

    EXPECT_FALSE(ShouldReportStatusError(true));
    EXPECT_TRUE(ShouldReportStatusError(false));

    EXPECT_FALSE(ShouldReportBusinessError(0, 0));
    EXPECT_TRUE(ShouldReportBusinessError(1, 0));
}
