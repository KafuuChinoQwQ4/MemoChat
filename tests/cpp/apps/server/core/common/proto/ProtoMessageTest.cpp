#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common/proto/message.pb.h"
#include "common/proto/message.grpc.pb.h"

#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/time_util.h>

#include <cstdint>
#include <string>

using namespace ::testing;
using namespace ::message;

// ---------------------------------------------------------------------------
// GetVarifyReq / GetVarifyRsp round-trip
// ---------------------------------------------------------------------------
TEST(ProtoMessageTest, GetVarifyReq_SerializeAndParse)
{
    GetVarifyReq req;
    req.set_email("user@example.com");

    std::string bytes = req.SerializeAsString();

    GetVarifyReq parsed;
    EXPECT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.email(), "user@example.com");
}

TEST(ProtoMessageTest, GetVarifyRsp_RoundTrip)
{
    GetVarifyRsp rsp;
    rsp.set_error(0);
    rsp.set_email("user@example.com");
    rsp.set_code("123456");

    std::string bytes = rsp.SerializeAsString();

    GetVarifyRsp parsed;
    EXPECT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.error(), 0);
    EXPECT_EQ(parsed.email(), "user@example.com");
    EXPECT_EQ(parsed.code(), "123456");
}

TEST(ProtoMessageTest, GetVarifyRsp_ErrorCode)
{
    GetVarifyRsp rsp;
    rsp.set_error(404);
    rsp.set_email("nobody@example.com");
    rsp.set_code("");

    std::string bytes = rsp.SerializeAsString();

    GetVarifyRsp parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.error(), 404);
}

// ---------------------------------------------------------------------------
// TextChatMsgReq / TextChatMsgRsp round-trip (repeated fields)
// ---------------------------------------------------------------------------
TEST(ProtoMessageTest, TextChatMsgReq_SingleMessage)
{
    TextChatMsgReq req;
    req.set_fromuid(100);
    req.set_touid(200);

    TextChatData* msg = req.add_textmsgs();
    msg->set_msgid("msg-001");
    msg->set_msgcontent("Hello, world!");

    std::string bytes = req.SerializeAsString();

    TextChatMsgReq parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.fromuid(), 100);
    EXPECT_EQ(parsed.touid(), 200);
    ASSERT_EQ(parsed.textmsgs_size(), 1);
    EXPECT_EQ(parsed.textmsgs(0).msgid(), "msg-001");
    EXPECT_EQ(parsed.textmsgs(0).msgcontent(), "Hello, world!");
}

TEST(ProtoMessageTest, TextChatMsgReq_MultipleMessages)
{
    TextChatMsgReq req;
    req.set_fromuid(1);
    req.set_touid(2);

    for (int i = 0; i < 3; ++i)
    {
        TextChatData* msg = req.add_textmsgs();
        msg->set_msgid("msg-" + std::to_string(i));
        msg->set_msgcontent("content-" + std::to_string(i));
    }

    std::string bytes = req.SerializeAsString();

    TextChatMsgReq parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.textmsgs_size(), 3);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(parsed.textmsgs(i).msgid(), "msg-" + std::to_string(i));
        EXPECT_EQ(parsed.textmsgs(i).msgcontent(), "content-" + std::to_string(i));
    }
}

TEST(ProtoMessageTest, TextChatMsgRsp_EchoesMessages)
{
    TextChatMsgRsp rsp;
    rsp.set_error(0);
    rsp.set_fromuid(10);
    rsp.set_touid(20);

    TextChatData* msg = rsp.add_textmsgs();
    msg->set_msgid("echo-001");
    msg->set_msgcontent("echo back");

    std::string bytes = rsp.SerializeAsString();

    TextChatMsgRsp parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.error(), 0);
    ASSERT_EQ(parsed.textmsgs_size(), 1);
    EXPECT_EQ(parsed.textmsgs(0).msgid(), "echo-001");
}

// ---------------------------------------------------------------------------
// AddFriendReq / AddFriendRsp round-trip
// ---------------------------------------------------------------------------
TEST(ProtoMessageTest, AddFriendReq_Fields)
{
    AddFriendReq req;
    req.set_applyuid(100);
    req.set_name("Alice");
    req.set_desc("Nice to meet you");
    req.set_icon("avatar_001");
    req.set_nick("AliceWonder");
    req.set_sex(1);
    req.set_touid(200);

    std::string bytes = req.SerializeAsString();

    AddFriendReq parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.applyuid(), 100);
    EXPECT_EQ(parsed.name(), "Alice");
    EXPECT_EQ(parsed.desc(), "Nice to meet you");
    EXPECT_EQ(parsed.icon(), "avatar_001");
    EXPECT_EQ(parsed.nick(), "AliceWonder");
    EXPECT_EQ(parsed.sex(), 1);
    EXPECT_EQ(parsed.touid(), 200);
}

// ---------------------------------------------------------------------------
// RplyFriendReq (bool agree field)
// ---------------------------------------------------------------------------
TEST(ProtoMessageTest, RplyFriendReq_AgreeAndReject)
{
    RplyFriendReq agree_req;
    agree_req.set_rplyuid(300);
    agree_req.set_agree(true);
    agree_req.set_touid(100);

    RplyFriendReq reject_req;
    reject_req.set_rplyuid(400);
    reject_req.set_agree(false);
    reject_req.set_touid(100);

    std::string agree_bytes = agree_req.SerializeAsString();
    std::string reject_bytes = reject_req.SerializeAsString();

    RplyFriendReq agree_parsed;
    ASSERT_TRUE(agree_parsed.ParseFromString(agree_bytes));
    EXPECT_TRUE(agree_parsed.agree());

    RplyFriendReq reject_parsed;
    ASSERT_TRUE(reject_parsed.ParseFromString(reject_bytes));
    EXPECT_FALSE(reject_parsed.agree());
}

