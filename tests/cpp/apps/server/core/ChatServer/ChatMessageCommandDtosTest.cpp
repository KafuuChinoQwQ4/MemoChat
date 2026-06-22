#include <gtest/gtest.h>

#include "ChatMessageCommandDtos.h"
#include "json/GlazeCompat.h"
#include "message/GroupResponseFormatter.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateEditRequestDto>(
    std::array<std::string_view, 4>{"uid", "peer_uid", "msgid", "content"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateForwardRequestDto>(
    std::array<std::string_view, 4>{"from_uid", "peer_uid", "source_msg_id", "client_msg_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateRevokeRequestDto>(
    std::array<std::string_view, 3>{"uid", "peer_uid", "msgid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateMessageChangedResultDto>(
    std::array<std::string_view, 7>{"error", "fromuid", "peer_uid", "msgid", "content", "changed_at_ms", "deleted"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateMessageChangedEventDto>(
    std::array<std::string_view,
               8>{"error", "event", "fromuid", "peer_uid", "msgid", "content", "changed_at_ms", "deleted"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateReadAckEventDto>(
    std::array<std::string_view, 5>{"error", "event", "fromuid", "peer_uid", "read_ts"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateForwardMessageDto>(
    std::array<std::string_view, 5>{"msgid", "content", "created_at", "reply_to_server_msg_id", "forward_meta"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateForwardResultDto>(
    std::array<std::string_view, 7>{"error", "fromuid", "peer_uid", "touid", "client_msg_id", "created_at", "msg"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupEditRequestDto>(
    std::array<std::string_view, 4>{"uid", "group_id", "msgid", "content"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupForwardRequestDto>(
    std::array<std::string_view, 4>{"from_uid", "group_id", "source_msg_id", "client_msg_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupRevokeRequestDto>(
    std::array<std::string_view, 3>{"uid", "group_id", "msgid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupMessageChangedResultDto>(
    std::array<std::string_view, 6>{"error", "groupid", "msgid", "content", "changed_at_ms", "deleted"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupMessageChangedEventDto>(
    std::array<std::string_view,
               8>{"error", "event", "groupid", "msgid", "content", "changed_at_ms", "operator_uid", "deleted"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupReadAckEventDto>(
    std::array<std::string_view, 5>{"error", "event", "groupid", "fromuid", "read_ts"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupInviteCreatedEventDto>(
    std::array<std::string_view, 6>{"error", "event", "groupid", "group_code", "name", "operator_uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupInviteMemberEventDto>(
    std::array<std::string_view,
               8>{"error", "event", "groupid", "group_code", "name", "operator_uid", "target_user_id", "reason"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupApplyEventDto>(
    std::array<std::string_view,
               7>{"error", "event", "groupid", "group_code", "applicant_uid", "applicant_user_id", "reason"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupApplyReviewedEventDto>(
    std::array<std::string_view, 8>{"error",
                                    "event",
                                    "groupid",
                                    "group_code",
                                    "applicant_uid",
                                    "applicant_user_id",
                                    "agree",
                                    "operator_uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupAnnouncementUpdatedEventDto>(
    std::array<std::string_view, 6>{"error", "event", "groupid", "group_code", "announcement", "operator_uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupIconUpdatedEventDto>(
    std::array<std::string_view, 6>{"error", "event", "groupid", "group_code", "icon", "operator_uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupAdminChangedEventDto>(
    std::array<std::string_view, 9>{"error",
                                    "event",
                                    "groupid",
                                    "group_code",
                                    "operator_uid",
                                    "target_uid",
                                    "target_user_id",
                                    "is_admin",
                                    "permission_bits"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupMuteChangedEventDto>(
    std::array<std::string_view, 8>{"error",
                                    "event",
                                    "groupid",
                                    "group_code",
                                    "operator_uid",
                                    "target_uid",
                                    "target_user_id",
                                    "mute_until"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupMemberKickedEventDto>(
    std::array<std::string_view,
               7>{"error", "event", "groupid", "group_code", "operator_uid", "target_uid", "target_user_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupMemberQuitEventDto>(
    std::array<std::string_view, 5>{"error", "event", "groupid", "group_code", "target_uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupDissolvedEventDto>(
    std::array<std::string_view, 6>{"error", "event", "groupid", "group_code", "name", "operator_uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupForwardMessageDto>(
    std::array<std::string_view, 13>{"msgid",
                                     "msgtype",
                                     "content",
                                     "mentions",
                                     "file_name",
                                     "mime",
                                     "size",
                                     "forwarded_from_msgid",
                                     "created_at",
                                     "server_msg_id",
                                     "group_seq",
                                     "reply_to_server_msg_id",
                                     "forward_meta"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupForwardResultDto>(
    std::array<std::string_view, 15>{"error",
                                     "fromuid",
                                     "groupid",
                                     "client_msg_id",
                                     "created_at",
                                     "server_msg_id",
                                     "group_seq",
                                     "reply_to_server_msg_id",
                                     "forward_meta",
                                     "msg",
                                     "from_name",
                                     "from_nick",
                                     "from_icon",
                                     "from_user_id",
                                     "group_code"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateSendEventDto>(
    std::array<std::string_view,
               8>{"fromuid", "touid", "trace_id", "request_id", "span_id", "event_id", "accept_node", "accept_ts"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupSendEventDto>(
    std::array<std::string_view,
               8>{"fromuid", "groupid", "trace_id", "request_id", "span_id", "event_id", "accept_node", "accept_ts"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatPrivateSendResponseDto>(
    std::array<std::string_view,
               8>{"error", "fromuid", "touid", "client_msg_id", "accept_node", "accept_ts", "status", "text_array"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::command::ChatGroupSendResponseDto>(
    std::array<std::string_view, 16>{"error",
                                     "fromuid",
                                     "groupid",
                                     "client_msg_id",
                                     "accept_node",
                                     "accept_ts",
                                     "status",
                                     "from_name",
                                     "from_nick",
                                     "from_icon",
                                     "from_user_id",
                                     "group_code",
                                     "msg",
                                     "created_at",
                                     "server_msg_id",
                                     "group_seq"}));
#endif

namespace
{

memochat::json::JsonValue Parse(std::string_view body)
{
    memochat::json::JsonValue root;
    EXPECT_TRUE(memochat::json::glaze_parse(root, body)) << std::string(body);
    return root;
}

} // namespace

TEST(ChatMessageCommandDtosTest, ReadsPrivateEditRequest)
{
    const auto request = memochat::chat::command::ChatPrivateEditRequestFromJsonValue(
        Parse(R"({"fromuid":10,"peer_uid":20,"msgid":"m-1","content":"edited"})"));

    EXPECT_EQ(request.uid, 10);
    EXPECT_EQ(request.peer_uid, 20);
    EXPECT_EQ(request.msgid, "m-1");
    EXPECT_EQ(request.content, "edited");
}

TEST(ChatMessageCommandDtosTest, ReadsPrivateEditDefaults)
{
    const auto request =
        memochat::chat::command::ChatPrivateEditRequestFromJsonValue(Parse(R"({"fromuid":11,"peer_uid":21})"));

    EXPECT_EQ(request.uid, 11);
    EXPECT_EQ(request.peer_uid, 21);
    EXPECT_TRUE(request.msgid.empty());
    EXPECT_TRUE(request.content.empty());
}

TEST(ChatMessageCommandDtosTest, ReadsPrivateForwardRequest)
{
    const auto request = memochat::chat::command::ChatPrivateForwardRequestFromJsonValue(
        Parse(R"({"fromuid":12,"peer_uid":22,"msgid":"source-1","client_msg_id":"client-1"})"));

    EXPECT_EQ(request.from_uid, 12);
    EXPECT_EQ(request.peer_uid, 22);
    EXPECT_EQ(request.source_msg_id, "source-1");
    EXPECT_EQ(request.client_msg_id, "client-1");
}

TEST(ChatMessageCommandDtosTest, ReadsPrivateForwardAliasesAndDefaults)
{
    const auto alias =
        memochat::chat::command::ChatPrivateForwardRequestFromJsonValue(Parse(R"({"fromuid":13,"peer_uid":23})"));
    EXPECT_EQ(alias.from_uid, 13);
    EXPECT_EQ(alias.peer_uid, 23);
    EXPECT_TRUE(alias.source_msg_id.empty());
    EXPECT_TRUE(alias.client_msg_id.empty());

    const auto preferred = memochat::chat::command::ChatPrivateForwardRequestFromJsonValue(
        Parse(R"({"fromuid":14,"peer_uid":24,"msgid":"source-2"})"));
    EXPECT_EQ(preferred.from_uid, 14);
    EXPECT_EQ(preferred.peer_uid, 24);
    EXPECT_EQ(preferred.source_msg_id, "source-2");
    EXPECT_TRUE(preferred.client_msg_id.empty());
}

TEST(ChatMessageCommandDtosTest, ReadsPrivateRevokeRequest)
{
    const auto request = memochat::chat::command::ChatPrivateRevokeRequestFromJsonValue(
        Parse(R"({"fromuid":15,"peer_uid":25,"msgid":"m-2"})"));

    EXPECT_EQ(request.uid, 15);
    EXPECT_EQ(request.peer_uid, 25);
    EXPECT_EQ(request.msgid, "m-2");
}

TEST(ChatMessageCommandDtosTest, ReadsPrivateRevokeDefaults)
{
    const auto request =
        memochat::chat::command::ChatPrivateRevokeRequestFromJsonValue(Parse(R"({"fromuid":16,"peer_uid":26})"));

    EXPECT_EQ(request.uid, 16);
    EXPECT_EQ(request.peer_uid, 26);
    EXPECT_TRUE(request.msgid.empty());
}

TEST(ChatMessageCommandDtosTest, WritesPrivateEditResultAndNotifyPayloads)
{
    const memochat::chat::command::ChatPrivateMessageChangedResultDto result{.error = 0,
                                                                             .fromuid = 10,
                                                                             .peer_uid = 20,
                                                                             .msgid = "m-1",
                                                                             .content = "edited",
                                                                             .changed_at_ms = 1234};
    const auto result_json = memochat::chat::command::ToJsonValue(result);

    EXPECT_EQ(result_json["error"].asInt(), 0);
    EXPECT_EQ(result_json["fromuid"].asInt(), 10);
    EXPECT_EQ(result_json["peer_uid"].asInt(), 20);
    EXPECT_EQ(result_json["msgid"].asString(), "m-1");
    EXPECT_EQ(result_json["content"].asString(), "edited");
    EXPECT_EQ(result_json["edited_at_ms"].asInt64(), 1234);
    EXPECT_FALSE(result_json.isMember("deleted_at_ms"));

    const memochat::chat::command::ChatPrivateMessageChangedEventDto event{.error = 0,
                                                                           .event = "private_msg_edited",
                                                                           .fromuid = 10,
                                                                           .peer_uid = 20,
                                                                           .msgid = "m-1",
                                                                           .content = "edited",
                                                                           .changed_at_ms = 1234};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["event"].asString(), "private_msg_edited");
    EXPECT_EQ(event_json["fromuid"].asInt(), 10);
    EXPECT_EQ(event_json["peer_uid"].asInt(), 20);
    EXPECT_EQ(event_json["msgid"].asString(), "m-1");
    EXPECT_EQ(event_json["content"].asString(), "edited");
    EXPECT_EQ(event_json["edited_at_ms"].asInt64(), 1234);
    EXPECT_FALSE(event_json.isMember("deleted_at_ms"));
}

TEST(ChatMessageCommandDtosTest, WritesPrivateRevokeResultAndNotifyPayloads)
{
    const memochat::chat::command::ChatPrivateMessageChangedResultDto result{.error = 0,
                                                                             .fromuid = 11,
                                                                             .peer_uid = 21,
                                                                             .msgid = "m-2",
                                                                             .content = "[消息已撤回]",
                                                                             .changed_at_ms = 2345,
                                                                             .deleted = true};
    const auto result_json = memochat::chat::command::ToJsonValue(result);

    EXPECT_EQ(result_json["fromuid"].asInt(), 11);
    EXPECT_EQ(result_json["peer_uid"].asInt(), 21);
    EXPECT_EQ(result_json["msgid"].asString(), "m-2");
    EXPECT_EQ(result_json["content"].asString(), "[消息已撤回]");
    EXPECT_EQ(result_json["deleted_at_ms"].asInt64(), 2345);
    EXPECT_FALSE(result_json.isMember("edited_at_ms"));

    const memochat::chat::command::ChatPrivateMessageChangedEventDto event{.error = 0,
                                                                           .event = "private_msg_revoked",
                                                                           .fromuid = 11,
                                                                           .peer_uid = 21,
                                                                           .msgid = "m-2",
                                                                           .content = "[消息已撤回]",
                                                                           .changed_at_ms = 2345,
                                                                           .deleted = true};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["event"].asString(), "private_msg_revoked");
    EXPECT_EQ(event_json["deleted_at_ms"].asInt64(), 2345);
    EXPECT_FALSE(event_json.isMember("edited_at_ms"));
}

TEST(ChatMessageCommandDtosTest, WritesPrivateReadAckNotifyPayload)
{
    const memochat::chat::command::ChatPrivateReadAckEventDto event{.error = 0,
                                                                    .event = "private_read_ack",
                                                                    .fromuid = 31,
                                                                    .peer_uid = 41,
                                                                    .read_ts = 7001};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "private_read_ack");
    EXPECT_EQ(event_json["fromuid"].asInt(), 31);
    EXPECT_EQ(event_json["peer_uid"].asInt(), 41);
    EXPECT_EQ(event_json["read_ts"].asInt64(), 7001);
}

TEST(ChatMessageCommandDtosTest, WritesPrivateForwardResultWithNestedMessageAndTextArray)
{
    const auto forward_meta =
        Parse(R"({"forwarded_from_msgid":"source-1","source_from_uid":12,"prev_forward_meta":{"depth":1}})");
    const memochat::chat::command::ChatPrivateForwardMessageDto message{.msgid = "client-1",
                                                                        .content = "forwarded content",
                                                                        .created_at = 5001,
                                                                        .reply_to_server_msg_id = 9001,
                                                                        .forward_meta = forward_meta};
    const auto message_json = memochat::chat::command::ToJsonValue(message);

    EXPECT_EQ(message_json["msgid"].asString(), "client-1");
    EXPECT_EQ(message_json["content"].asString(), "forwarded content");
    EXPECT_EQ(message_json["created_at"].asInt64(), 5001);
    EXPECT_EQ(message_json["reply_to_server_msg_id"].asInt64(), 9001);
    EXPECT_EQ(message_json["forward_meta"]["forwarded_from_msgid"].asString(), "source-1");
    EXPECT_EQ(message_json["forward_meta"]["prev_forward_meta"]["depth"].asInt(), 1);

    const memochat::chat::command::ChatPrivateForwardResultDto result{.error = 0,
                                                                      .fromuid = 12,
                                                                      .peer_uid = 34,
                                                                      .touid = 34,
                                                                      .client_msg_id = "client-1",
                                                                      .created_at = 5001,
                                                                      .msg = message_json};
    const auto result_json = memochat::chat::command::ToJsonValue(result);

    EXPECT_EQ(result_json["error"].asInt(), 0);
    EXPECT_EQ(result_json["fromuid"].asInt(), 12);
    EXPECT_EQ(result_json["peer_uid"].asInt(), 34);
    EXPECT_EQ(result_json["touid"].asInt(), 34);
    EXPECT_EQ(result_json["client_msg_id"].asString(), "client-1");
    EXPECT_EQ(result_json["created_at"].asInt64(), 5001);
    EXPECT_EQ(result_json["msg"]["msgid"].asString(), "client-1");
    const auto text_array = result_json["text_array"];
    ASSERT_TRUE(text_array.isArray()) << result_json.toStyledString();
    const auto text_item = text_array[0];
    EXPECT_EQ(text_item["msgid"].asString(), "client-1");
    EXPECT_EQ(text_item["forward_meta"]["source_from_uid"].asInt(), 12);
}

TEST(ChatMessageCommandDtosTest, WritesPrivateForwardInitialResultWithoutOptionalPayloads)
{
    const auto result_json = memochat::chat::command::ToJsonValue(
        memochat::chat::command::ChatPrivateForwardResultDto{.error = 0, .fromuid = 13, .peer_uid = 35, .touid = 35});

    EXPECT_EQ(result_json["error"].asInt(), 0);
    EXPECT_EQ(result_json["fromuid"].asInt(), 13);
    EXPECT_EQ(result_json["peer_uid"].asInt(), 35);
    EXPECT_EQ(result_json["touid"].asInt(), 35);
    EXPECT_FALSE(result_json.isMember("client_msg_id")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("created_at")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("msg")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("text_array")) << result_json.toStyledString();
}

TEST(ChatMessageCommandDtosTest, ReadsGroupEditRequest)
{
    const auto request = memochat::chat::command::ChatGroupEditRequestFromJsonValue(
        Parse(R"({"fromuid":17,"groupid":300,"msgid":"g-1","content":"edited group"})"));

    EXPECT_EQ(request.uid, 17);
    EXPECT_EQ(request.group_id, 300);
    EXPECT_EQ(request.msgid, "g-1");
    EXPECT_EQ(request.content, "edited group");
}

TEST(ChatMessageCommandDtosTest, ReadsGroupEditDefaults)
{
    const auto request =
        memochat::chat::command::ChatGroupEditRequestFromJsonValue(Parse(R"({"fromuid":18,"groupid":301})"));

    EXPECT_EQ(request.uid, 18);
    EXPECT_EQ(request.group_id, 301);
    EXPECT_TRUE(request.msgid.empty());
    EXPECT_TRUE(request.content.empty());
}

TEST(ChatMessageCommandDtosTest, ReadsGroupForwardRequest)
{
    const auto request = memochat::chat::command::ChatGroupForwardRequestFromJsonValue(
        Parse(R"({"fromuid":19,"groupid":302,"msgid":"source-g-1","client_msg_id":"client-g-1"})"));

    EXPECT_EQ(request.from_uid, 19);
    EXPECT_EQ(request.group_id, 302);
    EXPECT_EQ(request.source_msg_id, "source-g-1");
    EXPECT_EQ(request.client_msg_id, "client-g-1");
}

TEST(ChatMessageCommandDtosTest, ReadsGroupForwardAliasesAndDefaults)
{
    const auto alias = memochat::chat::command::ChatGroupForwardRequestFromJsonValue(Parse(R"({"fromuid":20})"));
    EXPECT_EQ(alias.from_uid, 20);
    EXPECT_EQ(alias.group_id, 0);
    EXPECT_TRUE(alias.source_msg_id.empty());
    EXPECT_TRUE(alias.client_msg_id.empty());

    const auto preferred = memochat::chat::command::ChatGroupForwardRequestFromJsonValue(
        Parse(R"({"fromuid":21,"groupid":303,"msgid":"source-g-2"})"));
    EXPECT_EQ(preferred.from_uid, 21);
    EXPECT_EQ(preferred.group_id, 303);
    EXPECT_EQ(preferred.source_msg_id, "source-g-2");
    EXPECT_TRUE(preferred.client_msg_id.empty());
}

TEST(ChatMessageCommandDtosTest, ReadsGroupRevokeRequest)
{
    const auto request = memochat::chat::command::ChatGroupRevokeRequestFromJsonValue(
        Parse(R"({"fromuid":22,"groupid":304,"msgid":"g-2"})"));

    EXPECT_EQ(request.uid, 22);
    EXPECT_EQ(request.group_id, 304);
    EXPECT_EQ(request.msgid, "g-2");
}

TEST(ChatMessageCommandDtosTest, ReadsGroupRevokeDefaults)
{
    const auto request =
        memochat::chat::command::ChatGroupRevokeRequestFromJsonValue(Parse(R"({"fromuid":23,"groupid":305})"));

    EXPECT_EQ(request.uid, 23);
    EXPECT_EQ(request.group_id, 305);
    EXPECT_TRUE(request.msgid.empty());
}

TEST(ChatMessageCommandDtosTest, WritesGroupEditResultAndNotifyPayloads)
{
    const memochat::chat::command::ChatGroupMessageChangedResultDto result{.error = 0,
                                                                           .groupid = 300,
                                                                           .msgid = "g-1",
                                                                           .content = "edited group",
                                                                           .changed_at_ms = 3456};
    const auto result_json = memochat::chat::command::ToJsonValue(result);

    EXPECT_EQ(result_json["error"].asInt(), 0);
    EXPECT_EQ(result_json["groupid"].asInt64(), 300);
    EXPECT_EQ(result_json["msgid"].asString(), "g-1");
    EXPECT_EQ(result_json["content"].asString(), "edited group");
    EXPECT_EQ(result_json["edited_at_ms"].asInt64(), 3456);
    EXPECT_FALSE(result_json.isMember("deleted_at_ms"));

    const memochat::chat::command::ChatGroupMessageChangedEventDto event{.error = 0,
                                                                         .event = "group_msg_edited",
                                                                         .groupid = 300,
                                                                         .msgid = "g-1",
                                                                         .content = "edited group",
                                                                         .changed_at_ms = 3456,
                                                                         .operator_uid = 17};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["event"].asString(), "group_msg_edited");
    EXPECT_EQ(event_json["groupid"].asInt64(), 300);
    EXPECT_EQ(event_json["operator_uid"].asInt(), 17);
    EXPECT_EQ(event_json["edited_at_ms"].asInt64(), 3456);
    EXPECT_FALSE(event_json.isMember("deleted_at_ms"));
}

TEST(ChatMessageCommandDtosTest, WritesGroupRevokeResultAndNotifyPayloads)
{
    const memochat::chat::command::ChatGroupMessageChangedResultDto result{.error = 0,
                                                                           .groupid = 301,
                                                                           .msgid = "g-2",
                                                                           .content = "[消息已撤回]",
                                                                           .changed_at_ms = 4567,
                                                                           .deleted = true};
    const auto result_json = memochat::chat::command::ToJsonValue(result);

    EXPECT_EQ(result_json["groupid"].asInt64(), 301);
    EXPECT_EQ(result_json["msgid"].asString(), "g-2");
    EXPECT_EQ(result_json["content"].asString(), "[消息已撤回]");
    EXPECT_EQ(result_json["deleted_at_ms"].asInt64(), 4567);
    EXPECT_FALSE(result_json.isMember("edited_at_ms"));

    const memochat::chat::command::ChatGroupMessageChangedEventDto event{.error = 0,
                                                                         .event = "group_msg_revoked",
                                                                         .groupid = 301,
                                                                         .msgid = "g-2",
                                                                         .content = "[消息已撤回]",
                                                                         .changed_at_ms = 4567,
                                                                         .operator_uid = 18,
                                                                         .deleted = true};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["event"].asString(), "group_msg_revoked");
    EXPECT_EQ(event_json["operator_uid"].asInt(), 18);
    EXPECT_EQ(event_json["deleted_at_ms"].asInt64(), 4567);
    EXPECT_FALSE(event_json.isMember("edited_at_ms"));
}

TEST(ChatMessageCommandDtosTest, WritesGroupReadAckNotifyPayload)
{
    const memochat::chat::command::ChatGroupReadAckEventDto event{.error = 0,
                                                                  .event = "group_read_ack",
                                                                  .groupid = 3301,
                                                                  .fromuid = 32,
                                                                  .read_ts = 8001};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_read_ack");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3301);
    EXPECT_EQ(event_json["fromuid"].asInt(), 32);
    EXPECT_EQ(event_json["read_ts"].asInt64(), 8001);
}

TEST(ChatMessageCommandDtosTest, WritesGroupInviteCreatedNotifyPayload)
{
    const memochat::chat::command::ChatGroupInviteCreatedEventDto event{.error = 0,
                                                                        .event = "group_invite",
                                                                        .groupid = 3310,
                                                                        .group_code = "G3310",
                                                                        .name = "Group 3310",
                                                                        .operator_uid = 41};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_invite");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3310);
    EXPECT_EQ(event_json["group_code"].asString(), "G3310");
    EXPECT_EQ(event_json["name"].asString(), "Group 3310");
    EXPECT_EQ(event_json["operator_uid"].asInt(), 41);
    EXPECT_FALSE(event_json.isMember("target_user_id"));
    EXPECT_FALSE(event_json.isMember("reason"));
}

TEST(ChatMessageCommandDtosTest, WritesGroupInviteMemberNotifyPayload)
{
    const memochat::chat::command::ChatGroupInviteMemberEventDto event{.error = 0,
                                                                       .event = "group_invite",
                                                                       .groupid = 3311,
                                                                       .group_code = "G3311",
                                                                       .name = "Group 3311",
                                                                       .operator_uid = 42,
                                                                       .target_user_id = "target-42",
                                                                       .reason = "join us"};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_invite");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3311);
    EXPECT_EQ(event_json["group_code"].asString(), "G3311");
    EXPECT_EQ(event_json["name"].asString(), "Group 3311");
    EXPECT_EQ(event_json["operator_uid"].asInt(), 42);
    EXPECT_EQ(event_json["target_user_id"].asString(), "target-42");
    EXPECT_EQ(event_json["reason"].asString(), "join us");
}

TEST(ChatMessageCommandDtosTest, WritesGroupApplyNotifyPayload)
{
    const memochat::chat::command::ChatGroupApplyEventDto event{.error = 0,
                                                                .event = "group_apply",
                                                                .groupid = 3312,
                                                                .group_code = "G3312",
                                                                .applicant_uid = 43,
                                                                .applicant_user_id =
                                                                    std::optional<std::string>{"applicant-43"},
                                                                .reason = "please"};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_apply");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3312);
    EXPECT_EQ(event_json["group_code"].asString(), "G3312");
    EXPECT_EQ(event_json["applicant_uid"].asInt(), 43);
    EXPECT_EQ(event_json["applicant_user_id"].asString(), "applicant-43");
    EXPECT_EQ(event_json["reason"].asString(), "please");

    const memochat::chat::command::ChatGroupApplyEventDto without_user_id{.error = 0,
                                                                          .event = "group_apply",
                                                                          .groupid = 3313,
                                                                          .group_code = "G3313",
                                                                          .applicant_uid = 44,
                                                                          .reason = ""};
    const auto without_user_id_json = memochat::chat::command::ToJsonValue(without_user_id);
    EXPECT_FALSE(without_user_id_json.isMember("applicant_user_id"));
    EXPECT_EQ(without_user_id_json["reason"].asString(), "");
}

TEST(ChatMessageCommandDtosTest, WritesGroupApplyReviewedNotifyPayload)
{
    const memochat::chat::command::ChatGroupApplyReviewedEventDto event{.error = 0,
                                                                        .event = "group_member_changed",
                                                                        .groupid = 3314,
                                                                        .group_code = "G3314",
                                                                        .applicant_uid = 45,
                                                                        .applicant_user_id =
                                                                            std::optional<std::string>{"applicant-45"},
                                                                        .agree = true,
                                                                        .operator_uid = 55};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_member_changed");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3314);
    EXPECT_EQ(event_json["group_code"].asString(), "G3314");
    EXPECT_EQ(event_json["applicant_uid"].asInt(), 45);
    EXPECT_EQ(event_json["applicant_user_id"].asString(), "applicant-45");
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(event_json, "agree", false));
    EXPECT_EQ(event_json["operator_uid"].asInt(), 55);
}

TEST(ChatMessageCommandDtosTest, WritesGroupAnnouncementUpdatedNotifyPayload)
{
    const memochat::chat::command::ChatGroupAnnouncementUpdatedEventDto event{.error = 0,
                                                                              .event = "group_announcement_updated",
                                                                              .groupid = 3302,
                                                                              .group_code = "G3302",
                                                                              .announcement = "new announcement",
                                                                              .operator_uid = 33};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_announcement_updated");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3302);
    EXPECT_EQ(event_json["group_code"].asString(), "G3302");
    EXPECT_EQ(event_json["announcement"].asString(), "new announcement");
    EXPECT_EQ(event_json["operator_uid"].asInt(), 33);
}

TEST(ChatMessageCommandDtosTest, WritesGroupIconUpdatedNotifyPayload)
{
    const memochat::chat::command::ChatGroupIconUpdatedEventDto event{.error = 0,
                                                                      .event = "group_icon_updated",
                                                                      .groupid = 3303,
                                                                      .group_code = "",
                                                                      .icon = "group.png",
                                                                      .operator_uid = 34};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_icon_updated");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3303);
    EXPECT_EQ(event_json["group_code"].asString(), "");
    EXPECT_EQ(event_json["icon"].asString(), "group.png");
    EXPECT_EQ(event_json["operator_uid"].asInt(), 34);
}

TEST(ChatMessageCommandDtosTest, WritesGroupAdminChangedNotifyPayloadAndPermissionFlags)
{
    const int64_t permission_bits = memochat::chat::message::GroupResponseFormatter::kPermChangeGroupInfo |
                                    memochat::chat::message::GroupResponseFormatter::kPermManageAdmins |
                                    memochat::chat::message::GroupResponseFormatter::kPermBanUsers;
    const memochat::chat::command::ChatGroupAdminChangedEventDto event{.error = 0,
                                                                       .event = "group_admin_changed",
                                                                       .groupid = 3304,
                                                                       .group_code = "G3304",
                                                                       .operator_uid = 35,
                                                                       .target_uid = 45,
                                                                       .target_user_id = "target-45",
                                                                       .is_admin = true,
                                                                       .permission_bits = permission_bits};
    auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_admin_changed");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3304);
    EXPECT_EQ(event_json["group_code"].asString(), "G3304");
    EXPECT_EQ(event_json["operator_uid"].asInt(), 35);
    EXPECT_EQ(event_json["target_uid"].asInt(), 45);
    EXPECT_EQ(event_json["target_user_id"].asString(), "target-45");
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(event_json, "is_admin", false));
    EXPECT_EQ(event_json["permission_bits"].asInt64(), permission_bits);
    EXPECT_FALSE(event_json.isMember("can_change_group_info"));
    EXPECT_FALSE(event_json.isMember("can_manage_admins"));
    EXPECT_FALSE(event_json.isMember("can_ban_users"));

    memochat::chat::message::GroupResponseFormatter::ApplyPermissionFlags(event_json, permission_bits);

    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(event_json, "can_change_group_info", false));
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(event_json, "can_delete_messages", true));
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(event_json, "can_invite_users", true));
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(event_json, "can_manage_admins", false));
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(event_json, "can_pin_messages", true));
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(event_json, "can_ban_users", false));
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(event_json, "can_manage_topics", true));
}

TEST(ChatMessageCommandDtosTest, WritesGroupMuteChangedNotifyPayload)
{
    const memochat::chat::command::ChatGroupMuteChangedEventDto event{.error = 0,
                                                                      .event = "group_mute_changed",
                                                                      .groupid = 3304,
                                                                      .group_code = "G3304",
                                                                      .operator_uid = 35,
                                                                      .target_uid = 45,
                                                                      .target_user_id = "target-45",
                                                                      .mute_until = 9001};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_mute_changed");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3304);
    EXPECT_EQ(event_json["group_code"].asString(), "G3304");
    EXPECT_EQ(event_json["operator_uid"].asInt(), 35);
    EXPECT_EQ(event_json["target_uid"].asInt(), 45);
    EXPECT_EQ(event_json["target_user_id"].asString(), "target-45");
    EXPECT_EQ(event_json["mute_until"].asInt64(), 9001);
}

TEST(ChatMessageCommandDtosTest, WritesGroupMemberKickedNotifyPayload)
{
    const memochat::chat::command::ChatGroupMemberKickedEventDto event{.error = 0,
                                                                       .event = "group_member_kicked",
                                                                       .groupid = 3305,
                                                                       .group_code = "G3305",
                                                                       .operator_uid = 36,
                                                                       .target_uid = 46,
                                                                       .target_user_id = "target-46"};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_member_kicked");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3305);
    EXPECT_EQ(event_json["group_code"].asString(), "G3305");
    EXPECT_EQ(event_json["operator_uid"].asInt(), 36);
    EXPECT_EQ(event_json["target_uid"].asInt(), 46);
    EXPECT_EQ(event_json["target_user_id"].asString(), "target-46");
}

TEST(ChatMessageCommandDtosTest, WritesGroupMemberQuitNotifyPayload)
{
    const memochat::chat::command::ChatGroupMemberQuitEventDto event{.error = 0,
                                                                     .event = "group_member_quit",
                                                                     .groupid = 3306,
                                                                     .group_code = "",
                                                                     .target_uid = 47};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_member_quit");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3306);
    EXPECT_EQ(event_json["group_code"].asString(), "");
    EXPECT_EQ(event_json["target_uid"].asInt(), 47);
}

TEST(ChatMessageCommandDtosTest, WritesGroupDissolvedNotifyPayload)
{
    const memochat::chat::command::ChatGroupDissolvedEventDto event{.error = 0,
                                                                    .event = "group_dissolved",
                                                                    .groupid = 3307,
                                                                    .group_code = "G3307",
                                                                    .name = "Group 3307",
                                                                    .operator_uid = 37};
    const auto event_json = memochat::chat::command::ToJsonValue(event);

    EXPECT_EQ(event_json["error"].asInt(), 0);
    EXPECT_EQ(event_json["event"].asString(), "group_dissolved");
    EXPECT_EQ(event_json["groupid"].asInt64(), 3307);
    EXPECT_EQ(event_json["group_code"].asString(), "G3307");
    EXPECT_EQ(event_json["name"].asString(), "Group 3307");
    EXPECT_EQ(event_json["operator_uid"].asInt(), 37);
}

TEST(ChatMessageCommandDtosTest, WritesGroupForwardResultWithCopiedDynamicFields)
{
    memochat::json::JsonValue mentions(memochat::json::array_t{});
    mentions.append(Parse(R"({"uid":22,"name":"Ada"})"));
    const auto forward_meta =
        Parse(R"({"forwarded_from_msgid":"source-g-1","source_group_seq":44,"prev_forward_meta":{"depth":2}})");
    const memochat::chat::command::ChatGroupForwardMessageDto message{.msgid = "client-g-1",
                                                                      .msgtype = "file",
                                                                      .content = "group forwarded",
                                                                      .mentions = mentions,
                                                                      .file_name = "memo.txt",
                                                                      .mime = "text/plain",
                                                                      .size = 128,
                                                                      .forwarded_from_msgid = "source-g-1",
                                                                      .created_at = 6001,
                                                                      .server_msg_id = 7001,
                                                                      .group_seq = 45,
                                                                      .reply_to_server_msg_id = 8001,
                                                                      .forward_meta = forward_meta};
    const auto message_json = memochat::chat::command::ToJsonValue(message);

    EXPECT_EQ(message_json["msgid"].asString(), "client-g-1");
    EXPECT_EQ(message_json["msgtype"].asString(), "file");
    EXPECT_EQ(message_json["content"].asString(), "group forwarded");
    ASSERT_TRUE(message_json["mentions"].isArray()) << message_json.toStyledString();
    const auto message_mentions = message_json["mentions"];
    EXPECT_EQ(message_mentions[0]["uid"].asInt(), 22);
    EXPECT_EQ(message_json["file_name"].asString(), "memo.txt");
    EXPECT_EQ(message_json["mime"].asString(), "text/plain");
    EXPECT_EQ(message_json["size"].asInt64(), 128);
    EXPECT_EQ(message_json["forwarded_from_msgid"].asString(), "source-g-1");
    EXPECT_EQ(message_json["created_at"].asInt64(), 6001);
    EXPECT_EQ(message_json["server_msg_id"].asInt64(), 7001);
    EXPECT_EQ(message_json["group_seq"].asInt64(), 45);
    EXPECT_EQ(message_json["reply_to_server_msg_id"].asInt64(), 8001);
    EXPECT_EQ(message_json["forward_meta"]["prev_forward_meta"]["depth"].asInt(), 2);

    const memochat::chat::command::ChatGroupForwardResultDto result{.error = 0,
                                                                    .fromuid = 21,
                                                                    .groupid = 3001,
                                                                    .client_msg_id = "client-g-1",
                                                                    .created_at = 6001,
                                                                    .server_msg_id = 7001,
                                                                    .group_seq = 45,
                                                                    .reply_to_server_msg_id = 8001,
                                                                    .forward_meta = forward_meta,
                                                                    .msg = message_json,
                                                                    .from_name = "Alice",
                                                                    .from_nick = "A",
                                                                    .from_icon = "icon.png",
                                                                    .from_user_id = "alice",
                                                                    .group_code = "G3001"};
    const auto result_json = memochat::chat::command::ToJsonValue(result);

    EXPECT_EQ(result_json["error"].asInt(), 0);
    EXPECT_EQ(result_json["fromuid"].asInt(), 21);
    EXPECT_EQ(result_json["groupid"].asInt64(), 3001);
    EXPECT_EQ(result_json["client_msg_id"].asString(), "client-g-1");
    EXPECT_EQ(result_json["created_at"].asInt64(), 6001);
    EXPECT_EQ(result_json["server_msg_id"].asInt64(), 7001);
    EXPECT_EQ(result_json["group_seq"].asInt64(), 45);
    EXPECT_EQ(result_json["reply_to_server_msg_id"].asInt64(), 8001);
    EXPECT_EQ(result_json["forward_meta"]["source_group_seq"].asInt64(), 44);
    const auto result_mentions = result_json["msg"]["mentions"];
    EXPECT_EQ(result_mentions[0]["name"].asString(), "Ada");
    EXPECT_EQ(result_json["from_name"].asString(), "Alice");
    EXPECT_EQ(result_json["from_nick"].asString(), "A");
    EXPECT_EQ(result_json["from_icon"].asString(), "icon.png");
    EXPECT_EQ(result_json["from_user_id"].asString(), "alice");
    EXPECT_EQ(result_json["group_code"].asString(), "G3001");
}

TEST(ChatMessageCommandDtosTest, WritesGroupForwardInitialResultWithoutOptionalPayloads)
{
    const auto result_json = memochat::chat::command::ToJsonValue(
        memochat::chat::command::ChatGroupForwardResultDto{.error = 0, .fromuid = 22, .groupid = 3002});

    EXPECT_EQ(result_json["error"].asInt(), 0);
    EXPECT_EQ(result_json["fromuid"].asInt(), 22);
    EXPECT_EQ(result_json["groupid"].asInt64(), 3002);
    EXPECT_FALSE(result_json.isMember("client_msg_id")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("created_at")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("server_msg_id")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("group_seq")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("reply_to_server_msg_id")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("forward_meta")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("msg")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("from_name")) << result_json.toStyledString();
    EXPECT_FALSE(result_json.isMember("group_code")) << result_json.toStyledString();
}

TEST(ChatMessageCommandDtosTest, WritesPrivateSendEventShellWithDynamicTextArray)
{
    auto event_payload = memochat::chat::command::ToJsonValue(
        memochat::chat::command::ChatPrivateSendEventDto{.fromuid = 10,
                                                         .touid = 20,
                                                         .trace_id = "trace-1",
                                                         .request_id = "req-1",
                                                         .span_id = "span-1",
                                                         .event_id = "evt-1",
                                                         .accept_node = "chatserver1",
                                                         .accept_ts = 7001});

    memochat::json::JsonValue text_array(memochat::json::array_t{});
    text_array.append(Parse(R"({"msgid":"evt-1","content":"hello"})"));
    event_payload["text_array"] = text_array;

    // Read assertions go through a const JsonValue so nested lookups use the
    // value-returning const operator[] chain (the non-const JsonValueProxy chain
    // does not descend into array elements / nested objects).
    const memochat::json::JsonValue out = event_payload;
    EXPECT_EQ(out["fromuid"].asInt(), 10);
    EXPECT_EQ(out["touid"].asInt(), 20);
    EXPECT_EQ(out["trace_id"].asString(), "trace-1");
    EXPECT_EQ(out["request_id"].asString(), "req-1");
    EXPECT_EQ(out["span_id"].asString(), "span-1");
    EXPECT_EQ(out["event_id"].asString(), "evt-1");
    EXPECT_EQ(out["accept_node"].asString(), "chatserver1");
    EXPECT_EQ(out["accept_ts"].asInt64(), 7001);
    ASSERT_TRUE(out["text_array"].isArray()) << out.toStyledString();
    EXPECT_EQ(out["text_array"][0]["msgid"].asString(), "evt-1");
    EXPECT_EQ(out["text_array"][0]["content"].asString(), "hello");
}

TEST(ChatMessageCommandDtosTest, WritesGroupSendEventShellWithDynamicMessageAndTail)
{
    auto event_payload = memochat::chat::command::ToJsonValue(
        memochat::chat::command::ChatGroupSendEventDto{.fromuid = 11,
                                                       .groupid = 3001,
                                                       .trace_id = "trace-2",
                                                       .request_id = "req-2",
                                                       .span_id = "span-2",
                                                       .event_id = "evt-2",
                                                       .accept_node = "chatserver2",
                                                       .accept_ts = 8001});

    event_payload["msg"] = Parse(R"({"msgid":"evt-2","content":"group hello"})");
    event_payload["from_name"] = "Alice";
    event_payload["from_nick"] = "A";
    event_payload["from_icon"] = "icon.png";
    event_payload["from_user_id"] = "alice";
    event_payload["group_code"] = "G3001";

    const memochat::json::JsonValue out = event_payload;
    EXPECT_EQ(out["fromuid"].asInt(), 11);
    EXPECT_EQ(out["groupid"].asInt64(), 3001);
    EXPECT_EQ(out["trace_id"].asString(), "trace-2");
    EXPECT_EQ(out["request_id"].asString(), "req-2");
    EXPECT_EQ(out["span_id"].asString(), "span-2");
    EXPECT_EQ(out["event_id"].asString(), "evt-2");
    EXPECT_EQ(out["accept_node"].asString(), "chatserver2");
    EXPECT_EQ(out["accept_ts"].asInt64(), 8001);
    EXPECT_EQ(out["msg"]["msgid"].asString(), "evt-2");
    EXPECT_EQ(out["msg"]["content"].asString(), "group hello");
    EXPECT_EQ(out["from_name"].asString(), "Alice");
    EXPECT_EQ(out["from_nick"].asString(), "A");
    EXPECT_EQ(out["from_icon"].asString(), "icon.png");
    EXPECT_EQ(out["from_user_id"].asString(), "alice");
    EXPECT_EQ(out["group_code"].asString(), "G3001");
}

TEST(ChatMessageCommandDtosTest, WritesGroupSendEventShellOmitsAbsentSenderAndGroupTail)
{
    auto event_payload = memochat::chat::command::ToJsonValue(
        memochat::chat::command::ChatGroupSendEventDto{.fromuid = 12,
                                                       .groupid = 3002,
                                                       .trace_id = "trace-3",
                                                       .request_id = "req-3",
                                                       .span_id = "span-3",
                                                       .event_id = "evt-3",
                                                       .accept_node = "chatserver1",
                                                       .accept_ts = 8002});

    event_payload["msg"] = Parse(R"({"msgid":"evt-3","content":"no tail"})");

    const memochat::json::JsonValue out = event_payload;
    EXPECT_EQ(out["fromuid"].asInt(), 12);
    EXPECT_EQ(out["groupid"].asInt64(), 3002);
    EXPECT_EQ(out["event_id"].asString(), "evt-3");
    EXPECT_EQ(out["msg"]["content"].asString(), "no tail");
    EXPECT_FALSE(out.isMember("from_name")) << out.toStyledString();
    EXPECT_FALSE(out.isMember("from_nick")) << out.toStyledString();
    EXPECT_FALSE(out.isMember("from_icon")) << out.toStyledString();
    EXPECT_FALSE(out.isMember("from_user_id")) << out.toStyledString();
    EXPECT_FALSE(out.isMember("group_code")) << out.toStyledString();
}
