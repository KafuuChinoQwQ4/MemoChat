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
    FakeRelationServiceConfig(std::string backend, std::string endpoint)
        : backend_(std::move(backend))
        , endpoint_(std::move(endpoint))
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

private:
    std::string backend_;
    std::string endpoint_;
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
