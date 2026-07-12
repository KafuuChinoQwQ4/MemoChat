#include <gtest/gtest.h>

#include "MessageGrpcServiceAdapter.hpp"
#include "MessageServiceFactory.hpp"

#include <memory>
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

    std::string private_error;
    std::string group_error;
    EXPECT_EQ(CreatePrivateMessageService(config, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &private_error),
              nullptr);
    EXPECT_EQ(CreateGroupMessageService(config, nullptr, nullptr, nullptr, nullptr, &group_error), nullptr);
    EXPECT_EQ(private_error, "Message service remote endpoint is empty: grpc");
    EXPECT_EQ(group_error, "Message service remote endpoint is empty: grpc");
}
