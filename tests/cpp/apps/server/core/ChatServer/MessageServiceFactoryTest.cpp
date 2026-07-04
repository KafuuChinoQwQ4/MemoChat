#include <gtest/gtest.h>

#include "MessageGrpcServiceAdapter.hpp"
#include "MessageServiceFactory.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
class FakeMessageServiceConfig final : public IMessageServiceConfig
{
public:
    FakeMessageServiceConfig(std::string backend, std::string endpoint)
        : backend_(std::move(backend))
        , endpoint_(std::move(endpoint))
    {
    }

    std::string MessageServiceBackend() const override
    {
        return backend_;
    }

    std::string MessageServiceEndpoint() const override
    {
        return endpoint_;
    }

private:
    std::string backend_;
    std::string endpoint_;
};
} // namespace

TEST(MessageServiceFactoryTest, GrpcBackendCreatesRemotePrivateMessageAdapter)
{
    FakeMessageServiceConfig config("grpc", "127.0.0.1:50092");

    auto selected = CreatePrivateMessageService(config, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    ASSERT_NE(selected, nullptr);
    EXPECT_NE(dynamic_cast<MessageGrpcServiceAdapter*>(selected.get()), nullptr);
}

TEST(MessageServiceFactoryTest, RemoteBackendCreatesRemoteGroupMessageAdapter)
{
    FakeMessageServiceConfig config("remote", "127.0.0.1:50092");

    auto selected = CreateGroupMessageService(config, nullptr, nullptr, nullptr, nullptr);

    ASSERT_NE(selected, nullptr);
    EXPECT_NE(dynamic_cast<MessageGrpcServiceAdapter*>(selected.get()), nullptr);
}

TEST(MessageServiceFactoryTest, RemoteBackendRequiresEndpoint)
{
    FakeMessageServiceConfig config("grpc", "");

    EXPECT_THROW((void) CreatePrivateMessageService(config, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr),
                 std::runtime_error);
    EXPECT_THROW((void) CreateGroupMessageService(config, nullptr, nullptr, nullptr, nullptr), std::runtime_error);
}
