#include <gtest/gtest.h>

#include "ChatRelationGroupDtos.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::output::ChatGroupListRowDto>(
    std::array<std::string_view, 11>{"groupid",
                                     "group_code",
                                     "name",
                                     "icon",
                                     "owner_uid",
                                     "announcement",
                                     "member_limit",
                                     "member_count",
                                     "is_all_muted",
                                     "role",
                                     "permission_bits"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::output::ChatPendingGroupApplyRowDto>(
    std::array<std::string_view, 10>{"apply_id",
                                     "groupid",
                                     "group_code",
                                     "applicant_uid",
                                     "inviter_uid",
                                     "applicant_user_id",
                                     "inviter_user_id",
                                     "type",
                                     "status",
                                     "reason"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::output::ChatRelationApplyRowDto>(
    std::array<std::string_view, 9>{"name", "uid", "user_id", "icon", "nick", "sex", "desc", "status", "labels"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::output::ChatRelationFriendRowDto>(
    std::array<std::string_view, 9>{"name", "uid", "user_id", "icon", "nick", "sex", "desc", "back", "labels"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::output::ChatRelationBootstrapDto>(
    std::array<std::string_view, 2>{"apply_list", "friend_list"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::output::ChatDialogRowDto>(
    std::array<std::string_view, 12>{"dialog_id",
                                     "dialog_type",
                                     "peer_uid",
                                     "group_id",
                                     "title",
                                     "avatar",
                                     "last_msg_preview",
                                     "last_msg_ts",
                                     "unread_count",
                                     "pinned_rank",
                                     "draft_text",
                                     "mute_state"}));
#endif

TEST(ChatRelationGroupDtosTest, WritesGroupListRowWithPermissionFlags)
{
    memochat::chat::output::ChatGroupListRowDto row;
    row.groupid = 9001;
    row.group_code = "G9001";
    row.name = "core";
    row.icon = "core.png";
    row.owner_uid = 42;
    row.announcement = "ship";
    row.member_limit = 200;
    row.member_count = 12;
    row.is_all_muted = 1;
    row.role = 2;
    row.permission_bits = memochat::chat::output::kGroupPermChangeGroupInfo |
                          memochat::chat::output::kGroupPermInviteUsers |
                          memochat::chat::output::kGroupPermBanUsers;

    const memochat::json::JsonValue json = memochat::chat::output::ToJsonValue(row);

    ASSERT_TRUE(json.isObject()) << json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(json, "groupid", 0), 9001);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "group_code", ""), "G9001");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "name", ""), "core");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "icon", ""), "core.png");
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(json, "owner_uid", 0), 42);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "announcement", ""), "ship");
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(json, "member_limit", 0), 200);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(json, "member_count", 0), 12);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(json, "is_all_muted", 0), 1);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(json, "role", 0), 2);
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(json, "permission_bits", 0), row.permission_bits);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(json, "can_change_group_info", false));
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(json, "can_delete_messages", true));
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(json, "can_invite_users", false));
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(json, "can_manage_admins", true));
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(json, "can_pin_messages", true));
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(json, "can_ban_users", false));
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(json, "can_manage_topics", true));
}

TEST(ChatRelationGroupDtosTest, WritesPendingGroupApplyOptionalFieldsOnlyWhenPresent)
{
    memochat::chat::output::ChatPendingGroupApplyRowDto row;
    row.apply_id = 77;
    row.groupid = 9001;
    row.applicant_uid = 10;
    row.inviter_uid = 20;
    row.type = "apply";
    row.status = 0;
    row.reason = "join";

    const memochat::json::JsonValue missing = memochat::chat::output::ToJsonValue(row);
    ASSERT_TRUE(missing.isObject()) << missing.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(missing, "apply_id", 0), 77);
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(missing, "groupid", 0), 9001);
    EXPECT_FALSE(missing.isMember("group_code")) << missing.toStyledString();
    EXPECT_FALSE(missing.isMember("applicant_user_id")) << missing.toStyledString();
    EXPECT_FALSE(missing.isMember("inviter_user_id")) << missing.toStyledString();

    row.group_code = "G9001";
    row.applicant_user_id = "alice-id";
    row.inviter_user_id = "bob-id";
    const memochat::json::JsonValue present = memochat::chat::output::ToJsonValue(row);
    ASSERT_TRUE(present.isObject()) << present.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(present, "group_code", ""), "G9001");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(present, "applicant_user_id", ""), "alice-id");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(present, "inviter_user_id", ""), "bob-id");
}

