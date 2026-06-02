#include "AppChatConnectionPolicy.h"

#include <gtest/gtest.h>

namespace
{
ChatEndpoint endpoint(ChatTransportKind kind, const QString& host, const QString& port, const QString& serverName = {})
{
    ChatEndpoint value;
    value.transport = kind;
    value.host = host;
    value.port = port;
    value.serverName = serverName;
    return value;
}

AppChatConnectionPolicy::AppChatConnectionSnapshot validFallbackInput()
{
    AppChatConnectionPolicy::AppChatConnectionSnapshot input;
    input.isLoginPage = true;
    input.busy = true;
    input.uid = 42;
    input.token = QStringLiteral("token");
    input.loginTicket = QStringLiteral("ticket");
    input.protocolVersion = 3;
    input.endpoints = {endpoint(ChatTransportKind::Quic,
                                QStringLiteral("quic.local"), QStringLiteral("443")),
                                endpoint(ChatTransportKind::Tcp, QStringLiteral("tcp.local"), QStringLiteral("9001"))};
    input.connectTimeoutMs = 111;
    input.backupDialDelayMs = 222;
    input.totalLoginTimeoutMs = 333;
    return input;
}

AppChatConnectionPolicy::AppChatConnectionSnapshot validReconnectInput()
{
    AppChatConnectionPolicy::AppChatConnectionSnapshot input;
    input.isChatPage = true;
    input.uid = 42;
    input.token = QStringLiteral("token");
    input.loginTicket = QStringLiteral("ticket");
    input.protocolVersion = 3;
    input.endpoints = {endpoint(ChatTransportKind::Quic,
                                QStringLiteral("quic.local"), QStringLiteral("443")),
                                endpoint(ChatTransportKind::Tcp, QStringLiteral("tcp.local"), QStringLiteral("9001"))};
    input.connectTimeoutMs = 111;
    input.backupDialDelayMs = 222;
    input.totalLoginTimeoutMs = 333;
    return input;
}
} // namespace

TEST(AppChatConnectionPolicyTest, LoginTcpFallbackIsFalseAfterOneFallbackAttempt)
{
    auto input = validFallbackInput();
    input.loginTcpFallbackAttempted = true;

    const auto decision = AppChatConnectionPolicy::evaluateLoginTcpFallback(input);

    EXPECT_FALSE(decision.allowed);
}

TEST(AppChatConnectionPolicyTest, LoginTcpFallbackIsFalseOutsideLoginPageOrWhenNotBusy)
{
    auto input = validFallbackInput();
    input.isLoginPage = false;
    EXPECT_FALSE(AppChatConnectionPolicy::evaluateLoginTcpFallback(input).allowed);

    input = validFallbackInput();
    input.busy = false;
    EXPECT_FALSE(AppChatConnectionPolicy::evaluateLoginTcpFallback(input).allowed);
}

TEST(AppChatConnectionPolicyTest, LoginTcpFallbackIsFalseWithNoTcpEndpoint)
{
    auto input = validFallbackInput();
    input.endpoints = {endpoint(ChatTransportKind::Quic, QStringLiteral("quic.local"), QStringLiteral("443"))};

    const auto decision = AppChatConnectionPolicy::evaluateLoginTcpFallback(input);

    EXPECT_FALSE(decision.allowed);
}

TEST(AppChatConnectionPolicyTest, LoginTcpFallbackIsFalseWithMissingCredential)
{
    auto input = validFallbackInput();
    input.loginTicket.clear();
    EXPECT_FALSE(AppChatConnectionPolicy::evaluateLoginTcpFallback(input).allowed);

    input = validFallbackInput();
    input.protocolVersion = 2;
    input.token.clear();
    EXPECT_FALSE(AppChatConnectionPolicy::evaluateLoginTcpFallback(input).allowed);
}

TEST(AppChatConnectionPolicyTest, LoginTcpFallbackBuildsTcpServerInfo)
{
    const auto decision = AppChatConnectionPolicy::evaluateLoginTcpFallback(validFallbackInput());

    ASSERT_TRUE(decision.allowed);
    EXPECT_EQ(decision.serverInfo.PreferredTransport, ChatTransportKind::Tcp);
    EXPECT_EQ(decision.serverInfo.FallbackTransport, ChatTransportKind::Tcp);
    EXPECT_EQ(decision.serverInfo.Host, QStringLiteral("tcp.local"));
    EXPECT_EQ(decision.serverInfo.Port, QStringLiteral("9001"));
    EXPECT_EQ(decision.serverInfo.ConnectTimeoutMs, 111);
    EXPECT_EQ(decision.serverInfo.BackupDialDelayMs, 222);
    EXPECT_EQ(decision.serverInfo.TotalLoginTimeoutMs, 333);
}

TEST(AppChatConnectionPolicyTest, ReconnectIsFalseOutsideChatPage)
{
    auto input = validReconnectInput();
    input.isChatPage = false;

    const auto decision = AppChatConnectionPolicy::evaluateReconnect(input);

    EXPECT_FALSE(decision.allowed);
}

TEST(AppChatConnectionPolicyTest, ReconnectIsFalseAfterMaxAttempts)
{
    auto input = validReconnectInput();
    input.reconnectAttempts = AppChatConnectionPolicy::kChatReconnectMaxAttempts;

    const auto decision = AppChatConnectionPolicy::evaluateReconnect(input);

    EXPECT_FALSE(decision.allowed);
}

TEST(AppChatConnectionPolicyTest, ReconnectPrefersQuicWhenAvailableOtherwiseTcp)
{
    auto decision = AppChatConnectionPolicy::evaluateReconnect(validReconnectInput());

    ASSERT_TRUE(decision.allowed);
    EXPECT_EQ(decision.serverInfo.PreferredTransport, ChatTransportKind::Quic);
    EXPECT_EQ(decision.serverInfo.FallbackTransport, ChatTransportKind::Tcp);
    EXPECT_EQ(decision.serverInfo.Host, QStringLiteral("quic.local"));
    EXPECT_EQ(decision.serverInfo.Port, QStringLiteral("443"));

    auto input = validReconnectInput();
    input.endpoints = {endpoint(ChatTransportKind::Tcp, QStringLiteral("tcp.local"), QStringLiteral("9001"))};
    decision = AppChatConnectionPolicy::evaluateReconnect(input);

    ASSERT_TRUE(decision.allowed);
    EXPECT_EQ(decision.serverInfo.PreferredTransport, ChatTransportKind::Tcp);
    EXPECT_EQ(decision.serverInfo.Host, QStringLiteral("tcp.local"));
    EXPECT_EQ(decision.serverInfo.Port, QStringLiteral("9001"));
}
