#include <gtest/gtest.h>

#include "json/GlazeCompat.h"
#include "message/GroupMessageService.h"
#include "message/PrivateMessageService.h"
#include "ports/IDeliveryGateway.h"
#include "ports/IEventPublisher.h"
#include "ports/IMessageRepository.h"
#include "ports/IRelationRepository.h"

#include <memory>
#include <string>
#include <vector>

// Golden tests pinning the EXACT wire output of the private/group send-path
// response roots (TextChatMessage / GroupChatMessage rtvalue) BEFORE and AFTER
// the slice-BQ DTO refactor. They run in the default-config process: no config
// file is loaded, so AsyncEventBusBackend() defaults to "redis" (kafka OFF) and
// SelfServerName() is empty. That deterministically exercises the persisted +
// error branches — the dominant production path. Kafka_primary/shadow branches
// depend on process-wide ConfigMgr singleton state set once at first access and
// are out of scope for this in-process golden (documented in bq/context.md).

namespace
{

using memochat::json::JsonValue;

JsonValue Parse(const std::string& body)
{
    JsonValue root;
    EXPECT_TRUE(memochat::json::glaze_parse(root, body)) << body;
    return root;
}

// ---- Fakes -----------------------------------------------------------------

class StubMessageRepository : public IMessageRepository
{
public:
    bool save_private_ok = true;
    bool save_group_ok = true;
    int64_t group_server_msg_id = 7777;
    int64_t group_seq_value = 42;

    bool SavePrivateMessage(const PrivateMessageInfo&) override
    {
        return save_private_ok;
    }
    bool FindPrivateMessageByClientId(const std::string&, std::shared_ptr<PrivateMessageInfo>&) override
    {
        return false;
    }
    bool IsPrivateFriend(int, int) override
    {
        return true;
    }
    bool UpsertPrivateReadState(int, int, int64_t) override
    {
        return true;
    }
    bool UpdatePrivateMessageContent(int, int, const std::string&, const std::string&, int64_t) override
    {
        return true;
    }
    bool RevokePrivateMessage(int, int, const std::string&, int64_t) override
    {
        return true;
    }
    bool GetPrivateHistory(int,
                           int,
                           int64_t,
                           const std::string&,
                           int,
                           std::vector<std::shared_ptr<PrivateMessageInfo>>&,
                           bool&) override
    {
        return true;
    }
    bool SaveGroupMessage(const GroupMessageInfo&, int64_t* out_server_msg_id, int64_t* out_group_seq, int64_t) override
    {
        if (out_server_msg_id)
        {
            *out_server_msg_id = group_server_msg_id;
        }
        if (out_group_seq)
        {
            *out_group_seq = group_seq_value;
        }
        return save_group_ok;
    }
    bool FindGroupMessageByClientId(int64_t, const std::string&, std::shared_ptr<GroupMessageInfo>&) override
    {
        return false;
    }
    bool
    GetGroupHistory(int64_t, int64_t, int64_t, int, std::vector<std::shared_ptr<GroupMessageInfo>>&, bool&) override
    {
        return true;
    }
    bool UpdateGroupMessageContent(int64_t, int, const std::string&, const std::string&, int64_t) override
    {
        return true;
    }
    bool RevokeGroupMessage(int64_t, int, const std::string&, int64_t) override
    {
        return true;
    }
    bool UpsertGroupReadState(int, int64_t, int64_t) override
    {
        return true;
    }
};

class StubDeliveryGateway : public IDeliveryGateway
{
public:
    int push_calls = 0;
    bool try_push_result = true;