// ---------------------------------------------------------------------------
// GroupMessageNotifyReq (repeated int32 touids)
// ---------------------------------------------------------------------------
TEST(ProtoMessageTest, GroupMessageNotifyReq_RepeatedTouids)
{
    GroupMessageNotifyReq req;
    req.set_tcp_msgid(999);
    req.set_payload_json(R"({"text":"hello group"})");
    req.add_touids(10);
    req.add_touids(20);
    req.add_touids(30);

    std::string bytes = req.SerializeAsString();

    GroupMessageNotifyReq parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.tcp_msgid(), 999);
    ASSERT_EQ(parsed.touids_size(), 3);
    EXPECT_EQ(parsed.touids(0), 10);
    EXPECT_EQ(parsed.touids(1), 20);
    EXPECT_EQ(parsed.touids(2), 30);
}

// ---------------------------------------------------------------------------
// GroupMemberBatchReq (nested repeated message)
// ---------------------------------------------------------------------------
TEST(ProtoMessageTest, GroupMemberBatchReq_NestedRepeated)
{
    GroupMemberBatchReq req;
    req.set_tcp_msgid(1);
    req.set_payload_json(R"({"action":"mute"})");
    req.add_touids(5);
    req.add_touids(6);

    GroupMemberState* m1 = req.add_members();
    m1->set_uid(5);
    m1->set_role(1);
    m1->set_mute_until(9999999999LL);
    m1->set_status(1);

    GroupMemberState* m2 = req.add_members();
    m2->set_uid(6);
    m2->set_role(2);
    m2->set_mute_until(0);
    m2->set_status(0);

    std::string bytes = req.SerializeAsString();

    GroupMemberBatchReq parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    ASSERT_EQ(parsed.members_size(), 2);

    EXPECT_EQ(parsed.members(0).uid(), 5);
    EXPECT_EQ(parsed.members(0).role(), 1);
    EXPECT_EQ(parsed.members(0).mute_until(), 9999999999LL);
    EXPECT_EQ(parsed.members(0).status(), 1);

    EXPECT_EQ(parsed.members(1).uid(), 6);
    EXPECT_EQ(parsed.members(1).role(), 2);
    EXPECT_EQ(parsed.members(1).mute_until(), 0);
    EXPECT_EQ(parsed.members(1).status(), 0);
}

// ---------------------------------------------------------------------------
// SendChatMsgReq / SendChatMsgRsp
// ---------------------------------------------------------------------------
TEST(ProtoMessageTest, SendChatMsgReq_RoundTrip)
{
    SendChatMsgReq req;
    req.set_fromuid(1);
    req.set_touid(2);
    req.set_message("Hi there");

    std::string bytes = req.SerializeAsString();

    SendChatMsgReq parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.fromuid(), 1);
    EXPECT_EQ(parsed.touid(), 2);
    EXPECT_EQ(parsed.message(), "Hi there");
}

TEST(ProtoMessageTest, SendChatMsgRsp_ErrorPropagation)
{
    SendChatMsgRsp ok;
    ok.set_error(0);
    ok.set_fromuid(1);
    ok.set_touid(2);

    SendChatMsgRsp err;
    err.set_error(500);
    err.set_fromuid(1);
    err.set_touid(2);

    std::string ok_bytes = ok.SerializeAsString();
    std::string err_bytes = err.SerializeAsString();

    SendChatMsgRsp ok_parsed, err_parsed;
    ASSERT_TRUE(ok_parsed.ParseFromString(ok_bytes));
    ASSERT_TRUE(err_parsed.ParseFromString(err_bytes));

    EXPECT_EQ(ok_parsed.error(), 0);
    EXPECT_EQ(err_parsed.error(), 500);
}

// ---------------------------------------------------------------------------
// Default / zero-value fields
// ---------------------------------------------------------------------------
TEST(ProtoMessageTest, DefaultValues_AreZeroOrEmpty)
{
    GetVarifyReq req;
    EXPECT_EQ(req.email(), "");

    SendChatMsgReq send;
    EXPECT_EQ(send.fromuid(), 0);
    EXPECT_EQ(send.touid(), 0);
    EXPECT_EQ(send.message(), "");

    TextChatMsgReq chat;
    EXPECT_EQ(chat.fromuid(), 0);
    EXPECT_EQ(chat.touid(), 0);
    EXPECT_EQ(chat.textmsgs_size(), 0);
}

// ---------------------------------------------------------------------------
// Byte-size sanity
// ---------------------------------------------------------------------------
TEST(ProtoMessageTest, SerializedSize_Reasonable)
{
    TextChatMsgReq req;
    req.set_fromuid(100);
    req.set_touid(200);
    req.add_textmsgs()->set_msgcontent("test");

    int size = req.ByteSizeLong();
    EXPECT_GT(size, 0);
    EXPECT_LT(size, 1024);
}
