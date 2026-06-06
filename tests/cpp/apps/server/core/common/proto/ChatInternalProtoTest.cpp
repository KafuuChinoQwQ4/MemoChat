#include <gtest/gtest.h>

#include "common/proto/chat_internal.grpc.pb.h"
#include "common/proto/chat_internal.pb.h"

#include <string>

using namespace ::chatinternal;

namespace
{
const google::protobuf::ServiceDescriptor* FindService(const std::string& service_name)
{
    return google::protobuf::DescriptorPool::generated_pool()->FindServiceByName("chatinternal." + service_name);
}
} // namespace

TEST(ChatInternalProtoTest, JsonPayloadRequestRoundTripKeepsSessionContext)
{
    JsonPayloadRequest req;
    req.mutable_session()->set_uid(42);
    req.mutable_session()->set_session_id("session-42");
    req.mutable_session()->set_server_name("ChatServer-1");
    req.mutable_session()->set_trace_id("trace-abc");
    req.set_tcp_msg_id(1005);
    req.set_payload_json(R"({"keyword":"alice"})");

    const std::string bytes = req.SerializeAsString();

    JsonPayloadRequest parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.session().uid(), 42);
    EXPECT_EQ(parsed.session().session_id(), "session-42");
    EXPECT_EQ(parsed.session().server_name(), "ChatServer-1");
    EXPECT_EQ(parsed.session().trace_id(), "trace-abc");
    EXPECT_EQ(parsed.tcp_msg_id(), 1005);
    EXPECT_EQ(parsed.payload_json(), R"({"keyword":"alice"})");
}

TEST(ChatInternalProtoTest, JsonPayloadResponseCarriesDeliveryAndEventCommands)
{
    JsonPayloadResponse rsp;
    rsp.set_error(0);
    rsp.set_tcp_msg_id(1006);
    rsp.set_payload_json(R"({"accepted":true})");

    auto* delivery = rsp.add_delivery_commands();
    delivery->set_tcp_msg_id(2001);
    delivery->set_payload_json(R"({"text":"hello"})");
    delivery->add_target_uids(7);
    delivery->add_target_uids(8);
    delivery->set_route_hint("ChatServer-2");
    delivery->set_require_offline_push(true);

    auto* event = rsp.add_event_commands();
    event->set_topic("chat.private.v1");
    event->set_key("msg-1");
    event->set_payload_json(R"({"msg_id":"msg-1"})");

    const std::string bytes = rsp.SerializeAsString();

    JsonPayloadResponse parsed;
    ASSERT_TRUE(parsed.ParseFromString(bytes));
    EXPECT_EQ(parsed.error(), 0);
    EXPECT_EQ(parsed.tcp_msg_id(), 1006);
    EXPECT_EQ(parsed.payload_json(), R"({"accepted":true})");
    ASSERT_EQ(parsed.delivery_commands_size(), 1);
    EXPECT_EQ(parsed.delivery_commands(0).tcp_msg_id(), 2001);
    ASSERT_EQ(parsed.delivery_commands(0).target_uids_size(), 2);
    EXPECT_EQ(parsed.delivery_commands(0).target_uids(0), 7);
    EXPECT_EQ(parsed.delivery_commands(0).target_uids(1), 8);
    EXPECT_EQ(parsed.delivery_commands(0).route_hint(), "ChatServer-2");
    EXPECT_TRUE(parsed.delivery_commands(0).require_offline_push());
    ASSERT_EQ(parsed.event_commands_size(), 1);
    EXPECT_EQ(parsed.event_commands(0).topic(), "chat.private.v1");
    EXPECT_EQ(parsed.event_commands(0).key(), "msg-1");
    EXPECT_EQ(parsed.event_commands(0).payload_json(), R"({"msg_id":"msg-1"})");
}

TEST(ChatInternalProtoTest, BootstrapRoundTripKeepsJsonPayload)
{
    BootstrapRequest req;
    req.set_uid(99);
    req.set_trace_id("trace-bootstrap");

    const std::string req_bytes = req.SerializeAsString();
    BootstrapRequest parsed_req;
    ASSERT_TRUE(parsed_req.ParseFromString(req_bytes));
    EXPECT_EQ(parsed_req.uid(), 99);
    EXPECT_EQ(parsed_req.trace_id(), "trace-bootstrap");

    BootstrapResponse rsp;
    rsp.set_error(0);
    rsp.set_payload_json(R"({"dialogs":[],"friends":[]})");

    const std::string rsp_bytes = rsp.SerializeAsString();
    BootstrapResponse parsed_rsp;
    ASSERT_TRUE(parsed_rsp.ParseFromString(rsp_bytes));
    EXPECT_EQ(parsed_rsp.error(), 0);
    EXPECT_EQ(parsed_rsp.payload_json(), R"({"dialogs":[],"friends":[]})");
}

TEST(ChatInternalProtoTest, GrpcDescriptorsExposeInternalServiceContracts)
{
    const auto* relation_service = FindService("ChatRelationInternalService");
    ASSERT_NE(relation_service, nullptr);
    EXPECT_NE(relation_service->FindMethodByName("AppendRelationBootstrap"), nullptr);
    EXPECT_NE(relation_service->FindMethodByName("BuildDialogList"), nullptr);
    EXPECT_NE(relation_service->FindMethodByName("SearchUser"), nullptr);
    EXPECT_NE(relation_service->FindMethodByName("AddFriendApply"), nullptr);
    EXPECT_NE(relation_service->FindMethodByName("PinDialog"), nullptr);

    const auto* private_service = FindService("ChatPrivateMessageInternalService");
    ASSERT_NE(private_service, nullptr);
    EXPECT_NE(private_service->FindMethodByName("SendTextChatMessage"), nullptr);
    EXPECT_NE(private_service->FindMethodByName("ForwardPrivateMessage"), nullptr);
    EXPECT_NE(private_service->FindMethodByName("PrivateReadAck"), nullptr);
    EXPECT_NE(private_service->FindMethodByName("PrivateHistory"), nullptr);

    const auto* group_service = FindService("ChatGroupMessageInternalService");
    ASSERT_NE(group_service, nullptr);
    EXPECT_NE(group_service->FindMethodByName("BuildGroupList"), nullptr);
    EXPECT_NE(group_service->FindMethodByName("CreateGroup"), nullptr);
    EXPECT_NE(group_service->FindMethodByName("GroupChatMessage"), nullptr);
    EXPECT_NE(group_service->FindMethodByName("DissolveGroup"), nullptr);
}
