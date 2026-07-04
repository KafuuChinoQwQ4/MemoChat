#include <gtest/gtest.h>

#include "RelationQueryGrpcClient.hpp"
#include "RelationQueryServiceFactory.hpp"
#include "const.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
class FakeRelationQueryServiceConfig final : public IRelationQueryServiceConfig
{
public:
    FakeRelationQueryServiceConfig(std::string backend, std::string endpoint)
        : backend_(std::move(backend))
        , endpoint_(std::move(endpoint))
    {
    }

    std::string RelationQueryServiceBackend() const override
    {
        return backend_;
    }

    std::string RelationQueryServiceEndpoint() const override
    {
        return endpoint_;
    }

private:
    std::string backend_;
    std::string endpoint_;
};

class FakeRelationQueryService final : public IRelationQueryService
{
public:
    void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override
    {
        append_called = true;
        out["bootstrap_uid"] = uid;
        out["friend_list"] = memochat::json::array_t{};
        out["friend_list"].append("fallback-friend");
    }

    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override
    {
        dialog_called = true;
        out["dialog_uid"] = uid;
        out["dialogs"] = memochat::json::array_t{};
        out["dialogs"].append("fallback-dialog");
    }

    bool append_called = false;
    bool dialog_called = false;
};
} // namespace

TEST(RelationQueryServiceFactoryTest, InProcessBackendReturnsSuppliedQueryPort)
{
    FakeRelationQueryServiceConfig config("inprocess", "");
    FakeRelationQueryService inprocess;
    std::unique_ptr<IRelationQueryService> remote;

    auto* selected = SelectRelationQueryService(config, &inprocess, remote);

    EXPECT_EQ(selected, &inprocess);
    EXPECT_EQ(remote, nullptr);
}

TEST(RelationQueryServiceFactoryTest, UnsupportedBackendFallsBackToInProcess)
{
    FakeRelationQueryServiceConfig config("unexpected", "127.0.0.1:1");
    FakeRelationQueryService inprocess;
    std::unique_ptr<IRelationQueryService> remote;

    auto* selected = SelectRelationQueryService(config, &inprocess, remote);

    EXPECT_EQ(selected, &inprocess);
    EXPECT_EQ(remote, nullptr);
}

TEST(RelationQueryServiceFactoryTest, GrpcBackendCreatesOwnedRemoteQueryClient)
{
    FakeRelationQueryServiceConfig config("grpc", "127.0.0.1:50090");
    FakeRelationQueryService inprocess;
    std::unique_ptr<IRelationQueryService> remote;

    auto* selected = SelectRelationQueryService(config, &inprocess, remote);

    ASSERT_NE(selected, nullptr);
    EXPECT_NE(selected, &inprocess);
    ASSERT_NE(remote, nullptr);
    EXPECT_EQ(selected, remote.get());
}

TEST(RelationQueryServiceFactoryTest, RemoteBackendRequiresEndpoint)
{
    FakeRelationQueryServiceConfig config("remote", "");

    EXPECT_THROW((void) CreateRemoteRelationQueryService(config), std::runtime_error);
}

TEST(RelationQueryServiceFactoryTest, GrpcBackendFallsBackToInProcessWhenRemoteQueryFails)
{
    FakeRelationQueryServiceConfig config("grpc", "127.0.0.1:1");
    FakeRelationQueryService inprocess;
    std::unique_ptr<IRelationQueryService> remote;

    auto* selected = SelectRelationQueryService(config, &inprocess, remote);

    ASSERT_NE(selected, nullptr);
    ASSERT_NE(selected, &inprocess);
    ASSERT_NE(remote, nullptr);

    memochat::json::JsonValue bootstrap(memochat::json::object_t{});
    bootstrap["error"] = ErrorCodes::Success;
    bootstrap["uid"] = 42;

    selected->AppendRelationBootstrapJson(42, bootstrap);

    EXPECT_TRUE(inprocess.append_called);
    EXPECT_EQ(bootstrap["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(bootstrap["uid"].asInt(), 42);
    EXPECT_EQ(bootstrap["bootstrap_uid"].asInt(), 42);
    const auto friend_list = bootstrap["friend_list"].get<memochat::json::JsonValue>();
    ASSERT_TRUE(friend_list.isArray()) << bootstrap.toStyledString();
    ASSERT_EQ(friend_list.size(), 1);
    EXPECT_EQ(friend_list[0].asString(), "fallback-friend");
    EXPECT_FALSE(bootstrap.isMember("relation_query_remote_error"));
    EXPECT_FALSE(bootstrap.isMember("relation_query_remote_status_code"));

    memochat::json::JsonValue dialogs(memochat::json::object_t{});
    dialogs["error"] = ErrorCodes::Success;
    dialogs["uid"] = 42;

    selected->BuildDialogListJson(42, dialogs);

    EXPECT_TRUE(inprocess.dialog_called);
    EXPECT_EQ(dialogs["error"].asInt(), ErrorCodes::Success);
    EXPECT_EQ(dialogs["uid"].asInt(), 42);
    EXPECT_EQ(dialogs["dialog_uid"].asInt(), 42);
    const auto dialog_list = dialogs["dialogs"].get<memochat::json::JsonValue>();
    ASSERT_TRUE(dialog_list.isArray()) << dialogs.toStyledString();
    ASSERT_EQ(dialog_list.size(), 1);
    EXPECT_EQ(dialog_list[0].asString(), "fallback-dialog");
    EXPECT_FALSE(dialogs.isMember("relation_query_remote_error"));
    EXPECT_FALSE(dialogs.isMember("relation_query_remote_status_code"));
}