    void PushPayload(const std::vector<int>&, short, const JsonValue&, int) override
    {
        ++push_calls;
    }
    bool TryPushPayload(const std::vector<int>&, short, const JsonValue&, int, bool) override
    {
        ++push_calls;
        return try_push_result;
    }
};

class StubEventPublisher : public IEventPublisher
{
public:
    bool PublishEvent(const std::string&, const JsonValue&, std::string*) override
    {
        return true;
    }
};

// Minimal relation repository for the group path: only the four methods
// GroupChatMessage exercises return meaningful values; the rest are inert.
class StubRelationRepository : public IRelationRepository
{
public:
    int role = 2; // owner/admin so mention_all is allowed
    std::shared_ptr<UserInfo> sender;
    std::shared_ptr<GroupInfo> group;
    std::vector<std::shared_ptr<GroupMemberInfo>> members;
    // CreateGroup / ReviewGroupApply controllable outputs
    bool create_group_ok = true;
    int64_t create_group_id = 0;
    std::string create_group_code;
    bool review_ok = true;
    std::shared_ptr<GroupApplyInfo> review_apply;

    bool GetUserRoleInGroup(int64_t, int, int& out_role) override
    {
        out_role = role;
        return true;
    }
    bool GetGroupMemberList(int64_t, std::vector<std::shared_ptr<GroupMemberInfo>>& out) override
    {
        out = members;
        return true;
    }
    std::shared_ptr<UserInfo> GetUserByUid(int) override
    {
        return sender;
    }
    bool GetGroupById(int64_t, std::shared_ptr<GroupInfo>& out) override
    {
        out = group;
        return group != nullptr;
    }

    // ---- inert remainder ----
    bool GetUidByUserId(const std::string&, int&) override
    {
        return false;
    }
    bool RefreshDialogsForOwner(int) override
    {
        return true;
    }
    bool GetDialogMetaByOwner(int, std::vector<std::shared_ptr<DialogMetaInfo>>&) override
    {
        return true;
    }
    bool GetPrivateDialogRuntime(int, int, DialogRuntimeInfo&) override
    {
        return true;
    }
    bool GetGroupDialogRuntime(int, int64_t, DialogRuntimeInfo&) override
    {
        return true;
    }
    bool GetUserGroupList(int, std::vector<std::shared_ptr<GroupInfo>>&) override
    {
        return true;
    }
    bool GetPendingGroupApplyForReviewer(int, std::vector<std::shared_ptr<GroupApplyInfo>>&, int) override
    {
        return true;
    }
    bool GetGroupIdByCode(const std::string&, int64_t&) override
    {
        return false;
    }
    bool AddFriendApply(int, int) override
    {
        return true;
    }
    bool ReplaceApplyTags(int, int, const std::vector<std::string>&) override
    {
        return true;
    }
    bool AuthFriendApply(int, int) override
    {
        return true;
    }
    bool AddFriend(int, int, const std::string&) override
    {
        return true;
    }
    std::vector<std::string> GetApplyTags(int, int) override
    {
        return {};
    }
    bool ReplaceFriendTags(int, int, const std::vector<std::string>&) override
    {
        return true;
    }
    bool DeleteFriend(int, int) override
    {
        return true;
    }
    bool IsPrivateFriend(int, int) override
    {
        return true;
    }
    std::vector<int> FilterFriendUids(int, const std::vector<int>&) override
    {
        return {};
    }
    bool IsGroupMember(int64_t, int) override
    {
        return true;
    }
    bool CreateGroup(int,
                     const std::string&,
                     const std::string&,
                     int,
                     const std::vector<int>&,
                     int64_t& out_group_id,
                     std::string& out_group_code) override
    {
        out_group_id = create_group_id;
        out_group_code = create_group_code;
        return create_group_ok;
    }
    bool InviteGroupMember(int64_t, int, int, const std::string&) override
    {
        return true;
    }
    bool ApplyJoinGroup(int64_t, int, const std::string&) override
    {
        return true;
    }
    bool ReviewGroupApply(int64_t, int, bool, std::shared_ptr<GroupApplyInfo>& out_apply) override
    {
        out_apply = review_apply;
        return review_ok;
    }
    bool UpdateGroupAnnouncement(int64_t, int, const std::string&) override
    {
        return true;
    }
    bool UpdateGroupIcon(int64_t, int, const std::string&) override
    {
        return true;
    }
    bool SetGroupAdmin(int64_t, int, int, bool, int64_t) override
    {
        return true;
    }
    bool MuteGroupMember(int64_t, int, int, int64_t) override
    {
        return true;
    }
    bool KickGroupMember(int64_t, int, int) override
    {
        return true;
    }
    bool QuitGroup(int64_t, int) override
    {
        return true;
    }
    bool DissolveGroup(int64_t, int) override
    {
        return true;
    }
    bool UpsertDialogDraft(int, const std::string&, int, int64_t, const std::string&) override
    {
        return true;
    }
    bool UpsertDialogMuteState(int, const std::string&, int, int64_t, int) override
    {
        return true;
    }
    bool UpsertDialogPinned(int, const std::string&, int, int64_t, int) override
    {
        return true;
    }
};

MessageCommandRequest MakeRequest(const std::string& payload_json)
{
    MessageCommandRequest request;
    request.payload_json = payload_json;
    return request;
}

} // namespace

