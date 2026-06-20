#include <gtest/gtest.h>

#include "ChatHistoryCommandDtos.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::history::ChatPrivateHistoryRequestDto>(
    std::array<std::string_view, 5>{"uid", "peer_uid", "before_ts", "before_msg_id", "limit"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::history::ChatGroupHistoryRequestDto>(
    std::array<std::string_view, 5>{"uid", "group_id", "before_ts", "before_seq", "limit"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::history::ChatPrivateHistoryResponseDto>(
    std::array<std::string_view, 3>{"error", "peer_uid", "has_more"}));
#endif

TEST(ChatHistoryCommandDtosTest, WritesPrivateHistoryResponseShell)
{
    const memochat::json::JsonValue out = memochat::chat::history::ToJsonValue(
        memochat::chat::history::ChatPrivateHistoryResponseDto{.error = 0, .peer_uid = 20, .has_more = true});
    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["peer_uid"].asInt(), 20);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(out, "has_more", false));
    EXPECT_FALSE(out.isMember("messages")) << out.toStyledString();
}

namespace
{

memochat::json::JsonValue Parse(std::string_view body)
{
    memochat::json::JsonValue root;
    EXPECT_TRUE(memochat::json::glaze_parse(root, body)) << std::string(body);
    return root;
}

} // namespace

TEST(ChatHistoryCommandDtosTest, ReadsPrivateHistoryRequest)
{
    const auto request = memochat::chat::history::ChatPrivateHistoryRequestFromJsonValue(
        Parse(R"({"fromuid":11,"peer_uid":12,"before_ts":1000,"before_msg_id":"m-1","limit":30})"));

    EXPECT_EQ(request.uid, 11);
    EXPECT_EQ(request.peer_uid, 12);
    EXPECT_EQ(request.before_ts, 1000);
    EXPECT_EQ(request.before_msg_id, "m-1");
    EXPECT_EQ(request.limit, 30);
}

TEST(ChatHistoryCommandDtosTest, ReadsPrivateHistoryDefaults)
{
    const auto request =
        memochat::chat::history::ChatPrivateHistoryRequestFromJsonValue(Parse(R"({"fromuid":13,"peer_uid":14})"));

    EXPECT_EQ(request.uid, 13);
    EXPECT_EQ(request.peer_uid, 14);
    EXPECT_EQ(request.before_ts, 0);
    EXPECT_TRUE(request.before_msg_id.empty());
    EXPECT_EQ(request.limit, 20);
}

TEST(ChatHistoryCommandDtosTest, ReadsGroupHistoryRequest)
{
    const auto request = memochat::chat::history::ChatGroupHistoryRequestFromJsonValue(
        Parse(R"({"fromuid":21,"groupid":300,"before_ts":2000,"before_seq":77,"limit":40})"));

    EXPECT_EQ(request.uid, 21);
    EXPECT_EQ(request.group_id, 300);
    EXPECT_EQ(request.before_ts, 2000);
    EXPECT_EQ(request.before_seq, 77);
    EXPECT_EQ(request.limit, 40);
}

TEST(ChatHistoryCommandDtosTest, ReadsGroupHistoryDefaults)
{
    const auto request =
        memochat::chat::history::ChatGroupHistoryRequestFromJsonValue(Parse(R"({"fromuid":22,"groupid":301})"));

    EXPECT_EQ(request.uid, 22);
    EXPECT_EQ(request.group_id, 301);
    EXPECT_EQ(request.before_ts, 0);
    EXPECT_EQ(request.before_seq, 0);
    EXPECT_EQ(request.limit, 20);
}