TEST(ChatRelationGroupDtosTest, WritesRelationBootstrapRowsWithLabelsArrays)
{
    memochat::chat::output::ChatRelationApplyRowDto apply;
    apply.name = "Alice";
    apply.uid = 10;
    apply.user_id = "alice-id";
    apply.icon = "alice.png";
    apply.nick = "A";
    apply.sex = 2;
    apply.desc = "hello";
    apply.status = 1;
    apply.labels = {"school", "work"};

    const memochat::json::JsonValue apply_json = memochat::chat::output::ToJsonValue(apply);
    ASSERT_TRUE(apply_json.isObject()) << apply_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(apply_json, "name", ""), "Alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(apply_json, "uid", 0), 10);
    const memochat::json::JsonValue apply_labels = apply_json["labels"];
    ASSERT_TRUE(apply_labels.isArray()) << apply_json.toStyledString();
    ASSERT_EQ(apply_labels.size(), 2U);
    EXPECT_EQ(apply_labels[0].asString(), "school");
    EXPECT_EQ(apply_labels[1].asString(), "work");

    memochat::chat::output::ChatRelationFriendRowDto friend_row;
    friend_row.name = "Bob";
    friend_row.uid = 20;
    friend_row.user_id = "bob-id";
    friend_row.icon = "bob.png";
    friend_row.nick = "B";
    friend_row.sex = 1;
    friend_row.desc = "world";
    friend_row.back = "memo";

    const memochat::json::JsonValue friend_json = memochat::chat::output::ToJsonValue(friend_row);
    ASSERT_TRUE(friend_json.isObject()) << friend_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(friend_json, "back", ""), "memo");
    ASSERT_TRUE(friend_json["labels"].isArray()) << friend_json.toStyledString();
    EXPECT_EQ(friend_json["labels"].size(), 0U);
}

TEST(ChatRelationGroupDtosTest, WritesRelationBootstrapRootWithStableArrays)
{
    memochat::chat::output::ChatRelationApplyRowDto apply;
    apply.name = "Alice";
    apply.uid = 10;
    apply.user_id = "alice-id";
    apply.labels = {"school"};

    memochat::chat::output::ChatRelationFriendRowDto friend_row;
    friend_row.name = "Bob";
    friend_row.uid = 20;
    friend_row.user_id = "bob-id";
    friend_row.labels = {"team", "ops"};

    memochat::chat::output::ChatRelationBootstrapDto bootstrap;
    bootstrap.apply_list.push_back(apply);
    bootstrap.friend_list.push_back(friend_row);

    const memochat::json::JsonValue json = memochat::chat::output::ToJsonValue(bootstrap);
    ASSERT_TRUE(json.isObject()) << json.toStyledString();
    ASSERT_TRUE(json["apply_list"].isArray()) << json.toStyledString();
    ASSERT_TRUE(json["friend_list"].isArray()) << json.toStyledString();
    ASSERT_EQ(json["apply_list"].size(), 1U);
    ASSERT_EQ(json["friend_list"].size(), 1U);
    EXPECT_EQ(json["apply_list"][0]["name"].asString(), "Alice");
    EXPECT_EQ(json["friend_list"][0]["back"].asString(), "");
    const std::string body = memochat::json::glaze_stringify(json);
    EXPECT_NE(body.find(R"("labels":["school"])"), std::string::npos) << body;
    EXPECT_NE(body.find(R"("labels":["team","ops"])"), std::string::npos) << body;

    const memochat::json::JsonValue empty_json =
        memochat::chat::output::ToJsonValue(memochat::chat::output::ChatRelationBootstrapDto{});
    ASSERT_TRUE(empty_json["apply_list"].isArray()) << empty_json.toStyledString();
    ASSERT_TRUE(empty_json["friend_list"].isArray()) << empty_json.toStyledString();
    EXPECT_EQ(empty_json["apply_list"].size(), 0U);
    EXPECT_EQ(empty_json["friend_list"].size(), 0U);
}

TEST(ChatRelationGroupDtosTest, WritesPrivateAndGroupDialogRowsWithExclusiveIds)
{
    memochat::chat::output::ChatDialogRowDto private_row;
    private_row.dialog_id = "u_20";
    private_row.dialog_type = "private";
    private_row.peer_uid = 20;
    private_row.title = "Bob";
    private_row.avatar = "bob.png";
    private_row.last_msg_preview = "hi";
    private_row.last_msg_ts = 123456;
    private_row.unread_count = 3;
    private_row.pinned_rank = 9;
    private_row.draft_text = "draft";
    private_row.mute_state = 1;

    const memochat::json::JsonValue private_json = memochat::chat::output::ToJsonValue(private_row);
    ASSERT_TRUE(private_json.isObject()) << private_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(private_json, "dialog_id", ""), "u_20");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(private_json, "dialog_type", ""), "private");
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(private_json, "peer_uid", 0), 20);
    EXPECT_FALSE(private_json.isMember("group_id")) << private_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(private_json, "last_msg_ts", 0), 123456);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(private_json, "unread_count", 0), 3);

    memochat::chat::output::ChatDialogRowDto group_row;
    group_row.dialog_id = "g_9001";
    group_row.dialog_type = "group";
    group_row.group_id = 9001;
    group_row.title = "Core";
    group_row.avatar = "core.png";
    group_row.last_msg_preview = "ship";
    group_row.last_msg_ts = 223344;
    group_row.unread_count = 5;
    group_row.pinned_rank = 1;
    group_row.draft_text = "later";
    group_row.mute_state = 0;

    const memochat::json::JsonValue group_json = memochat::chat::output::ToJsonValue(group_row);
    ASSERT_TRUE(group_json.isObject()) << group_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(group_json, "dialog_id", ""), "g_9001");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(group_json, "dialog_type", ""), "group");
    EXPECT_FALSE(group_json.isMember("peer_uid")) << group_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(group_json, "group_id", 0), 9001);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(group_json, "title", ""), "Core");
}
