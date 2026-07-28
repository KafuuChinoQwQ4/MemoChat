#include <gtest/gtest.h>

#include "RelationQueryGrpcClient.hpp"
#include "RelationQueryServiceFactory.hpp"
#include "const.hpp"

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

namespace
{
class ScopedEnvironmentVariable
{
public:
    ScopedEnvironmentVariable(const char* name, const char* value)
        : name_(name)
    {
        if (const char* previous = std::getenv(name_.c_str()))
        {
            previous_value_ = previous;
            had_previous_ = true;
        }
        if (value == nullptr)
        {
            unsetenv(name_.c_str());
        }
        else
        {
            setenv(name_.c_str(), value, 1);
        }
    }

    ~ScopedEnvironmentVariable()
    {
        if (had_previous_)
        {
            setenv(name_.c_str(), previous_value_.c_str(), 1);
        }
        else
        {
            unsetenv(name_.c_str());
        }
    }

private:
    std::string name_;
    bool had_previous_ = false;
    std::string previous_value_;
};

class FakeRelationQueryServiceConfig final : public IRelationQueryServiceConfig
{
public:
    FakeRelationQueryServiceConfig(std::string backend,
                                   std::string endpoint,
                                   std::string auth_token = std::string(32, 'c'))
        : backend_(std::move(backend))
        , endpoint_(std::move(endpoint))
        , auth_token_(std::move(auth_token))
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

    std::string RelationQueryServiceChatAuthToken() const override
    {
        return auth_token_;
    }

private:
    std::string backend_;
    std::string endpoint_;
    std::string auth_token_;
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

class UnauthenticatedRelationQueryService final : public chatinternal::ChatRelationInternalService::Service
{
public:
    grpc::Status AppendRelationBootstrap(grpc::ServerContext*,
                                         const chatinternal::BootstrapRequest*,
                                         chatinternal::BootstrapResponse*) override
    {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "bad relation test token");
    }
};

struct RunningGrpcServer
{
    int port = 0;
    std::unique_ptr<grpc::Server> server;

    std::string Endpoint() const
    {
        return "127.0.0.1:" + std::to_string(port);
    }
};

RunningGrpcServer StartServer(chatinternal::ChatRelationInternalService::Service* service)
{
    RunningGrpcServer running;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &running.port);
    builder.RegisterService(service);
    running.server = builder.BuildAndStart();
    return running;
}
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

TEST(RelationQueryServiceFactoryTest, UnsupportedBackendFailsClosed)
{
    FakeRelationQueryServiceConfig config("unexpected", "127.0.0.1:1");
    FakeRelationQueryService inprocess;
    std::unique_ptr<IRelationQueryService> remote;

    std::string error;
    auto* selected = SelectRelationQueryService(config, &inprocess, remote, &error);

    EXPECT_EQ(selected, nullptr);
    EXPECT_EQ(remote, nullptr);
    EXPECT_EQ(error, "Unsupported relation query service backend: unexpected");
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

    std::string error;
    EXPECT_EQ(CreateRemoteRelationQueryService(config, &error), nullptr);
    EXPECT_EQ(error, "Relation query service remote endpoint is empty: remote");
}

TEST(RelationQueryServiceFactoryTest, RemoteBackendRequiresStrongAuthToken)
{
    FakeRelationQueryServiceConfig config("remote", "127.0.0.1:50090", "too-short");

    std::string error;
    EXPECT_EQ(CreateRemoteRelationQueryService(config, &error), nullptr);
    EXPECT_EQ(error, "Relation query service chat auth token must be at least 32 printable ASCII bytes");
}

TEST(RelationQueryServiceFactoryTest, GrpcBackendFallsBackOnlyWhenExplicitlyAllowedForDevelopment)
{
    ScopedEnvironmentVariable release_mode("MEMOCHAT_RELEASE_MODE", nullptr);
    ScopedEnvironmentVariable allow_fallback("MEMOCHAT_RELATION_QUERY_ALLOW_INPROCESS_FALLBACK", "1");
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

TEST(RelationQueryServiceFactoryTest, AuthenticationFailureNeverFallsBackToInProcess)
{
    UnauthenticatedRelationQueryService service;
    auto running = StartServer(&service);
    ASSERT_NE(running.server, nullptr);

    FakeRelationQueryServiceConfig config("grpc", running.Endpoint());
    FakeRelationQueryService inprocess;
    std::unique_ptr<IRelationQueryService> remote;
    auto* selected = SelectRelationQueryService(config, &inprocess, remote);
    ASSERT_NE(selected, nullptr);

    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;
    selected->AppendRelationBootstrapJson(42, out);

    EXPECT_FALSE(inprocess.append_called);
    EXPECT_EQ(out["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(out["relation_query_remote_status_code"].asInt(), static_cast<int>(grpc::StatusCode::UNAUTHENTICATED));
    running.server->Shutdown();
}

TEST(RelationQueryServiceFactoryTest, GrpcBackendFailsClosedByDefault)
{
    ScopedEnvironmentVariable release_mode("MEMOCHAT_RELEASE_MODE", nullptr);
    ScopedEnvironmentVariable allow_fallback("MEMOCHAT_RELATION_QUERY_ALLOW_INPROCESS_FALLBACK", nullptr);
    FakeRelationQueryServiceConfig config("grpc", "127.0.0.1:1");
    FakeRelationQueryService inprocess;
    std::unique_ptr<IRelationQueryService> remote;

    auto* selected = SelectRelationQueryService(config, &inprocess, remote);
    ASSERT_NE(selected, nullptr);
    ASSERT_NE(selected, &inprocess);

    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;
    selected->AppendRelationBootstrapJson(42, out);

    EXPECT_FALSE(inprocess.append_called);
    EXPECT_EQ(out["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(out["relation_query_remote_status_code"].asInt(), static_cast<int>(grpc::StatusCode::UNAVAILABLE));
}

TEST(RelationQueryServiceFactoryTest, ReleaseModeRejectsExplicitInProcessFallback)
{
    ScopedEnvironmentVariable release_mode("MEMOCHAT_RELEASE_MODE", "1");
    ScopedEnvironmentVariable allow_fallback("MEMOCHAT_RELATION_QUERY_ALLOW_INPROCESS_FALLBACK", "1");
    FakeRelationQueryServiceConfig config("grpc", "127.0.0.1:1");
    FakeRelationQueryService inprocess;
    std::unique_ptr<IRelationQueryService> remote;

    auto* selected = SelectRelationQueryService(config, &inprocess, remote);
    ASSERT_NE(selected, nullptr);
    ASSERT_NE(selected, &inprocess);

    memochat::json::JsonValue out(memochat::json::object_t{});
    out["error"] = ErrorCodes::Success;
    selected->AppendRelationBootstrapJson(42, out);

    EXPECT_FALSE(inprocess.append_called);
    EXPECT_EQ(out["error"].asInt(), ErrorCodes::RPCFailed);
    EXPECT_EQ(out["relation_query_remote_status_code"].asInt(), static_cast<int>(grpc::StatusCode::UNAVAILABLE));
}
