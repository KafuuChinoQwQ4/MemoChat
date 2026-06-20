#include <gtest/gtest.h>

#include "ChatRelationCommandDtos.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatSearchUserRequestDto>(
    std::array<std::string_view, 1>{"user_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatSearchUserResponseDto>(
    std::array<std::string_view, 9>{"error", "uid", "user_id", "name", "email", "nick", "desc", "sex", "icon"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatFilterFriendUidsRequestDto>(
    std::array<std::string_view, 2>{"viewer_uid", "author_uids"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatFilterFriendUidsResponseDto>(
    std::array<std::string_view, 2>{"error", "friend_uids"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatAddFriendApplyRequestDto>(
    std::array<std::string_view, 4>{"uid", "applyname", "touid", "labels"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatAddFriendApplyResponseDto>(
    std::array<std::string_view, 1>{"error"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatAuthFriendApplyRequestDto>(
    std::array<std::string_view, 4>{"fromuid", "touid", "back", "labels"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatAuthFriendApplyResponseDto>(
    std::array<std::string_view, 7>{"error", "name", "nick", "icon", "sex", "uid", "user_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatDeleteFriendRequestDto>(
    std::array<std::string_view, 2>{"uid", "friend_uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatDeleteFriendResponseDto>(
    std::array<std::string_view, 3>{"error", "fromuid", "friend_uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatDialogListRequestDto>(
    std::array<std::string_view, 1>{"uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatDialogListResponseDto>(
    std::array<std::string_view, 3>{"error", "uid", "dialogs"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatSyncDraftRequestDto>(
    std::array<std::string_view, 7>{"uid",
                                    "dialog_type",
                                    "peer_uid",
                                    "group_id",
                                    "draft_text",
                                    "has_mute_state",
                                    "mute_state"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatSyncDraftResponseDto>(
    std::array<std::string_view, 7>{"error",
                                    "uid",
                                    "dialog_type",
                                    "peer_uid",
                                    "draft_text",
                                    "mute_state",
                                    "group_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatPinDialogRequestDto>(
    std::array<std::string_view, 5>{"uid", "dialog_type", "peer_uid", "group_id", "pinned_rank"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatPinDialogResponseDto>(
    std::array<std::string_view, 6>{"error", "uid", "dialog_type", "peer_uid", "pinned_rank", "group_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatAddFriendApplyNotifyDto>(
    std::array<std::string_view, 8>{"error", "applyuid", "name", "desc", "icon", "sex", "nick", "user_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatAuthFriendApplyNotifyDto>(
    std::array<std::string_view, 8>{"error", "fromuid", "touid", "name", "nick", "icon", "sex", "user_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::relation::ChatRelationStateEventDto>(
    std::array<std::string_view, 6>{"event_type", "uid", "touid", "uids", "event_ts", "labels"}));
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

TEST(ChatRelationCommandDtosTest, ReadsSearchUserRequest)
{
    const auto request =
        memochat::chat::relation::ChatSearchUserRequestFromJsonValue(Parse(R"({"user_id":"alice-id"})"));

    EXPECT_EQ(request.user_id, "alice-id");
}

TEST(ChatRelationCommandDtosTest, WritesSearchUserResponsePayload)
{
    const memochat::chat::relation::ChatSearchUserResponseDto failure{.error = 1001};
    const auto failure_json = memochat::chat::relation::ToJsonValue(failure);
    EXPECT_EQ(failure_json["error"].asInt(), 1001);
    EXPECT_FALSE(failure_json.isMember("uid"));
    EXPECT_FALSE(failure_json.isMember("user_id"));
    EXPECT_FALSE(failure_json.isMember("icon"));

    const memochat::chat::relation::ChatSearchUserResponseDto success{
        .error = 0,
        .uid = std::optional<int>{42},
        .user_id = std::optional<std::string>{"alice-id"},
        .name = std::optional<std::string>{"Alice"},
        .email = std::optional<std::string>{"alice@example.test"},
        .nick = std::optional<std::string>{"Ali"},
        .desc = std::optional<std::string>{"backend"},
        .sex = std::optional<int>{2},
        .icon = std::optional<std::string>{"alice.png"}};
    const auto success_json = memochat::chat::relation::ToJsonValue(success);

    EXPECT_EQ(success_json["error"].asInt(), 0);
    EXPECT_EQ(success_json["uid"].asInt(), 42);
    EXPECT_EQ(success_json["user_id"].asString(), "alice-id");
    EXPECT_EQ(success_json["name"].asString(), "Alice");
    EXPECT_EQ(success_json["email"].asString(), "alice@example.test");
    EXPECT_EQ(success_json["nick"].asString(), "Ali");
    EXPECT_EQ(success_json["desc"].asString(), "backend");
    EXPECT_EQ(success_json["sex"].asInt(), 2);
    EXPECT_EQ(success_json["icon"].asString(), "alice.png");
    EXPECT_FALSE(success_json.isMember("pwd"));
}

TEST(ChatRelationCommandDtosTest, ReadsSearchUserResponseFromProfileRoot)
{
    memochat::json::JsonValue root(memochat::json::object_t{});
    root["error"] = 0;
    root["uid"] = 42;
    root["user_id"] = "alice-id";
    root["name"] = "Alice";
    root["email"] = "alice@example.test";
    root["nick"] = "Ali";
    root["desc"] = "backend";
    root["sex"] = 2;
    root["icon"] = "alice.png";

    const auto response = memochat::chat::relation::ChatSearchUserResponseFromJsonValue(root);

    EXPECT_EQ(response.error, 0);
    ASSERT_TRUE(response.uid.has_value());
    EXPECT_EQ(*response.uid, 42);
    ASSERT_TRUE(response.user_id.has_value());
    EXPECT_EQ(*response.user_id, "alice-id");
    ASSERT_TRUE(response.icon.has_value());
    EXPECT_EQ(*response.icon, "alice.png");

    const auto failure = memochat::chat::relation::ChatSearchUserResponseFromJsonValue(Parse(R"({"error":1001})"));
    EXPECT_EQ(failure.error, 1001);
    EXPECT_FALSE(failure.uid.has_value());
    EXPECT_FALSE(failure.user_id.has_value());
}

TEST(ChatRelationCommandDtosTest, FiltersAuthorUidsToPositiveIntegers)
{
    const auto request = memochat::chat::relation::ChatFilterFriendUidsRequestFromJsonValue(
        Parse(R"({"viewer_uid":7,"author_uids":[1,0,-2,3]})"));

    EXPECT_EQ(request.viewer_uid, 7);
    ASSERT_EQ(request.author_uids.size(), 2U);
    EXPECT_EQ(request.author_uids[0], 1);
    EXPECT_EQ(request.author_uids[1], 3);
}

TEST(ChatRelationCommandDtosTest, ReadsAddFriendApplyRequestAndLabels)
{
    const auto request = memochat::chat::relation::ChatAddFriendApplyRequestFromJsonValue(
        Parse(R"({"uid":11,"applyname":"Alice","touid":22,"labels":["classmate","backend"]})"));

    EXPECT_EQ(request.uid, 11);
    EXPECT_EQ(request.applyname, "Alice");
    EXPECT_EQ(request.touid, 22);
    ASSERT_EQ(request.labels.size(), 2U);
    EXPECT_EQ(request.labels[0], "classmate");
    EXPECT_EQ(request.labels[1], "backend");

    const auto without_labels = memochat::chat::relation::ChatAddFriendApplyRequestFromJsonValue(
        Parse(R"({"uid":12,"applyname":"Bob","touid":23})"));
    EXPECT_TRUE(without_labels.labels.empty());
}

TEST(ChatRelationCommandDtosTest, ReadsAuthFriendApplyRequestAndLabels)
{
    const auto request = memochat::chat::relation::ChatAuthFriendApplyRequestFromJsonValue(
        Parse(R"({"fromuid":31,"touid":42,"back":"Bobby","labels":["team","ops"]})"));

    EXPECT_EQ(request.fromuid, 31);
    EXPECT_EQ(request.touid, 42);
    EXPECT_EQ(request.back, "Bobby");
    ASSERT_EQ(request.labels.size(), 2U);
    EXPECT_EQ(request.labels[0], "team");
    EXPECT_EQ(request.labels[1], "ops");

    const auto without_labels = memochat::chat::relation::ChatAuthFriendApplyRequestFromJsonValue(
        Parse(R"({"fromuid":32,"touid":43,"back":""})"));
    EXPECT_TRUE(without_labels.labels.empty());
}

TEST(ChatRelationCommandDtosTest, PreservesRelationAliasPrecedence)
{
    const auto deleted = memochat::chat::relation::ChatDeleteFriendRequestFromJsonValue(
        Parse(R"({"uid":1,"fromuid":2,"touid":3,"friend_uid":4})"));
    EXPECT_EQ(deleted.uid, 2);
    EXPECT_EQ(deleted.friend_uid, 4);

    const auto dialog =
        memochat::chat::relation::ChatDialogListRequestFromJsonValue(Parse(R"({"uid":1,"fromuid":9})"));
    EXPECT_EQ(dialog.uid, 9);
}

TEST(ChatRelationCommandDtosTest, WritesFilterFriendUidsResponsePayload)
{
    const memochat::chat::relation::ChatFilterFriendUidsResponseDto response{.error = 0, .friend_uids = {3, 5}};
    const auto response_json = memochat::chat::relation::ToJsonValue(response);

    EXPECT_EQ(response_json["error"].asInt(), 0);
    ASSERT_TRUE(response_json["friend_uids"].isArray());
    EXPECT_EQ(response_json["friend_uids"][0].asInt(), 3);
    EXPECT_EQ(response_json["friend_uids"][1].asInt(), 5);

    const memochat::chat::relation::ChatFilterFriendUidsResponseDto empty{.error = 0};
    const auto empty_json = memochat::chat::relation::ToJsonValue(empty);
    EXPECT_EQ(empty_json["error"].asInt(), 0);
    EXPECT_TRUE(empty_json["friend_uids"].isArray());
    EXPECT_EQ(empty_json["friend_uids"].size(), 0);
}

TEST(ChatRelationCommandDtosTest, WritesDeleteFriendResponsePayload)
{
    const memochat::chat::relation::ChatDeleteFriendResponseDto response{.error = 0, .fromuid = 11, .friend_uid = 22};
    const auto response_json = memochat::chat::relation::ToJsonValue(response);

    EXPECT_EQ(response_json["error"].asInt(), 0);
    EXPECT_EQ(response_json["fromuid"].asInt(), 11);
    EXPECT_EQ(response_json["friend_uid"].asInt(), 22);
}

TEST(ChatRelationCommandDtosTest, WritesDialogListResponsePayload)
{
    const memochat::chat::relation::ChatDialogListResponseDto invalid{.error = 1001, .uid = 0};
    const auto invalid_json = memochat::chat::relation::ToJsonValue(invalid);
    EXPECT_EQ(invalid_json["error"].asInt(), 1001);
    EXPECT_EQ(invalid_json["uid"].asInt(), 0);
    EXPECT_FALSE(invalid_json.isMember("dialogs"));

    memochat::json::JsonValue empty_dialogs(memochat::json::array_t{});
    const memochat::chat::relation::ChatDialogListResponseDto empty{.error = 0,
                                                                    .uid = 7,
                                                                    .dialogs = std::optional<memochat::json::JsonValue>{
                                                                        empty_dialogs}};
    const auto empty_json = memochat::chat::relation::ToJsonValue(empty);
    EXPECT_EQ(empty_json["error"].asInt(), 0);
    EXPECT_EQ(empty_json["uid"].asInt(), 7);
    ASSERT_TRUE(empty_json["dialogs"].isArray());
    EXPECT_EQ(empty_json["dialogs"].size(), 0);

    memochat::json::JsonValue dialogs(memochat::json::array_t{});
    memochat::json::JsonValue dialog(memochat::json::object_t{});
    dialog["dialog_id"] = "u_42";
    dialog["dialog_type"] = "private";
    dialogs.append(dialog);
    const memochat::chat::relation::ChatDialogListResponseDto response{
        .error = 0, .uid = 8, .dialogs = std::optional<memochat::json::JsonValue>{dialogs}};
    const auto response_json = memochat::chat::relation::ToJsonValue(response);
    EXPECT_EQ(response_json["error"].asInt(), 0);
    EXPECT_EQ(response_json["uid"].asInt(), 8);
    ASSERT_TRUE(response_json["dialogs"].isArray());
    ASSERT_EQ(response_json["dialogs"].size(), 1);
    EXPECT_EQ(response_json["dialogs"][0]["dialog_id"].asString(), "u_42");
    EXPECT_EQ(response_json["dialogs"][0]["dialog_type"].asString(), "private");
}

TEST(ChatRelationCommandDtosTest, ReadsSyncDraftAliasesPresenceAndDraftClamp)
{
    const std::string long_draft(2105, 'x');
    memochat::json::JsonValue root(memochat::json::object_t{});
    root["uid"] = 1;
    root["fromuid"] = 2;
    root["dialog_type"] = "group";
    root["peer_uid"] = 3;
    root["group_id"] = static_cast<int64_t>(400);
    root["groupid"] = static_cast<int64_t>(500);
    root["draft_text"] = long_draft;
    root["mute_state"] = 3;

    const auto request = memochat::chat::relation::ChatSyncDraftRequestFromJsonValue(root);

    EXPECT_EQ(request.uid, 2);
    EXPECT_EQ(request.dialog_type, "group");
    EXPECT_EQ(request.peer_uid, 3);
    EXPECT_EQ(request.group_id, 500);
    EXPECT_EQ(request.draft_text.size(), 2000U);
    EXPECT_TRUE(request.has_mute_state);
    EXPECT_EQ(request.mute_state, 3);
}

TEST(ChatRelationCommandDtosTest, ReadsPinDialogAliasesAndClampsRank)
{
    const auto negative = memochat::chat::relation::ChatPinDialogRequestFromJsonValue(
        Parse(R"({"uid":1,"dialog_type":"private","peer_uid":8,"pinned_rank":-5})"));
    EXPECT_EQ(negative.uid, 1);
    EXPECT_EQ(negative.peer_uid, 8);
    EXPECT_EQ(negative.pinned_rank, 0);

    const auto huge = memochat::chat::relation::ChatPinDialogRequestFromJsonValue(
        Parse(R"({"fromuid":2,"dialog_type":"group","group_id":40,"groupid":50,"pinned_rank":2000000001})"));
    EXPECT_EQ(huge.uid, 2);
    EXPECT_EQ(huge.dialog_type, "group");
    EXPECT_EQ(huge.group_id, 50);
    EXPECT_EQ(huge.pinned_rank, 1000000000);
}

TEST(ChatRelationCommandDtosTest, WritesSyncDraftResponsePayload)
{
    const memochat::chat::relation::ChatSyncDraftResponseDto private_response{.error = 0,
                                                                              .uid = 7,
                                                                              .dialog_type = "private",
                                                                              .peer_uid = 8,
                                                                              .draft_text = "draft"};
    const auto private_json = memochat::chat::relation::ToJsonValue(private_response);

    EXPECT_EQ(private_json["error"].asInt(), 0);
    EXPECT_EQ(private_json["uid"].asInt(), 7);
    EXPECT_EQ(private_json["dialog_type"].asString(), "private");
    EXPECT_EQ(private_json["peer_uid"].asInt(), 8);
    EXPECT_EQ(private_json["draft_text"].asString(), "draft");
    EXPECT_FALSE(private_json.isMember("mute_state"));
    EXPECT_FALSE(private_json.isMember("group_id"));

    const memochat::chat::relation::ChatSyncDraftResponseDto group_response{
        .error = 0,
        .uid = 9,
        .dialog_type = "group",
        .peer_uid = 0,
        .draft_text = "group-draft",
        .mute_state = std::optional<int>{1},
        .group_id = std::optional<int64_t>{99}};
    const auto group_json = memochat::chat::relation::ToJsonValue(group_response);

    EXPECT_EQ(group_json["error"].asInt(), 0);
    EXPECT_EQ(group_json["uid"].asInt(), 9);
    EXPECT_EQ(group_json["dialog_type"].asString(), "group");
    EXPECT_EQ(group_json["peer_uid"].asInt(), 0);
    EXPECT_EQ(group_json["draft_text"].asString(), "group-draft");
    EXPECT_EQ(group_json["mute_state"].asInt(), 1);
    EXPECT_EQ(group_json["group_id"].asInt64(), 99);
}

TEST(ChatRelationCommandDtosTest, WritesPinDialogResponsePayload)
{
    const memochat::chat::relation::ChatPinDialogResponseDto private_response{.error = 0,
                                                                              .uid = 7,
                                                                              .dialog_type = "private",
                                                                              .peer_uid = 8,
                                                                              .pinned_rank = 4};
    const auto private_json = memochat::chat::relation::ToJsonValue(private_response);

    EXPECT_EQ(private_json["error"].asInt(), 0);
    EXPECT_EQ(private_json["uid"].asInt(), 7);
    EXPECT_EQ(private_json["dialog_type"].asString(), "private");
    EXPECT_EQ(private_json["peer_uid"].asInt(), 8);
    EXPECT_EQ(private_json["pinned_rank"].asInt(), 4);
    EXPECT_FALSE(private_json.isMember("group_id"));

    const memochat::chat::relation::ChatPinDialogResponseDto group_response{.error = 0,
                                                                            .uid = 9,
                                                                            .dialog_type = "group",
                                                                            .peer_uid = 0,
                                                                            .pinned_rank = 5,
                                                                            .group_id = std::optional<int64_t>{99}};
    const auto group_json = memochat::chat::relation::ToJsonValue(group_response);

    EXPECT_EQ(group_json["error"].asInt(), 0);
    EXPECT_EQ(group_json["uid"].asInt(), 9);
    EXPECT_EQ(group_json["dialog_type"].asString(), "group");
    EXPECT_EQ(group_json["peer_uid"].asInt(), 0);
    EXPECT_EQ(group_json["pinned_rank"].asInt(), 5);
    EXPECT_EQ(group_json["group_id"].asInt64(), 99);
}

TEST(ChatRelationCommandDtosTest, WritesAddFriendApplyResponsePayload)
{
    const memochat::chat::relation::ChatAddFriendApplyResponseDto response{.error = 0};
    const auto response_json = memochat::chat::relation::ToJsonValue(response);

    EXPECT_EQ(response_json["error"].asInt(), 0);
}

TEST(ChatRelationCommandDtosTest, WritesAuthFriendApplyResponsePayload)
{
    const memochat::chat::relation::ChatAuthFriendApplyResponseDto response{
        .error = 0,
        .name = std::optional<std::string>{"Alice"},
        .nick = std::optional<std::string>{"Ali"},
        .icon = std::optional<std::string>{"alice.png"},
        .sex = std::optional<int>{2},
        .uid = std::optional<int>{42},
        .user_id = std::optional<std::string>{"alice-id"}};
    const auto response_json = memochat::chat::relation::ToJsonValue(response);

    EXPECT_EQ(response_json["error"].asInt(), 0);
    EXPECT_EQ(response_json["name"].asString(), "Alice");
    EXPECT_EQ(response_json["nick"].asString(), "Ali");
    EXPECT_EQ(response_json["icon"].asString(), "alice.png");
    EXPECT_EQ(response_json["sex"].asInt(), 2);
    EXPECT_EQ(response_json["uid"].asInt(), 42);
    EXPECT_EQ(response_json["user_id"].asString(), "alice-id");

    const memochat::chat::relation::ChatAuthFriendApplyResponseDto missing_profile{
        .error = 1001};
    const auto missing_profile_json = memochat::chat::relation::ToJsonValue(missing_profile);
    EXPECT_EQ(missing_profile_json["error"].asInt(), 1001);
    EXPECT_FALSE(missing_profile_json.isMember("name"));
    EXPECT_FALSE(missing_profile_json.isMember("nick"));
    EXPECT_FALSE(missing_profile_json.isMember("icon"));
    EXPECT_FALSE(missing_profile_json.isMember("sex"));
    EXPECT_FALSE(missing_profile_json.isMember("uid"));
    EXPECT_FALSE(missing_profile_json.isMember("user_id"));
}

TEST(ChatRelationCommandDtosTest, WritesAddFriendApplyNotifyPayload)
{
    const memochat::chat::relation::ChatAddFriendApplyNotifyDto notify{
        .error = 0,
        .applyuid = 11,
        .name = "Alice",
        .desc = "",
        .icon = std::optional<std::string>{"alice.png"},
        .sex = std::optional<int>{1},
        .nick = std::optional<std::string>{"Ali"},
        .user_id = std::optional<std::string>{"alice-id"}};
    const auto notify_json = memochat::chat::relation::ToJsonValue(notify);

    EXPECT_EQ(notify_json["error"].asInt(), 0);
    EXPECT_EQ(notify_json["applyuid"].asInt(), 11);
    EXPECT_EQ(notify_json["name"].asString(), "Alice");
    EXPECT_EQ(notify_json["desc"].asString(), "");
    EXPECT_EQ(notify_json["icon"].asString(), "alice.png");
    EXPECT_EQ(notify_json["sex"].asInt(), 1);
    EXPECT_EQ(notify_json["nick"].asString(), "Ali");
    EXPECT_EQ(notify_json["user_id"].asString(), "alice-id");

    const memochat::chat::relation::ChatAddFriendApplyNotifyDto without_profile{
        .error = 0, .applyuid = 12, .name = "Bob", .desc = ""};
    const auto without_profile_json = memochat::chat::relation::ToJsonValue(without_profile);
    EXPECT_EQ(without_profile_json["applyuid"].asInt(), 12);
    EXPECT_FALSE(without_profile_json.isMember("icon"));
    EXPECT_FALSE(without_profile_json.isMember("sex"));
    EXPECT_FALSE(without_profile_json.isMember("nick"));
    EXPECT_FALSE(without_profile_json.isMember("user_id"));
}

TEST(ChatRelationCommandDtosTest, WritesAuthFriendApplyNotifyPayload)
{
    const memochat::chat::relation::ChatAuthFriendApplyNotifyDto notify{
        .error = 0,
        .fromuid = 31,
        .touid = 42,
        .name = std::optional<std::string>{"Alice"},
        .nick = std::optional<std::string>{"Ali"},
        .icon = std::optional<std::string>{"alice.png"},
        .sex = std::optional<int>{2},
        .user_id = std::optional<std::string>{"alice-id"}};
    const auto notify_json = memochat::chat::relation::ToJsonValue(notify);

    EXPECT_EQ(notify_json["error"].asInt(), 0);
    EXPECT_EQ(notify_json["fromuid"].asInt(), 31);
    EXPECT_EQ(notify_json["touid"].asInt(), 42);
    EXPECT_EQ(notify_json["name"].asString(), "Alice");
    EXPECT_EQ(notify_json["nick"].asString(), "Ali");
    EXPECT_EQ(notify_json["icon"].asString(), "alice.png");
    EXPECT_EQ(notify_json["sex"].asInt(), 2);
    EXPECT_EQ(notify_json["user_id"].asString(), "alice-id");

    const memochat::chat::relation::ChatAuthFriendApplyNotifyDto missing_profile{
        .error = 1001, .fromuid = 32, .touid = 43};
    const auto missing_profile_json = memochat::chat::relation::ToJsonValue(missing_profile);
    EXPECT_EQ(missing_profile_json["error"].asInt(), 1001);
    EXPECT_EQ(missing_profile_json["fromuid"].asInt(), 32);
    EXPECT_EQ(missing_profile_json["touid"].asInt(), 43);
    EXPECT_FALSE(missing_profile_json.isMember("name"));
    EXPECT_FALSE(missing_profile_json.isMember("nick"));
    EXPECT_FALSE(missing_profile_json.isMember("icon"));
    EXPECT_FALSE(missing_profile_json.isMember("sex"));
    EXPECT_FALSE(missing_profile_json.isMember("user_id"));
}

TEST(ChatRelationCommandDtosTest, WritesRelationStateEventPayload)
{
    const memochat::chat::relation::ChatRelationStateEventDto event{.event_type = "friend_apply_created",
                                                                    .uid = 31,
                                                                    .touid = 42,
                                                                    .uids = {31, 42},
                                                                    .event_ts = 9001,
                                                                    .labels = {"team", "ops"}};
    const auto event_json = memochat::chat::relation::ToJsonValue(event);

    EXPECT_EQ(event_json["event_type"].asString(), "friend_apply_created");
    EXPECT_EQ(event_json["uid"].asInt(), 31);
    EXPECT_EQ(event_json["touid"].asInt(), 42);
    ASSERT_TRUE(event_json["uids"].isArray());
    EXPECT_EQ(event_json["uids"][0].asInt(), 31);
    EXPECT_EQ(event_json["uids"][1].asInt(), 42);
    EXPECT_EQ(event_json["event_ts"].asInt64(), 9001);
    ASSERT_TRUE(event_json["labels"].isArray());
    EXPECT_EQ(event_json["labels"][0].asString(), "team");
    EXPECT_EQ(event_json["labels"][1].asString(), "ops");
}

TEST(ChatRelationCommandDtosTest, OmitsEmptyRelationStateEventLabels)
{
    const memochat::chat::relation::ChatRelationStateEventDto event{.event_type = "friend_apply_approved",
                                                                    .uid = 32,
                                                                    .touid = 43,
                                                                    .uids = {32, 43},
                                                                    .event_ts = 9002};
    const auto event_json = memochat::chat::relation::ToJsonValue(event);

    EXPECT_EQ(event_json["event_type"].asString(), "friend_apply_approved");
    ASSERT_TRUE(event_json["uids"].isArray());
    EXPECT_EQ(event_json["uids"][0].asInt(), 32);
    EXPECT_EQ(event_json["uids"][1].asInt(), 43);
    EXPECT_FALSE(event_json.isMember("labels"));
}
