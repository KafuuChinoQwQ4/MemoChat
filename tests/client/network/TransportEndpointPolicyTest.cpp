#include "TransportEndpointPolicy.h"

#include <gtest/gtest.h>

namespace
{
ChatEndpoint endpoint(ChatTransportKind kind, const QString& host, const QString& port, const QString& serverName = {})
{
    ChatEndpoint item;
    item.transport = kind;
    item.host = host;
    item.port = port;
    item.serverName = serverName;
    return item;
}
} // namespace

TEST(TransportEndpointPolicyTest, OrdersPreferredThenFallbackThenRemaining)
{
    ServerInfo info;
    info.PreferredTransport = ChatTransportKind::Quic;
    info.FallbackTransport = ChatTransportKind::Tcp;
    info.Endpoints = {
        endpoint(ChatTransportKind::Tcp,
                 QStringLiteral("10.0.0.2"), QStringLiteral("9001")),
                 endpoint(ChatTransportKind::Quic,
                          QStringLiteral("10.0.0.1"), QStringLiteral("9000")),
                          endpoint(ChatTransportKind::Tcp, QStringLiteral("10.0.0.3"), QStringLiteral("9002")),
                          };

    const QVector<ChatEndpoint> candidates = buildCandidateChatEndpoints(info);

    ASSERT_EQ(candidates.size(), 3);
    EXPECT_EQ(candidates[0].transport, ChatTransportKind::Quic);
    EXPECT_EQ(candidates[0].host, QStringLiteral("10.0.0.1"));
    EXPECT_EQ(candidates[1].transport, ChatTransportKind::Tcp);
    EXPECT_EQ(candidates[1].host, QStringLiteral("10.0.0.2"));
    EXPECT_EQ(candidates[2].transport, ChatTransportKind::Tcp);
    EXPECT_EQ(candidates[2].host, QStringLiteral("10.0.0.3"));
}

TEST(TransportEndpointPolicyTest, DeduplicatesEndpointByTransportHostAndPort)
{
    ServerInfo info;
    info.PreferredTransport = ChatTransportKind::Quic;
    info.FallbackTransport = ChatTransportKind::Tcp;
    info.Endpoints = {
        endpoint(ChatTransportKind::Quic,
                 QStringLiteral("HOST"), QStringLiteral("9000")),
                 endpoint(ChatTransportKind::Quic,
                          QStringLiteral("host"), QStringLiteral("9000")),
                          endpoint(ChatTransportKind::Tcp, QStringLiteral("host"), QStringLiteral("9000")),
                          };

    const QVector<ChatEndpoint> candidates = buildCandidateChatEndpoints(info);

    ASSERT_EQ(candidates.size(), 2);
    EXPECT_EQ(candidates[0].transport, ChatTransportKind::Quic);
    EXPECT_EQ(candidates[1].transport, ChatTransportKind::Tcp);
}

TEST(TransportEndpointPolicyTest, UsesLegacyHostPortWhenNoEndpointListExists)
{
    ServerInfo info;
    info.Host = QStringLiteral("127.0.0.1");
    info.Port = QStringLiteral("9001");
    info.ServerName = QStringLiteral("chat-local");
    info.PreferredTransport = ChatTransportKind::Tcp;

    const QVector<ChatEndpoint> candidates = buildCandidateChatEndpoints(info);

    ASSERT_EQ(candidates.size(), 1);
    EXPECT_EQ(candidates[0].transport, ChatTransportKind::Tcp);
    EXPECT_EQ(candidates[0].host, info.Host);
    EXPECT_EQ(candidates[0].port, info.Port);
    EXPECT_EQ(candidates[0].serverName, info.ServerName);
}

TEST(TransportEndpointPolicyTest, ResolvesExistingEndpointOrLegacyFallback)
{
    ServerInfo info;
    info.Host = QStringLiteral("legacy-host");
    info.Port = QStringLiteral("9001");
    info.ServerName = QStringLiteral("legacy-server");
    info.Endpoints = {
        endpoint(ChatTransportKind::Quic,
                 QStringLiteral("quic-host"), QStringLiteral("9000"), QStringLiteral("quic-server")),
        };

    const ChatEndpoint quic = resolveChatEndpoint(info, ChatTransportKind::Quic);
    EXPECT_EQ(quic.host, QStringLiteral("quic-host"));
    EXPECT_EQ(quic.port, QStringLiteral("9000"));
    EXPECT_EQ(quic.serverName, QStringLiteral("quic-server"));

    const ChatEndpoint tcp = resolveChatEndpoint(info, ChatTransportKind::Tcp);
    EXPECT_EQ(tcp.transport, ChatTransportKind::Tcp);
    EXPECT_EQ(tcp.host, info.Host);
    EXPECT_EQ(tcp.port, info.Port);
    EXPECT_EQ(tcp.serverName, info.ServerName);
}

TEST(TransportEndpointPolicyTest, NamesTransportKinds)
{
    EXPECT_EQ(transportKindName(ChatTransportKind::Quic), QStringLiteral("quic"));
    EXPECT_EQ(transportKindName(ChatTransportKind::Tcp), QStringLiteral("tcp"));
}