// ============================ Private send path ============================

TEST(SendPathGoldenTest, PrivateBadInputRoot)
{
    StubMessageRepository repo;
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    PrivateMessageService service(nullptr, nullptr, &repo, &delivery, &publisher);

    const auto result = service.TextChatMessage(MakeRequest(R"({"fromuid":0,"touid":20,"text_array":[]})"));
    const JsonValue out = Parse(result.payload_json);

    // bad-input branch: exactly {error, fromuid, touid}, error = Error_Json (1005-ish; assert non-zero)
    EXPECT_NE(out["error"].asInt(), 0);
    EXPECT_EQ(out["fromuid"].asInt(), 0);
    EXPECT_EQ(out["touid"].asInt(), 20);
    EXPECT_FALSE(out.isMember("client_msg_id")) << result.payload_json;
    EXPECT_FALSE(out.isMember("status")) << result.payload_json;
    EXPECT_FALSE(out.isMember("text_array")) << result.payload_json;
    EXPECT_FALSE(out.isMember("accept_node")) << result.payload_json;
}

TEST(SendPathGoldenTest, PrivatePersistedSuccessRoot)
{
    StubMessageRepository repo;
    repo.save_private_ok = true;
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    PrivateMessageService service(nullptr, nullptr, &repo, &delivery, &publisher);

    const auto result = service.TextChatMessage(
        MakeRequest(R"({"fromuid":10,"touid":20,"text_array":[{"msgid":"m-1","content":"hello"}]})"));
    const JsonValue out = Parse(result.payload_json);

    // persisted (redis default, kafka off) success branch
    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["fromuid"].asInt(), 10);
    EXPECT_EQ(out["touid"].asInt(), 20);
    EXPECT_EQ(out["client_msg_id"].asString(), "m-1");
    EXPECT_TRUE(out.isMember("accept_node")) << result.payload_json; // value volatile (SelfServerName)
    EXPECT_TRUE(out.isMember("accept_ts")) << result.payload_json;   // value volatile (NowMs)
    EXPECT_EQ(out["status"].asString(), "persisted");
    ASSERT_TRUE(out["text_array"].isArray()) << result.payload_json;
    EXPECT_EQ(out["text_array"][0]["msgid"].asString(), "m-1");
    EXPECT_EQ(out["text_array"][0]["content"].asString(), "hello");
}

TEST(SendPathGoldenTest, PrivatePersistedRepoFailRoot)
{
    StubMessageRepository repo;
    repo.save_private_ok = false; // SavePrivateMessage fails
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    PrivateMessageService service(nullptr, nullptr, &repo, &delivery, &publisher);

    const auto result = service.TextChatMessage(
        MakeRequest(R"({"fromuid":10,"touid":20,"text_array":[{"msgid":"m-2","content":"hi"}]})"));
    const JsonValue out = Parse(result.payload_json);

    EXPECT_NE(out["error"].asInt(), 0); // RPCFailed
    EXPECT_EQ(out["status"].asString(), "failed");
    EXPECT_EQ(out["client_msg_id"].asString(), "m-2");
    ASSERT_TRUE(out["text_array"].isArray()) << result.payload_json; // persisted path keeps text_array
}

