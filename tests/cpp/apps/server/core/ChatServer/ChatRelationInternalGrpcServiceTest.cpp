#include <gtest/gtest.h>

#include "ChatRelationInternalGrpcService.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"

namespace
{
class FakeRelationService final : public IRelationService
{
public:
    bool AreUsersFriends(int uid, int peer_uid) override
    {
        last_friendship_uid = uid;
        last_friendship_peer_uid = peer_uid;
        return friendship_result;
    }

    bool CheckHealth(std::string* error) override
    {
        if (error != nullptr)
        {
            *error = health_error;
        }
        return health_result;
    }

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
    int last_friendship_uid = 0;
    int last_friendship_peer_uid = 0;
    bool friendship_result = false;
    bool health_result = true;
    std::string health_error;
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
    ChatRelationInternalGrpcService service(&fake,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

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
    ChatRelationInternalGrpcService service(&fake,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

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

TEST(ChatRelationInternalGrpcServiceTest, CheckFriendshipUsesStrictRelationQuery)
{
    FakeRelationService fake;
    fake.friendship_result = true;
    ChatRelationInternalGrpcService service(&fake,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

    chatinternal::FriendshipRequest request;
    request.set_uid(42);
    request.set_peer_uid(84);
    chatinternal::FriendshipResponse response;

    const auto status = service.CheckFriendship(nullptr, &request, &response);

    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(response.are_friends());
    EXPECT_EQ(fake.last_friendship_uid, 42);
    EXPECT_EQ(fake.last_friendship_peer_uid, 84);
}

TEST(ChatRelationInternalGrpcServiceTest, CheckFriendshipRejectsInvalidPairs)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

    chatinternal::FriendshipRequest request;
    request.set_uid(42);
    request.set_peer_uid(42);
    chatinternal::FriendshipResponse response;

    const auto status = service.CheckFriendship(nullptr, &request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(fake.last_friendship_uid, 0);
}

TEST(ChatRelationInternalGrpcServiceTest, CheckHealthDelegatesToRelationDependency)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

    chatinternal::RelationHealthRequest request;
    chatinternal::RelationHealthResponse response;
    const auto status = service.CheckHealth(nullptr, &request, &response);

    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(response.ready());
}

TEST(ChatRelationInternalGrpcServiceTest, CheckHealthFailsWhenRelationDependencyIsUnavailable)
{
    FakeRelationService fake;
    fake.health_result = false;
    fake.health_error = "database unavailable";
    ChatRelationInternalGrpcService service(&fake,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

    chatinternal::RelationHealthRequest request;
    chatinternal::RelationHealthResponse response;
    const auto status = service.CheckHealth(nullptr, &request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
    EXPECT_FALSE(response.ready());
    EXPECT_EQ(response.error(), "relation dependency unavailable");
}

TEST(ChatRelationInternalGrpcServiceTest, InvalidUidReturnsInvalidArgument)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

    chatinternal::BootstrapRequest request;
    request.set_uid(0);
    chatinternal::BootstrapResponse response;

    const auto status = service.AppendRelationBootstrap(nullptr, &request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_EQ(response.error(), ErrorCodes::UidInvalid);
}

TEST(ChatRelationInternalGrpcServiceTest, MissingRelationServiceReturnsFailedPrecondition)
{
    ChatRelationInternalGrpcService service(nullptr,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

    chatinternal::BootstrapRequest request;
    request.set_uid(42);
    chatinternal::BootstrapResponse response;

    const auto status = service.AppendRelationBootstrap(nullptr, &request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
}

TEST(ChatRelationInternalGrpcServiceTest, SearchUserDelegatesToRelationCommandService)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

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
    ChatRelationInternalGrpcService service(nullptr,
                                            RelationGrpcAccessMode::ReadWrite,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);

    chatinternal::JsonPayloadRequest request;
    request.set_tcp_msg_id(ID_SEARCH_USER_REQ);
    request.set_payload_json(R"({"user_id":"alice"})");
    chatinternal::JsonPayloadResponse response;

    const auto status = service.SearchUser(nullptr, &request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
    EXPECT_EQ(response.error(), ErrorCodes::RPCFailed);
}

TEST(ChatRelationInternalGrpcServiceTest, QueryOnlyModeRejectsCommandSurfaceButKeepsVisibilityQuery)
{
    FakeRelationService fake;
    ChatRelationInternalGrpcService service(&fake,
                                            RelationGrpcAccessMode::QueryOnly,
                                            {},
                                            RelationGrpcAuthMode::DisabledForTests);
    chatinternal::JsonPayloadRequest request;
    request.set_tcp_msg_id(999);
    request.set_payload_json(R"({"viewer_uid":42,"author_uids":[84]})");
    request.mutable_session()->set_uid(42);

    using RestrictedCommand = grpc::Status (ChatRelationInternalGrpcService::*)(grpc::ServerContext*,
                                                                                const chatinternal::JsonPayloadRequest*,
                                                                                chatinternal::JsonPayloadResponse*);
    const RestrictedCommand restricted_commands[] = {
        &ChatRelationInternalGrpcService::SearchUser,
        &ChatRelationInternalGrpcService::AddFriendApply,
        &ChatRelationInternalGrpcService::AuthFriendApply,
        &ChatRelationInternalGrpcService::DeleteFriend,
        &ChatRelationInternalGrpcService::GetDialogList,
        &ChatRelationInternalGrpcService::SyncDraft,
        &ChatRelationInternalGrpcService::PinDialog,
    };
    for (const auto command : restricted_commands)
    {
        chatinternal::JsonPayloadResponse response;
        const auto status = (service.*command)(nullptr, &request, &response);
        EXPECT_EQ(status.error_code(), grpc::StatusCode::PERMISSION_DENIED);
        EXPECT_EQ(response.error(), ErrorCodes::RPCFailed);
    }
    EXPECT_TRUE(fake.last_request.payload_json.empty());

    chatinternal::JsonPayloadResponse filter_response;
    const auto filter_status = service.FilterFriendUids(nullptr, &request, &filter_response);
    ASSERT_TRUE(filter_status.ok()) << filter_status.error_message();
    EXPECT_EQ(fake.last_request.payload_json, request.payload_json());
}
