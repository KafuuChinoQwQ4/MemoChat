#include <gtest/gtest.h>

#include "ChatGroupCommandDtos.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupCreateRequestDto>(
    std::array<std::string_view, 5>{"owner_uid", "name", "announcement", "member_limit", "member_user_ids"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupListRequestDto>(
    std::array<std::string_view, 1>{"uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupInviteMemberRequestDto>(
    std::array<std::string_view, 4>{"from_uid", "target_user_id", "group_id", "reason"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupApplyJoinRequestDto>(
    std::array<std::string_view, 3>{"from_uid", "group_code", "reason"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupReviewApplyRequestDto>(
    std::array<std::string_view, 3>{"reviewer_uid", "apply_id", "agree"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupReadAckRequestDto>(
    std::array<std::string_view, 3>{"uid", "group_id", "read_ts"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupAnnouncementUpdateRequestDto>(
    std::array<std::string_view, 3>{"uid", "group_id", "announcement"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupIconUpdateRequestDto>(
    std::array<std::string_view, 3>{"uid", "group_id", "icon"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupIdRequestDto>(
    std::array<std::string_view, 2>{"uid", "group_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupSetAdminRequestDto>(
    std::array<std::string_view, 7>{"uid",
                                    "target_user_id",
                                    "group_id",
                                    "is_admin",
                                    "has_permission_bits",
                                    "permission_bits",
                                    "requested_permission_bits"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupMemberActionRequestDto>(
    std::array<std::string_view, 4>{"uid", "target_user_id", "group_id", "mute_seconds"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupInviteMemberResponseDto>(
    std::array<std::string_view, 4>{"error", "groupid", "touid", "target_user_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupApplyJoinResponseDto>(
    std::array<std::string_view, 3>{"error", "groupid", "group_code"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupAnnouncementUpdateResponseDto>(
    std::array<std::string_view, 4>{"error", "groupid", "announcement", "group_code"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupIconUpdateResponseDto>(
    std::array<std::string_view, 4>{"error", "groupid", "icon", "group_code"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupQuitResponseDto>(
    std::array<std::string_view, 3>{"error", "groupid", "group_code"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupDissolveResponseDto>(
    std::array<std::string_view, 4>{"error", "groupid", "group_code", "name"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupMuteMemberResponseDto>(
    std::array<std::string_view, 6>{"error", "groupid", "touid", "target_user_id", "mute_until", "group_code"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupKickMemberResponseDto>(
    std::array<std::string_view, 5>{"error", "groupid", "touid", "target_user_id", "group_code"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupSetAdminResponseDto>(
    std::array<std::string_view, 6>{"error", "groupid", "touid", "target_user_id", "is_admin", "permission_bits"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupListResponseDto>(
    std::array<std::string_view, 1>{"error"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupHistoryResponseDto>(
    std::array<std::string_view, 4>{"error", "groupid", "has_more", "next_before_seq"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupCreateResponseDto>(
    std::array<std::string_view, 5>{"error", "groupid", "group_code", "name", "announcement"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::chat::group::ChatGroupReviewApplyResponseDto>(
    std::array<std::string_view, 7>{
        "error", "apply_id", "agree", "groupid", "applicant_uid", "group_code", "applicant_user_id"}));
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

TEST(ChatGroupCommandDtosTest, ReadsCreateGroupRequest)
{
    const auto request = memochat::chat::group::ChatGroupCreateRequestFromJsonValue(
        Parse(R"({"fromuid":10,"name":"team","announcement":"hello","member_limit":128,"member_user_ids":["u1","u2","u1"]})"));

    EXPECT_EQ(request.owner_uid, 10);
    EXPECT_EQ(request.name, "team");
    EXPECT_EQ(request.announcement, "hello");
    EXPECT_EQ(request.member_limit, 128);
    ASSERT_EQ(request.member_user_ids.size(), 3U);
    EXPECT_EQ(request.member_user_ids[0], "u1");
    EXPECT_EQ(request.member_user_ids[1], "u2");
    EXPECT_EQ(request.member_user_ids[2], "u1");
}

TEST(ChatGroupCommandDtosTest, ReadsCreateGroupDefaultsAndIgnoresNonArrayMembers)
{
    const auto missing = memochat::chat::group::ChatGroupCreateRequestFromJsonValue(
        Parse(R"({"fromuid":11,"name":"team"})"));
    EXPECT_EQ(missing.owner_uid, 11);
    EXPECT_EQ(missing.name, "team");
    EXPECT_TRUE(missing.announcement.empty());
    EXPECT_EQ(missing.member_limit, 200);
    EXPECT_TRUE(missing.member_user_ids.empty());

    const auto non_array = memochat::chat::group::ChatGroupCreateRequestFromJsonValue(
        Parse(R"({"fromuid":12,"name":"team","member_user_ids":"u1"})"));
    EXPECT_TRUE(non_array.member_user_ids.empty());
}

TEST(ChatGroupCommandDtosTest, ReadsGroupListRequest)
{
    const auto request =
        memochat::chat::group::ChatGroupListRequestFromJsonValue(Parse(R"({"fromuid":11})"));

    EXPECT_EQ(request.uid, 11);
}

TEST(ChatGroupCommandDtosTest, ReadsInviteMemberRequest)
{
    const auto request = memochat::chat::group::ChatGroupInviteMemberRequestFromJsonValue(
        Parse(R"({"fromuid":12,"target_user_id":"bob","groupid":300,"reason":"join"})"));

    EXPECT_EQ(request.from_uid, 12);
    EXPECT_EQ(request.target_user_id, "bob");
    EXPECT_EQ(request.group_id, 300);
    EXPECT_EQ(request.reason, "join");
}

TEST(ChatGroupCommandDtosTest, ReadsApplyJoinRequest)
{
    const auto request = memochat::chat::group::ChatGroupApplyJoinRequestFromJsonValue(
        Parse(R"({"fromuid":13,"group_code":"G-1","reason":"hello"})"));

    EXPECT_EQ(request.from_uid, 13);
    EXPECT_EQ(request.group_code, "G-1");
    EXPECT_EQ(request.reason, "hello");
}

TEST(ChatGroupCommandDtosTest, ReadsReviewApplyRequestAndAgreeDefault)
{
    const auto missing =
        memochat::chat::group::ChatGroupReviewApplyRequestFromJsonValue(Parse(R"({"fromuid":14,"apply_id":400})"));
    EXPECT_EQ(missing.reviewer_uid, 14);
    EXPECT_EQ(missing.apply_id, 400);
    EXPECT_FALSE(missing.agree);

    const auto accepted = memochat::chat::group::ChatGroupReviewApplyRequestFromJsonValue(
        Parse(R"({"fromuid":15,"apply_id":401,"agree":true})"));
    EXPECT_EQ(accepted.reviewer_uid, 15);
    EXPECT_EQ(accepted.apply_id, 401);
    EXPECT_TRUE(accepted.agree);
}

TEST(ChatGroupCommandDtosTest, ReadsReadAckAliases)
{
    const auto uid_fallback = memochat::chat::group::ChatGroupReadAckRequestFromJsonValue(
        Parse(R"({"uid":16,"groupid":500,"read_ts":600})"));
    EXPECT_EQ(uid_fallback.uid, 16);
    EXPECT_EQ(uid_fallback.group_id, 500);
    EXPECT_EQ(uid_fallback.read_ts, 600);

    const auto fromuid_preferred = memochat::chat::group::ChatGroupReadAckRequestFromJsonValue(
        Parse(R"({"uid":16,"fromuid":17,"groupid":501,"read_ts":601})"));
    EXPECT_EQ(fromuid_preferred.uid, 17);
    EXPECT_EQ(fromuid_preferred.group_id, 501);
    EXPECT_EQ(fromuid_preferred.read_ts, 601);
}

TEST(ChatGroupCommandDtosTest, ReadsAnnouncementAndIconRequests)
{
    const auto announcement = memochat::chat::group::ChatGroupAnnouncementUpdateRequestFromJsonValue(
        Parse(R"({"fromuid":18,"groupid":700,"announcement":"new"})"));
    EXPECT_EQ(announcement.uid, 18);
    EXPECT_EQ(announcement.group_id, 700);
    EXPECT_EQ(announcement.announcement, "new");

    const auto icon = memochat::chat::group::ChatGroupIconUpdateRequestFromJsonValue(
        Parse(R"({"fromuid":19,"groupid":701,"icon":"avatar.png"})"));
    EXPECT_EQ(icon.uid, 19);
    EXPECT_EQ(icon.group_id, 701);
    EXPECT_EQ(icon.icon, "avatar.png");
}

TEST(ChatGroupCommandDtosTest, ReadsGroupIdRequest)
{
    const auto request =
        memochat::chat::group::ChatGroupIdRequestFromJsonValue(Parse(R"({"fromuid":20,"groupid":800})"));

    EXPECT_EQ(request.uid, 20);
    EXPECT_EQ(request.group_id, 800);
}

TEST(ChatGroupCommandDtosTest, ReadsSetAdminDefaultPermissionBits)
{
    const auto request = memochat::chat::group::ChatGroupSetAdminRequestFromJsonValue(
        Parse(R"({"fromuid":21,"target_user_id":"admin-user","groupid":900})"));

    EXPECT_EQ(request.uid, 21);
    EXPECT_EQ(request.target_user_id, "admin-user");
    EXPECT_EQ(request.group_id, 900);
    EXPECT_TRUE(request.is_admin);
    EXPECT_FALSE(request.has_permission_bits);
    EXPECT_EQ(request.permission_bits, 0);
    EXPECT_EQ(request.requested_permission_bits, memochat::chat::group::kDefaultAdminPermBits);
}

TEST(ChatGroupCommandDtosTest, MergesSetAdminPermissionBitsAndFlags)
{
    const int64_t base =
        memochat::chat::group::kGroupPermInviteUsers | memochat::chat::group::kGroupPermPinMessages;
    const auto request = memochat::chat::group::ChatGroupSetAdminRequestFromJsonValue(
        Parse(R"({"fromuid":22,"target_user_id":"target","groupid":901,"permission_bits":20,"can_invite_users":false,"can_manage_admins":true})"));

    EXPECT_EQ(base, memochat::chat::group::kGroupPermInviteUsers | memochat::chat::group::kGroupPermPinMessages);
    EXPECT_TRUE(request.is_admin);
    EXPECT_TRUE(request.has_permission_bits);
    EXPECT_EQ(request.permission_bits,
              memochat::chat::group::kGroupPermPinMessages | memochat::chat::group::kGroupPermManageAdmins);
    EXPECT_EQ(request.requested_permission_bits, request.permission_bits);
}

TEST(ChatGroupCommandDtosTest, ClearsSetAdminPermissionBitsWhenRemovingAdmin)
{
    const auto request = memochat::chat::group::ChatGroupSetAdminRequestFromJsonValue(
        Parse(R"({"fromuid":23,"target_user_id":"target","groupid":902,"is_admin":false,"permission_bits":127,"can_manage_admins":true})"));

    EXPECT_FALSE(request.is_admin);
    EXPECT_TRUE(request.has_permission_bits);
    EXPECT_EQ(request.permission_bits, 0);
    EXPECT_EQ(request.requested_permission_bits, 0);
}

TEST(ChatGroupCommandDtosTest, ReadsMemberActionRequestForMuteAndKick)
{
    const auto mute = memochat::chat::group::ChatGroupMemberActionRequestFromJsonValue(
        Parse(R"({"fromuid":24,"target_user_id":"muted","groupid":903,"mute_seconds":3600})"));
    EXPECT_EQ(mute.uid, 24);
    EXPECT_EQ(mute.target_user_id, "muted");
    EXPECT_EQ(mute.group_id, 903);
    EXPECT_EQ(mute.mute_seconds, 3600);

    const auto kick = memochat::chat::group::ChatGroupMemberActionRequestFromJsonValue(
        Parse(R"({"fromuid":25,"target_user_id":"kicked","groupid":904})"));
    EXPECT_EQ(kick.uid, 25);
    EXPECT_EQ(kick.target_user_id, "kicked");
    EXPECT_EQ(kick.group_id, 904);
    EXPECT_EQ(kick.mute_seconds, 0);
}

TEST(ChatGroupCommandDtosTest, WritesInviteMemberResponseRoot)
{
    const memochat::json::JsonValue out = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupInviteMemberResponseDto{
            .error = 0, .groupid = 901, .touid = 42, .target_user_id = "u-42"});

    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["groupid"].asInt64(), 901);
    EXPECT_EQ(out["touid"].asInt(), 42);
    EXPECT_EQ(out["target_user_id"].asString(), "u-42");
}

TEST(ChatGroupCommandDtosTest, WritesApplyJoinResponseRoot)
{
    const memochat::json::JsonValue out = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupApplyJoinResponseDto{.error = 0, .groupid = 902, .group_code = "G902"});

    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["groupid"].asInt64(), 902);
    EXPECT_EQ(out["group_code"].asString(), "G902");
}

TEST(ChatGroupCommandDtosTest, WritesAnnouncementUpdateResponseWithAndWithoutGroupCode)
{
    const memochat::json::JsonValue with = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupAnnouncementUpdateResponseDto{
            .error = 0, .groupid = 903, .announcement = "hello", .group_code = "G903"});
    EXPECT_EQ(with["error"].asInt(), 0);
    EXPECT_EQ(with["groupid"].asInt64(), 903);
    EXPECT_EQ(with["announcement"].asString(), "hello");
    EXPECT_EQ(with["group_code"].asString(), "G903");

    const memochat::json::JsonValue without = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupAnnouncementUpdateResponseDto{
            .error = 0, .groupid = 903, .announcement = "hello"});
    EXPECT_FALSE(without.isMember("group_code")) << without.toStyledString();
}

TEST(ChatGroupCommandDtosTest, WritesIconUpdateResponseWithAndWithoutGroupCode)
{
    const memochat::json::JsonValue with = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupIconUpdateResponseDto{
            .error = 0, .groupid = 905, .icon = "i.png", .group_code = "G905"});
    EXPECT_EQ(with["icon"].asString(), "i.png");
    EXPECT_EQ(with["group_code"].asString(), "G905");

    const memochat::json::JsonValue without = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupIconUpdateResponseDto{.error = 0, .groupid = 905, .icon = "i.png"});
    EXPECT_FALSE(without.isMember("group_code")) << without.toStyledString();
}

TEST(ChatGroupCommandDtosTest, WritesQuitResponseWithAndWithoutGroupCode)
{
    const memochat::json::JsonValue with = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupQuitResponseDto{.error = 0, .groupid = 906, .group_code = "G906"});
    EXPECT_EQ(with["error"].asInt(), 0);
    EXPECT_EQ(with["groupid"].asInt64(), 906);
    EXPECT_EQ(with["group_code"].asString(), "G906");

    const memochat::json::JsonValue without = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupQuitResponseDto{.error = 1, .groupid = 906});
    EXPECT_EQ(without["error"].asInt(), 1);
    EXPECT_FALSE(without.isMember("group_code")) << without.toStyledString();
}

TEST(ChatGroupCommandDtosTest, WritesDissolveResponseWithAndWithoutOptionalTail)
{
    const memochat::json::JsonValue with = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupDissolveResponseDto{
            .error = 0, .groupid = 907, .group_code = "G907", .name = "team"});
    EXPECT_EQ(with["error"].asInt(), 0);
    EXPECT_EQ(with["groupid"].asInt64(), 907);
    EXPECT_EQ(with["group_code"].asString(), "G907");
    EXPECT_EQ(with["name"].asString(), "team");

    const memochat::json::JsonValue without = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupDissolveResponseDto{.error = 8, .groupid = 907});
    EXPECT_EQ(without["error"].asInt(), 8);
    EXPECT_FALSE(without.isMember("group_code")) << without.toStyledString();
    EXPECT_FALSE(without.isMember("name")) << without.toStyledString();
}

TEST(ChatGroupCommandDtosTest, WritesMuteMemberResponseWithAndWithoutGroupCode)
{
    const memochat::json::JsonValue with = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupMuteMemberResponseDto{
            .error = 0, .groupid = 908, .touid = 51, .target_user_id = "u-51", .mute_until = 1700, .group_code = "G908"});
    EXPECT_EQ(with["error"].asInt(), 0);
    EXPECT_EQ(with["groupid"].asInt64(), 908);
    EXPECT_EQ(with["touid"].asInt(), 51);
    EXPECT_EQ(with["target_user_id"].asString(), "u-51");
    EXPECT_EQ(with["mute_until"].asInt64(), 1700);
    EXPECT_EQ(with["group_code"].asString(), "G908");

    const memochat::json::JsonValue without = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupMuteMemberResponseDto{
            .error = 0, .groupid = 908, .touid = 51, .target_user_id = "u-51", .mute_until = 0});
    EXPECT_EQ(without["mute_until"].asInt64(), 0);
    EXPECT_FALSE(without.isMember("group_code")) << without.toStyledString();
}

TEST(ChatGroupCommandDtosTest, WritesKickMemberResponseWithAndWithoutGroupCode)
{
    const memochat::json::JsonValue with = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupKickMemberResponseDto{
            .error = 0, .groupid = 909, .touid = 52, .target_user_id = "u-52", .group_code = "G909"});
    EXPECT_EQ(with["touid"].asInt(), 52);
    EXPECT_EQ(with["target_user_id"].asString(), "u-52");
    EXPECT_EQ(with["group_code"].asString(), "G909");

    const memochat::json::JsonValue without = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupKickMemberResponseDto{
            .error = 7, .groupid = 909, .touid = 52, .target_user_id = "u-52"});
    EXPECT_EQ(without["error"].asInt(), 7);
    EXPECT_FALSE(without.isMember("group_code")) << without.toStyledString();
}

// The SetGroupAdmin DTO owns only the scalar shell. The can_* permission flags
// must NOT come from the DTO — they are appended separately by
// GroupResponseFormatter::ApplyPermissionFlags() at the call site (slice AZ boundary).
TEST(ChatGroupCommandDtosTest, SetAdminResponseShellOmitsPermissionFlags)
{
    const memochat::json::JsonValue out = memochat::chat::group::ToJsonValue(
        memochat::chat::group::ChatGroupSetAdminResponseDto{
            .error = 0,
            .groupid = 910,
            .touid = 53,
            .target_user_id = "u-53",
            .is_admin = true,
            .permission_bits = 7});

    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["groupid"].asInt64(), 910);
    EXPECT_EQ(out["touid"].asInt(), 53);
    EXPECT_EQ(out["target_user_id"].asString(), "u-53");
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(out, "is_admin", false));
    EXPECT_EQ(out["permission_bits"].asInt64(), 7);
    // can_* flags are the formatter's responsibility, not the DTO's.
    EXPECT_FALSE(out.isMember("can_change_group_info")) << out.toStyledString();
    EXPECT_FALSE(out.isMember("can_delete_messages")) << out.toStyledString();
    EXPECT_FALSE(out.isMember("can_manage_admins")) << out.toStyledString();
    EXPECT_FALSE(out.isMember("group_code")) << out.toStyledString();
}

TEST(ChatGroupCommandDtosTest, WritesGroupListResponseShellThenArrays)
{
    // The DTO owns only the leading error scalar; the caller appends the arrays
    // afterward (here simulated) to preserve wire order.
    memochat::json::JsonValue rtvalue =
        memochat::chat::group::ToJsonValue(memochat::chat::group::ChatGroupListResponseDto{.error = 0});
    rtvalue["group_list"] = memochat::json::arrayValue;
    rtvalue["pending_group_apply_list"] = memochat::json::arrayValue;

    const memochat::json::JsonValue out = rtvalue;
    EXPECT_EQ(out["error"].asInt(), 0);
    ASSERT_TRUE(out["group_list"].isArray()) << out.toStyledString();
    ASSERT_TRUE(out["pending_group_apply_list"].isArray()) << out.toStyledString();
    EXPECT_EQ(out["group_list"].size(), 0u);
}

TEST(ChatGroupCommandDtosTest, GroupListResponseShellOmitsArrays)
{
    // The bare shell (error-only) must not carry the arrays — they are caller-applied.
    const memochat::json::JsonValue out =
        memochat::chat::group::ToJsonValue(memochat::chat::group::ChatGroupListResponseDto{.error = 5});
    EXPECT_EQ(out["error"].asInt(), 5);
    EXPECT_FALSE(out.isMember("group_list")) << out.toStyledString();
    EXPECT_FALSE(out.isMember("pending_group_apply_list")) << out.toStyledString();
}

TEST(ChatGroupCommandDtosTest, WritesGroupHistoryShellWithDynamicMessagesAndGroupCode)
{
    // DTO owns the 4 scalars; messages array sits after the shell and the
    // caller-gated group_code is appended last (current wire order).
    memochat::json::JsonValue rtvalue =
        memochat::chat::group::ToJsonValue(memochat::chat::group::ChatGroupHistoryResponseDto{
            .error = 0, .groupid = 920, .has_more = true, .next_before_seq = 4242});
    rtvalue["messages"] = memochat::json::arrayValue;
    append(rtvalue["messages"], Parse(R"({"msgid":"m-1","content":"hi"})"));
    rtvalue["group_code"] = "G920";

    const memochat::json::JsonValue out = rtvalue;
    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["groupid"].asInt64(), 920);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(out, "has_more", false));
    EXPECT_EQ(out["next_before_seq"].asInt64(), 4242);
    ASSERT_TRUE(out["messages"].isArray()) << out.toStyledString();
    EXPECT_EQ(out["messages"][0]["msgid"].asString(), "m-1");
    EXPECT_EQ(out["messages"][0]["content"].asString(), "hi");
    EXPECT_EQ(out["group_code"].asString(), "G920");
}

TEST(ChatGroupCommandDtosTest, GroupHistoryShellOmitsMessagesAndGroupCode)
{
    const memochat::json::JsonValue out =
        memochat::chat::group::ToJsonValue(memochat::chat::group::ChatGroupHistoryResponseDto{
            .error = 3, .groupid = 920, .has_more = false, .next_before_seq = 0});
    EXPECT_EQ(out["error"].asInt(), 3);
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(out, "has_more", true));
    EXPECT_FALSE(out.isMember("messages")) << out.toStyledString();
    EXPECT_FALSE(out.isMember("group_code")) << out.toStyledString();
}