// ============================= Group send path =============================

TEST(SendPathGoldenTest, GroupBadInputRoot)
{
    StubMessageRepository repo;
    StubRelationRepository relation;
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    GroupMessageService service(&repo, &relation, &delivery, &publisher);

    const auto result = service.GroupChatMessage(MakeRequest(R"({"fromuid":0,"groupid":900})"));
    const JsonValue out = Parse(result.payload_json);

    EXPECT_NE(out["error"].asInt(), 0);
    EXPECT_EQ(out["fromuid"].asInt(), 0);
    EXPECT_EQ(out["groupid"].asInt64(), 900);
    EXPECT_FALSE(out.isMember("status")) << result.payload_json;
    EXPECT_FALSE(out.isMember("msg")) << result.payload_json;
}

TEST(SendPathGoldenTest, GroupPersistedSuccessRoot)
{
    StubMessageRepository repo;
    repo.save_group_ok = true;
    repo.group_server_msg_id = 5555;
    repo.group_seq_value = 12;

    StubRelationRepository relation;
    relation.role = 2;
    relation.sender = std::make_shared<UserInfo>();
    relation.sender->name = "Alice";
    relation.sender->nick = "A";
    relation.sender->icon = "i.png";
    relation.sender->user_id = "alice";
    relation.group = std::make_shared<GroupInfo>();
    relation.group->group_code = "G900";
    auto member = std::make_shared<GroupMemberInfo>();
    member->uid = 10;
    member->status = 1;
    member->mute_until = 0;
    relation.members.push_back(member);

    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    GroupMessageService service(&repo, &relation, &delivery, &publisher);

    const auto result = service.GroupChatMessage(
        MakeRequest(R"({"fromuid":10,"groupid":900,"msg":{"msgid":"g-1","msgtype":"text","content":"hey"}})"));
    const JsonValue out = Parse(result.payload_json);

    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["fromuid"].asInt(), 10);
    EXPECT_EQ(out["groupid"].asInt64(), 900);
    EXPECT_EQ(out["client_msg_id"].asString(), "g-1");
    EXPECT_TRUE(out.isMember("accept_node")) << result.payload_json;
    EXPECT_TRUE(out.isMember("accept_ts")) << result.payload_json;
    EXPECT_EQ(out["status"].asString(), "persisted");
    EXPECT_EQ(out["from_name"].asString(), "Alice");
    EXPECT_EQ(out["from_nick"].asString(), "A");
    EXPECT_EQ(out["from_icon"].asString(), "i.png");
    EXPECT_EQ(out["from_user_id"].asString(), "alice");
    EXPECT_EQ(out["group_code"].asString(), "G900");
    ASSERT_TRUE(out["msg"].isObject()) << result.payload_json;
    EXPECT_EQ(out["msg"]["msgid"].asString(), "g-1");
    EXPECT_EQ(out["msg"]["server_msg_id"].asInt64(), 5555);
    EXPECT_EQ(out["msg"]["group_seq"].asInt64(), 12);
    EXPECT_EQ(out["server_msg_id"].asInt64(), 5555);
    EXPECT_EQ(out["group_seq"].asInt64(), 12);
    EXPECT_TRUE(out.isMember("created_at")) << result.payload_json; // volatile (NowMs)
}

