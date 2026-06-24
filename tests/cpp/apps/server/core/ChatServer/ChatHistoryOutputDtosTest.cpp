#include <gtest/gtest.h>

#include "ChatHistoryOutputDtos.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::history::output::ChatHistoryDynamicJson>(
    std::array<std::string_view, 2>{"present", "value"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::history::output::ChatPrivateHistoryMessageDto>(
    std::array<std::string_view, 10>{"msgid",
                                     "content",
                                     "fromuid",
                                     "touid",
                                     "from_user_id",
                                     "created_at",
                                     "reply_to_server_msg_id",
                                     "forward_meta",
                                     "edited_at_ms",
                                     "deleted_at_ms"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::history::output::ChatPrivateOfflinePushMessageDto>(
    std::array<std::string_view, 8>{"msgid",
                                    "content",
                                    "from_user_id",
                                    "created_at",
                                    "reply_to_server_msg_id",
                                    "forward_meta",
                                    "edited_at_ms",
                                    "deleted_at_ms"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::history::output::ChatPrivateOfflinePushNotifyDto>(
    std::array<std::string_view, 5>{"error", "fromuid", "touid", "from_user_id", "text_array"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::history::output::ChatGroupHistoryMessageDto>(
    std::array<std::string_view, 19>{"msgid",
                                     "groupid",
                                     "fromuid",
                                     "msgtype",
                                     "content",
                                     "mentions",
                                     "file_name",
                                     "mime",
                                     "size",
                                     "created_at",
                                     "server_msg_id",
                                     "group_seq",
                                     "reply_to_server_msg_id",
                                     "forward_meta",
                                     "edited_at_ms",
                                     "deleted_at_ms",
                                     "from_name",
                                     "from_nick",
                                     "from_icon"}));
#endif

namespace
{

PrivateMessageInfo MakePrivateMessage()
{
    PrivateMessageInfo info;
    info.msg_id = "private-1";
    info.content = "hello";
    info.from_uid = 10;
    info.to_uid = 20;
    info.from_user_id = "u100000010";
    info.created_at = 123456;
    info.reply_to_server_msg_id = 9001;
    info.forward_meta_json = R"({"forwarded_from_msgid":"source-1","depth":2})";
    info.edited_at_ms = 123500;
    info.deleted_at_ms = 123600;
    return info;
}

GroupMessageInfo MakeGroupMessage()
{
    GroupMessageInfo info;
    info.msg_id = "group-1";
    info.group_id = 7001;
    info.from_uid = 30;
    info.msg_type = "image";
    info.content = "group hello";
    info.mentions_json = R"([30,40])";
    info.file_name = "image.png";
    info.mime = "image/png";
    info.size = 2048;
    info.created_at = 223456;
    info.server_msg_id = 9901;
    info.group_seq = 88;
    info.reply_to_server_msg_id = 9900;
    info.forward_meta_json = R"({"source_group_seq":77})";
    info.edited_at_ms = 223500;
    info.deleted_at_ms = 223600;
    info.from_name = "alice";
    info.from_nick = "Alice";
    info.from_icon = "icon.png";
    return info;
}

} // namespace

TEST(ChatHistoryOutputDtosTest, WritesPrivateHistoryMessageWithOptionalDynamicFields)
{
    const auto dto = memochat::chat::history::output::ChatPrivateHistoryMessageFromInfo(MakePrivateMessage());
    const auto json = memochat::chat::history::output::ToJsonValue(dto);

    EXPECT_EQ(json["msgid"].asString(), "private-1");
    EXPECT_EQ(json["content"].asString(), "hello");
    EXPECT_EQ(json["fromuid"].asInt(), 10);
    EXPECT_EQ(json["touid"].asInt(), 20);
    EXPECT_EQ(json["from_user_id"].asString(), "u100000010");
    EXPECT_EQ(json["created_at"].asInt64(), 123456);
    EXPECT_EQ(json["reply_to_server_msg_id"].asInt64(), 9001);
    EXPECT_EQ(json["forward_meta"]["forwarded_from_msgid"].asString(), "source-1");
    EXPECT_EQ(json["forward_meta"]["depth"].asInt(), 2);
    EXPECT_EQ(json["edited_at_ms"].asInt64(), 123500);
    EXPECT_EQ(json["deleted_at_ms"].asInt64(), 123600);
}

TEST(ChatHistoryOutputDtosTest, OmitsPrivateHistoryOptionalFieldsWhenAbsentOrInvalid)
{
    PrivateMessageInfo info;
    info.msg_id = "private-2";
    info.content = "plain";
    info.from_uid = 11;
    info.to_uid = 21;
    info.created_at = 123;
    info.forward_meta_json = "{invalid";

    const auto json = memochat::chat::history::output::ToJsonValue(
        memochat::chat::history::output::ChatPrivateHistoryMessageFromInfo(info));

    EXPECT_EQ(json["msgid"].asString(), "private-2");
    EXPECT_EQ(json["created_at"].asInt64(), 123);
    EXPECT_FALSE(json.isMember("reply_to_server_msg_id")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("forward_meta")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("edited_at_ms")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("deleted_at_ms")) << json.toStyledString();
}

TEST(ChatHistoryOutputDtosTest, PreservesValidNullForwardMetaLikePreviousParser)
{
    PrivateMessageInfo info;
    info.msg_id = "private-3";
    info.content = "null forward";
    info.from_uid = 12;
    info.to_uid = 22;
    info.created_at = 456;
    info.forward_meta_json = "null";

    const auto json = memochat::chat::history::output::ToJsonValue(
        memochat::chat::history::output::ChatPrivateHistoryMessageFromInfo(info));

    ASSERT_TRUE(json.isMember("forward_meta")) << json.toStyledString();
    EXPECT_TRUE(json["forward_meta"].isNull()) << json.toStyledString();
}

TEST(ChatHistoryOutputDtosTest, WritesPrivateOfflinePushMessageWithoutRootIdentityFields)
{
    const auto dto = memochat::chat::history::output::ChatPrivateOfflinePushMessageFromInfo(MakePrivateMessage());
    const auto json = memochat::chat::history::output::ToJsonValue(dto);

    EXPECT_EQ(json["msgid"].asString(), "private-1");
    EXPECT_EQ(json["content"].asString(), "hello");
    EXPECT_EQ(json["from_user_id"].asString(), "u100000010");
    EXPECT_EQ(json["created_at"].asInt64(), 123456);
    EXPECT_EQ(json["reply_to_server_msg_id"].asInt64(), 9001);
    EXPECT_EQ(json["forward_meta"]["forwarded_from_msgid"].asString(), "source-1");
    EXPECT_EQ(json["forward_meta"]["depth"].asInt(), 2);
    EXPECT_EQ(json["edited_at_ms"].asInt64(), 123500);
    EXPECT_EQ(json["deleted_at_ms"].asInt64(), 123600);
    EXPECT_FALSE(json.isMember("fromuid")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("touid")) << json.toStyledString();
}

TEST(ChatHistoryOutputDtosTest, OmitsPrivateOfflinePushOptionalFieldsWhenAbsentOrInvalid)
{
    PrivateMessageInfo info;
    info.msg_id = "offline-2";
    info.content = "plain";
    info.from_uid = 13;
    info.to_uid = 23;
    info.created_at = 789;
    info.forward_meta_json = "{invalid";

    const auto json = memochat::chat::history::output::ToJsonValue(
        memochat::chat::history::output::ChatPrivateOfflinePushMessageFromInfo(info));

    EXPECT_EQ(json["msgid"].asString(), "offline-2");
    EXPECT_EQ(json["content"].asString(), "plain");
    EXPECT_EQ(json["created_at"].asInt64(), 789);
    EXPECT_FALSE(json.isMember("reply_to_server_msg_id")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("forward_meta")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("edited_at_ms")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("deleted_at_ms")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("fromuid")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("touid")) << json.toStyledString();
}

TEST(ChatHistoryOutputDtosTest, PreservesPrivateOfflinePushNullForwardMeta)
{
    PrivateMessageInfo info;
    info.msg_id = "offline-3";
    info.content = "null forward";
    info.from_uid = 14;
    info.to_uid = 24;
    info.created_at = 987;
    info.forward_meta_json = "null";

    const auto json = memochat::chat::history::output::ToJsonValue(
        memochat::chat::history::output::ChatPrivateOfflinePushMessageFromInfo(info));

    ASSERT_TRUE(json.isMember("forward_meta")) << json.toStyledString();
    EXPECT_TRUE(json["forward_meta"].isNull()) << json.toStyledString();
    EXPECT_FALSE(json.isMember("fromuid")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("touid")) << json.toStyledString();
}

TEST(ChatHistoryOutputDtosTest, WritesPrivateOfflinePushNotifyRootWithMessageArray)
{
    memochat::json::JsonValue text_array(memochat::json::array_t{});
    append(text_array,
           memochat::chat::history::output::ToJsonValue(
               memochat::chat::history::output::ChatPrivateOfflinePushMessageFromInfo(MakePrivateMessage())));

    const auto json = memochat::chat::history::output::ToJsonValue(
        memochat::chat::history::output::ChatPrivateOfflinePushNotifyDto{.error = 0,
                                                                         .fromuid = 10,
                                                                         .touid = 20,
                                                                         .from_user_id = "u100000010",
                                                                         .text_array = text_array});

    EXPECT_EQ(json["error"].asInt(), 0);
    EXPECT_EQ(json["fromuid"].asInt(), 10);
    EXPECT_EQ(json["touid"].asInt(), 20);
    EXPECT_EQ(json["from_user_id"].asString(), "u100000010");
    ASSERT_TRUE(json["text_array"].isArray()) << json.toStyledString();
    ASSERT_EQ(json["text_array"].size(), 1);
    EXPECT_EQ(json["text_array"][0]["msgid"].asString(), "private-1");
    EXPECT_EQ(json["text_array"][0]["from_user_id"].asString(), "u100000010");
    EXPECT_FALSE(json["text_array"][0].isMember("fromuid")) << json.toStyledString();
    EXPECT_FALSE(json["text_array"][0].isMember("touid")) << json.toStyledString();
}

TEST(ChatHistoryOutputDtosTest, WritesPrivateOfflinePushNotifyRootWithEmptyArray)
{
    const memochat::json::JsonValue text_array(memochat::json::array_t{});

    const auto json = memochat::chat::history::output::ToJsonValue(
        memochat::chat::history::output::ChatPrivateOfflinePushNotifyDto{.error = 0,
                                                                         .fromuid = 101,
                                                                         .touid = 202,
                                                                         .text_array = text_array});

    EXPECT_EQ(json["error"].asInt(), 0);
    EXPECT_EQ(json["fromuid"].asInt(), 101);
    EXPECT_EQ(json["touid"].asInt(), 202);
    ASSERT_TRUE(json["text_array"].isArray()) << json.toStyledString();
    EXPECT_TRUE(json["text_array"].empty()) << json.toStyledString();
}

TEST(ChatHistoryOutputDtosTest, WritesGroupHistoryMessageWithOptionalDynamicFields)
{
    const auto dto = memochat::chat::history::output::ChatGroupHistoryMessageFromInfo(MakeGroupMessage());
    const auto json = memochat::chat::history::output::ToJsonValue(dto);

    EXPECT_EQ(json["msgid"].asString(), "group-1");
    EXPECT_EQ(json["groupid"].asInt64(), 7001);
    EXPECT_EQ(json["fromuid"].asInt(), 30);
    EXPECT_EQ(json["msgtype"].asString(), "image");
    EXPECT_EQ(json["content"].asString(), "group hello");
    ASSERT_TRUE(json["mentions"].isArray()) << json.toStyledString();
    EXPECT_EQ(json["mentions"][0].asInt(), 30);
    EXPECT_EQ(json["mentions"][1].asInt(), 40);
    EXPECT_EQ(json["file_name"].asString(), "image.png");
    EXPECT_EQ(json["mime"].asString(), "image/png");
    EXPECT_EQ(json["size"].asInt64(), 2048);
    EXPECT_EQ(json["created_at"].asInt64(), 223456);
    EXPECT_EQ(json["server_msg_id"].asInt64(), 9901);
    EXPECT_EQ(json["group_seq"].asInt64(), 88);
    EXPECT_EQ(json["reply_to_server_msg_id"].asInt64(), 9900);
    EXPECT_EQ(json["forward_meta"]["source_group_seq"].asInt(), 77);
    EXPECT_EQ(json["edited_at_ms"].asInt64(), 223500);
    EXPECT_EQ(json["deleted_at_ms"].asInt64(), 223600);
    EXPECT_EQ(json["from_name"].asString(), "alice");
    EXPECT_EQ(json["from_nick"].asString(), "Alice");
    EXPECT_EQ(json["from_icon"].asString(), "icon.png");
}

TEST(ChatHistoryOutputDtosTest, DefaultsGroupMentionsAndOmitsInvalidOptionalFields)
{
    GroupMessageInfo info;
    info.msg_id = "group-2";
    info.group_id = 7002;
    info.from_uid = 31;
    info.msg_type = "text";
    info.content = "plain group";
    info.mentions_json = R"({"not":"array"})";
    info.forward_meta_json = "{invalid";
    info.created_at = 777;
    info.server_msg_id = 9902;
    info.group_seq = 89;

    const auto json = memochat::chat::history::output::ToJsonValue(
        memochat::chat::history::output::ChatGroupHistoryMessageFromInfo(info));

    ASSERT_TRUE(json["mentions"].isArray()) << json.toStyledString();
    EXPECT_TRUE(json["mentions"].empty()) << json.toStyledString();
    EXPECT_FALSE(json.isMember("file_name")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("mime")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("size")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("reply_to_server_msg_id")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("forward_meta")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("edited_at_ms")) << json.toStyledString();
    EXPECT_FALSE(json.isMember("deleted_at_ms")) << json.toStyledString();
    EXPECT_TRUE(json.isMember("from_name")) << json.toStyledString();
    EXPECT_TRUE(json.isMember("from_nick")) << json.toStyledString();
    EXPECT_TRUE(json.isMember("from_icon")) << json.toStyledString();
}
