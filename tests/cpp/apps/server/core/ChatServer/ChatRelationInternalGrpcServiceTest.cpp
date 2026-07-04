#include <gtest/gtest.h>

#include "ChatRelationInternalGrpcService.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"

namespace
{
class FakeRelationService final : public IRelationService
{
public:
    void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override
    {
        out["bootstrap_uid"] = uid;
        out["friend_list"] = memochat::json::array_t{};
        out["friend_list"].append("friend-a");
    }

    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override
    {
        out["dialog_uid"] = uid;
        out["dialogs"] = memochat::json::array_t{};
        out["dialogs"].append("dialog-a");
    }

    RelationCommandResult SearchUser(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_SEARCH_USER_RSP, R"({"error":0,"user_id":"alice"})"};
    }

    RelationCommandResult AddFriendApply(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_ADD_FRIEND_RSP, R"({"error":0,"applied":true})"};
    }

    RelationCommandResult AuthFriendApply(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_AUTH_FRIEND_RSP, R"({"error":0,"accepted":true})"};
    }

    RelationCommandResult DeleteFriend(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_DELETE_FRIEND_RSP, R"({"error":0,"deleted":true})"};
    }

    RelationCommandResult GetDialogList(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_GET_DIALOG_LIST_RSP, R"({"error":0,"dialogs":[]})"};
    }

    RelationCommandResult SyncDraft(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_SYNC_DRAFT_RSP, R"({"error":0,"draft_text":"hello"})"};
    }

    RelationCommandResult PinDialog(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {ID_PIN_DIALOG_RSP, R"({"error":0,"pinned_rank":10})"};
    }

    RelationCommandResult FilterFriendUids(const RelationCommandRequest& request) override
    {
        last_request = request;
        return {0, R"({"error":0,"friend_uids":[]})"};
    }

    RelationCommandRequest last_request;
};

memochat::json::JsonValue ParsePayload(const std::string& payload)
{
    memochat::json::JsonCharReaderBuilder builder;
    std::unique_ptr<memochat::json::JsonCharReader> reader(builder.newCharReader());
    memochat::json::JsonValue root;
    std::string errors;
    EXPECT_TRUE(reader->parse(payload.data(), payload.data() + payload.size(), &root, &errors)) << errors;
    return root;
}
} // namespace

TEST(ChatRelationInternalGrpcServiceTest, AppendRelationBootstrapBuildsBootstrapResponse)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake);

    chatinternal::BootstrapRequest request;
    request.set_uid(42);
    request.set_trace_id("trace-relation");
    chatinternal::BootstrapResponse response;

    const auto status = service.AppendRelationBootstrap(nullptr, &request, &response);

    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(response.error(), ErrorCodes::Success);
    const auto payload = ParsePayload(response.payload_json());
    EXPECT_EQ(payload["uid"].asInt(), 42);
    EXPECT_EQ(payload["bootstrap_uid"].asInt(), 42);
    ASSERT_TRUE(payload["friend_list"].isArray());
    EXPECT_EQ(payload["friend_list"][0].asString(), "friend-a");
}

TEST(ChatRelationInternalGrpcServiceTest, BuildDialogListBuildsBootstrapResponse)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake);

    chatinternal::BootstrapRequest request;
    request.set_uid(7);
    chatinternal::BootstrapResponse response;

    const auto status = service.BuildDialogList(nullptr, &request, &response);

    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(response.error(), ErrorCodes::Success);
    const auto payload = ParsePayload(response.payload_json());
    EXPECT_EQ(payload["uid"].asInt(), 7);
    EXPECT_EQ(payload["dialog_uid"].asInt(), 7);
    ASSERT_TRUE(payload["dialogs"].isArray());
    EXPECT_EQ(payload["dialogs"][0].asString(), "dialog-a");
}

TEST(ChatRelationInternalGrpcServiceTest, InvalidUidReturnsInvalidArgument)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake);

    chatinternal::BootstrapRequest request;
    request.set_uid(0);
    chatinternal::BootstrapResponse response;

    const auto status = service.AppendRelationBootstrap(nullptr, &request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(response.error(), ErrorCodes::UidInvalid);
}

TEST(ChatRelationInternalGrpcServiceTest, MissingRelationServiceReturnsFailedPrecondition)
{
    ChatRelationInternalGrpcService service(nullptr);

    chatinternal::BootstrapRequest request;
    request.set_uid(42);
    chatinternal::BootstrapResponse response;

    const auto status = service.AppendRelationBootstrap(nullptr, &request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
}

TEST(ChatRelationInternalGrpcServiceTest, SearchUserDelegatesToRelationCommandService)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake);

    chatinternal::JsonPayloadRequest request;
    request.set_tcp_msg_id(ID_SEARCH_USER_REQ);
    request.set_payload_json(R"({"user_id":"alice"})");
    request.mutable_session()->set_uid(42);
    request.mutable_session()->set_session_id("session-42");
    request.mutable_session()->set_server_name("ChatServer-1");
    request.mutable_session()->set_trace_id("trace-relation-command");
    chatinternal::JsonPayloadResponse response;

    const auto status = service.SearchUser(nullptr, &request, &response);

    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(response.error(), ErrorCodes::Success);
    EXPECT_EQ(response.tcp_msg_id(), ID_SEARCH_USER_RSP);
    EXPECT_EQ(response.payload_json(), R"({"error":0,"user_id":"alice"})");
    EXPECT_EQ(fake.last_request.request_msg_id, ID_SEARCH_USER_REQ);
    EXPECT_EQ(fake.last_request.payload_json, R"({"user_id":"alice"})");
    EXPECT_EQ(fake.last_request.session_uid, 42);
    EXPECT_EQ(fake.last_request.session_id, "session-42");
    EXPECT_EQ(fake.last_request.server_name, "ChatServer-1");
    EXPECT_EQ(fake.last_request.trace_id, "trace-relation-command");
}

TEST(ChatRelationInternalGrpcServiceTest, MissingRelationCommandServiceReturnsFailedPrecondition)
{
    ChatRelationInternalGrpcService service(nullptr);

    chatinternal::JsonPayloadRequest request;
    request.set_tcp_msg_id(ID_SEARCH_USER_REQ);
    request.set_payload_json(R"({"user_id":"alice"})");
    chatinternal::JsonPayloadResponse response;

    const auto status = service.SearchUser(nullptr, &request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
    EXPECT_EQ(response.error(), ErrorCodes::RPCFailed);
}
