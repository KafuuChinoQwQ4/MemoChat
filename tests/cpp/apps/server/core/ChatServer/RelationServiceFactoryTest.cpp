#include <gtest/gtest.h>

#include "RelationGrpcServiceAdapter.hpp"
#include "RelationServiceFactory.hpp"

#include <memory>
#include <string>
#include <utility>

namespace
{
class FakeRelationServiceConfig final : public IRelationServiceConfig
{
public:
    FakeRelationServiceConfig(std::string backend, std::string endpoint, std::string auth_token = std::string(32, 'c'))
        : backend_(std::move(backend))
        , endpoint_(std::move(endpoint))
        , auth_token_(std::move(auth_token))
    {
    }

    std::string RelationServiceBackend() const override
    {
        return backend_;
    }

    std::string RelationServiceEndpoint() const override
    {
        return endpoint_;
    }

    std::string RelationServiceAuthToken() const override
    {
        return auth_token_;
    }

private:
    std::string backend_;
    std::string endpoint_;
    std::string auth_token_;
};
} // namespace

TEST(RelationServiceFactoryTest, GrpcBackendCreatesRemoteRelationClient)
{
    FakeRelationServiceConfig config("grpc", "127.0.0.1:50091");

    auto selected = CreateRelationService(config, nullptr, nullptr, nullptr, nullptr, nullptr);

    ASSERT_NE(selected, nullptr);
    EXPECT_NE(dynamic_cast<RelationGrpcServiceAdapter*>(selected.get()), nullptr);
}

TEST(RelationServiceFactoryTest, RemoteBackendRequiresEndpoint)
{
    FakeRelationServiceConfig config("remote", "");

    std::string error;
    EXPECT_EQ(CreateRelationService(config, nullptr, nullptr, nullptr, nullptr, nullptr, &error), nullptr);
    EXPECT_EQ(error, "Relation service remote endpoint is empty: remote");
}

TEST(RelationServiceFactoryTest, RemoteBackendRequiresStrongAuthToken)
{
    FakeRelationServiceConfig config("remote", "127.0.0.1:50091", "too-short");

    std::string error;
    EXPECT_EQ(CreateRelationService(config, nullptr, nullptr, nullptr, nullptr, nullptr, &error), nullptr);
    EXPECT_EQ(error, "Relation service auth token must be at least 32 printable ASCII bytes");
}

TEST(RelationServiceFactoryTest, UnsupportedBackendFailsClosed)
{
    FakeRelationServiceConfig config("unexpected", "127.0.0.1:50091");

    std::string error;
    EXPECT_EQ(CreateRelationService(config, nullptr, nullptr, nullptr, nullptr, nullptr, &error), nullptr);
    EXPECT_EQ(error, "Unsupported relation service backend: unexpected");
}