TEST(SendPathGoldenTest, GroupPersistedRepoFailRoot)
{
    StubMessageRepository repo;
    repo.save_group_ok = false; // SaveGroupMessage fails

    StubRelationRepository relation;
    relation.role = 2;
    relation.group = std::make_shared<GroupInfo>();
    relation.group->group_code = "G900";

    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    GroupMessageService service(&repo, &relation, &delivery, &publisher);

    const auto result = service.GroupChatMessage(
        MakeRequest(R"({"fromuid":10,"groupid":900,"msg":{"msgid":"g-2","msgtype":"text","content":"hey"}})"));
    const JsonValue out = Parse(result.payload_json);

    EXPECT_NE(out["error"].asInt(), 0); // RPCFailed
    EXPECT_EQ(out["status"].asString(), "failed");
    EXPECT_FALSE(out.isMember("server_msg_id")) << result.payload_json; // not reached
}

TEST(SendPathGoldenTest, GroupMutedMemberRoot)
{
    StubMessageRepository repo;
    StubRelationRepository relation;
    relation.role = 2;
    auto member = std::make_shared<GroupMemberInfo>();
    member->uid = 10;
    member->status = 1;
    member->mute_until = 9999999999LL; // far future → muted
    relation.members.push_back(member);

    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    GroupMessageService service(&repo, &relation, &delivery, &publisher);

    const auto result = service.GroupChatMessage(
        MakeRequest(R"({"fromuid":10,"groupid":900,"msg":{"msgid":"g-3","msgtype":"text","content":"hey"}})"));
    const JsonValue out = Parse(result.payload_json);

    EXPECT_NE(out["error"].asInt(), 0); // GroupMuted
    EXPECT_FALSE(out.isMember("status")) << result.payload_json;
    EXPECT_FALSE(out.isMember("msg")) << result.payload_json;
}

// ---- BS overlooked-root golden coverage ----

TEST(SendPathGoldenTest, PrivateSendItemPreservesForwardMetaAndReplyFields)
{
    StubMessageRepository repo;
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    PrivateMessageService service(nullptr, nullptr, &repo, &delivery, &publisher);

    const auto result = service.TextChatMessage(MakeRequest(
        R"({"fromuid":10,"touid":20,"text_array":[{"msgid":"m-9","content":"hi","reply_to_server_msg_id":555,"forward_meta":{"src":"a"},"edited_at_ms":123}]})"));
    const memochat::json::JsonValue out = Parse(result.payload_json);

    ASSERT_TRUE(out["text_array"].isArray()) << result.payload_json;
    const memochat::json::JsonValue item = out["text_array"][0];
    EXPECT_EQ(item["msgid"].asString(), "m-9");
    EXPECT_EQ(item["content"].asString(), "hi");
    EXPECT_EQ(item["reply_to_server_msg_id"].asInt64(), 555);
    EXPECT_EQ(item["edited_at_ms"].asInt64(), 123);
    ASSERT_TRUE(item["forward_meta"].isObject()) << result.payload_json;
    EXPECT_EQ(item["forward_meta"]["src"].asString(), "a");
}

TEST(SendPathGoldenTest, PrivateHistoryBadInputRoot)
{
    StubMessageRepository repo;
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    PrivateMessageService service(nullptr, nullptr, &repo, &delivery, &publisher);

    const auto result = service.PrivateHistory(MakeRequest(R"({"fromuid":0,"peer_uid":20,"limit":10})"));
    const memochat::json::JsonValue out = Parse(result.payload_json);

    EXPECT_NE(out["error"].asInt(), 0);
    EXPECT_EQ(out["peer_uid"].asInt(), 20);
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(out, "has_more", true));
    ASSERT_TRUE(out["messages"].isArray()) << result.payload_json;
    EXPECT_EQ(out["messages"].size(), 0u);
}

TEST(SendPathGoldenTest, PrivateHistorySuccessEmptyRoot)
{
    StubMessageRepository repo; // GetPrivateHistory returns true, empty
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    PrivateMessageService service(nullptr, nullptr, &repo, &delivery, &publisher);

    const auto result = service.PrivateHistory(MakeRequest(R"({"fromuid":10,"peer_uid":20,"limit":10})"));
    const memochat::json::JsonValue out = Parse(result.payload_json);

    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["peer_uid"].asInt(), 20);
    ASSERT_TRUE(out["messages"].isArray()) << result.payload_json;
}

