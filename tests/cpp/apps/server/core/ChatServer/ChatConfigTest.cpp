#include "ChatSessionConfig.hpp"
#include "ConfigMgr.hpp"
#include "MessageServiceConfig.hpp"
#include "RelationQueryServiceConfig.hpp"
#include "RelationServiceConfig.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <mutex>
#include <string>

namespace
{
void EnsureConfigFixture()
{
    static std::once_flag once;
    std::call_once(once,
                   []
                   {
                       const std::string path = "/tmp/memochat_chat_config_module_test.ini";
                       std::ofstream out(path);
                       out << "[ChatAuth]\n";
                       out << "HmacSecret = test-chat-secret\n";
                       out << "[FeatureFlags]\n";
                       out << "EnabledMixed = YeS\n";
                       out << "DisabledMixed = Off\n";
                       out << "UnknownValue = maybe\n";
                       out << "[LogicSystem]\n";
                       out << "HeartbeatRouteRefreshSec = 999\n";
                       out << "LoginOfflinePushDelayMs = -5\n";
                       out << "[SelfServer]\n";
                       out << "Name = chat-node-a\n";
                       out << "[MessageService]\n";
                       out << "Backend = GRPC\n";
                       out << "Endpoint = 127.0.0.1:50092\n";
                       out << "[RelationService]\n";
                       out << "Backend = Remote\n";
                       out << "Endpoint = 127.0.0.1:50091\n";
                       out << "[RelationQueryService]\n";
                       out << "Backend = GRPC\n";
                       out << "Endpoint = 127.0.0.1:50090\n";
                       out.close();

                       ConfigMgr::InitConfigPath(path);
                   });
}
} // namespace

TEST(ChatConfigTest, SessionConfigUsesImportedAlgorithmsForFlagsAndClampedInts)
{
    EnsureConfigFixture();

    const ChatSessionConfig config;

    EXPECT_EQ(config.ChatAuthSecret(), "test-chat-secret");
    EXPECT_TRUE(config.FeatureFlagDefaultTrue("EnabledMixed"));
    EXPECT_FALSE(config.FeatureFlagDefaultTrue("DisabledMixed"));
    EXPECT_FALSE(config.FeatureFlagDefaultTrue("UnknownValue"));
    EXPECT_TRUE(config.FeatureFlagDefaultTrue("MissingFlag"));
    EXPECT_EQ(config.HeartbeatRouteRefreshSec(), 300);
    EXPECT_EQ(config.LoginOfflinePushDelayMs(), 0);
    EXPECT_EQ(config.SelfServerName(), "chat-node-a");
}

TEST(ChatConfigTest, ServiceBackendsAreLowercasedThroughImportedAlgorithms)
{
    EnsureConfigFixture();

    const MessageServiceConfig message_config;
    const RelationServiceConfig relation_config;
    const RelationQueryServiceConfig relation_query_config;

    EXPECT_EQ(message_config.MessageServiceBackend(), "grpc");
    EXPECT_EQ(message_config.MessageServiceEndpoint(), "127.0.0.1:50092");
    EXPECT_EQ(relation_config.RelationServiceBackend(), "remote");
    EXPECT_EQ(relation_config.RelationServiceEndpoint(), "127.0.0.1:50091");
    EXPECT_EQ(relation_query_config.RelationQueryServiceBackend(), "grpc");
    EXPECT_EQ(relation_query_config.RelationQueryServiceEndpoint(), "127.0.0.1:50090");
}
