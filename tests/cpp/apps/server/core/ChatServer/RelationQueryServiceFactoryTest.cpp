#include <gtest/gtest.h>

#include "RelationQueryGrpcClient.h"
#include "RelationQueryServiceFactory.h"

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
        out["bootstrap_uid"] = uid;
    }

    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override
    {
        out["dialog_uid"] = uid;
    }
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
    EXPECT_NE(dynamic_cast<RelationQueryGrpcClient*>(selected), nullptr);
}

TEST(RelationQueryServiceFactoryTest, RemoteBackendRequiresEndpoint)
{
    FakeRelationQueryServiceConfig config("remote", "");

    EXPECT_THROW((void) CreateRemoteRelationQueryService(config), std::runtime_error);
}