TEST(SendPathGoldenTest, CreateGroupBadInputRoot)
{
    StubMessageRepository repo;
    StubRelationRepository relation;
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    GroupMessageService service(&repo, &relation, &delivery, &publisher);

    const auto result = service.CreateGroup(MakeRequest(R"({"fromuid":0,"name":""})"));
    const memochat::json::JsonValue out = Parse(result.payload_json);

    EXPECT_NE(out["error"].asInt(), 0);
    EXPECT_FALSE(out.isMember("groupid")) << result.payload_json;
    EXPECT_FALSE(out.isMember("group_list")) << result.payload_json;
}

TEST(SendPathGoldenTest, CreateGroupSuccessRootHasTailAndLists)
{
    StubMessageRepository repo;
    StubRelationRepository relation;
    relation.create_group_ok = true;
    relation.create_group_id = 9001;
    relation.create_group_code = "G9001";
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    GroupMessageService service(&repo, &relation, &delivery, &publisher);

    const auto result =
        service.CreateGroup(MakeRequest(R"({"fromuid":10,"name":"team","announcement":"hi","member_user_ids":[]})"));
    const memochat::json::JsonValue out = Parse(result.payload_json);

    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["groupid"].asInt64(), 9001);
    EXPECT_EQ(out["group_code"].asString(), "G9001");
    EXPECT_EQ(out["name"].asString(), "team");
    EXPECT_EQ(out["announcement"].asString(), "hi");
    // BuildGroupListJson arrays remain present after the DTO tail.
    ASSERT_TRUE(out["group_list"].isArray()) << result.payload_json;
    ASSERT_TRUE(out["pending_group_apply_list"].isArray()) << result.payload_json;
}

TEST(SendPathGoldenTest, ReviewGroupApplyBadInputRoot)
{
    StubMessageRepository repo;
    StubRelationRepository relation;
    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    GroupMessageService service(&repo, &relation, &delivery, &publisher);

    const auto result = service.ReviewGroupApply(MakeRequest(R"({"fromuid":0,"apply_id":0,"agree":true})"));
    const memochat::json::JsonValue out = Parse(result.payload_json);

    EXPECT_NE(out["error"].asInt(), 0);
    EXPECT_EQ(out["apply_id"].asInt64(), 0);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(out, "agree", false));
    EXPECT_FALSE(out.isMember("groupid")) << result.payload_json;
    EXPECT_FALSE(out.isMember("applicant_uid")) << result.payload_json;
}

TEST(SendPathGoldenTest, ReviewGroupApplySuccessRootHasGatedTail)
{
    StubMessageRepository repo;
    StubRelationRepository relation;
    relation.review_ok = true;
    relation.review_apply = std::make_shared<GroupApplyInfo>();
    relation.review_apply->apply_id = 700;
    relation.review_apply->group_id = 9002;
    relation.review_apply->applicant_uid = 33;
    relation.group = std::make_shared<GroupInfo>();
    relation.group->group_code = "G9002";
    relation.sender = std::make_shared<UserInfo>();
    relation.sender->user_id = "applicant-33";

    StubDeliveryGateway delivery;
    StubEventPublisher publisher;
    GroupMessageService service(&repo, &relation, &delivery, &publisher);

    const auto result = service.ReviewGroupApply(MakeRequest(R"({"fromuid":10,"apply_id":700,"agree":true})"));
    const memochat::json::JsonValue out = Parse(result.payload_json);

    EXPECT_EQ(out["error"].asInt(), 0);
    EXPECT_EQ(out["apply_id"].asInt64(), 700);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(out, "agree", false));
    EXPECT_EQ(out["groupid"].asInt64(), 9002);
    EXPECT_EQ(out["applicant_uid"].asInt(), 33);
    EXPECT_EQ(out["group_code"].asString(), "G9002");
    EXPECT_EQ(out["applicant_user_id"].asString(), "applicant-33");
}
