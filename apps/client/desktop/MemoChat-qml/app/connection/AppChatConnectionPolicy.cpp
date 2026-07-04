#include "AppChatConnectionPolicy.h"

namespace
{
bool findEndpoint(const QVector<ChatEndpoint>& endpoints, ChatTransportKind kind, ChatEndpoint& endpoint)
{
    for (const auto& candidate : endpoints)
    {
        if (candidate.transport == kind && !candidate.host.trimmed().isEmpty() && !candidate.port.trimmed().isEmpty())
        {
            endpoint = candidate;
            return true;
        }
    }
    return false;
}

bool invalidChatTicket(const AppChatConnectionPolicy::AppChatConnectionSnapshot& snapshot)
{
    return snapshot.protocolVersion != 3 || snapshot.loginTicket.trimmed().isEmpty();
}

ServerInfo buildServerInfo(const AppChatConnectionPolicy::AppChatConnectionSnapshot& snapshot,
                           const ChatEndpoint& endpoint,
                           ChatTransportKind preferredTransport,
                           ChatTransportKind fallbackTransport)
{
    ServerInfo serverInfo;
    serverInfo.Uid = snapshot.uid;
    serverInfo.Token = snapshot.token;
    serverInfo.LoginTicket = snapshot.loginTicket;
    serverInfo.ProtocolVersion = snapshot.protocolVersion;
    serverInfo.Endpoints = snapshot.endpoints;
    serverInfo.PreferredTransport = preferredTransport;
    serverInfo.FallbackTransport = fallbackTransport;
    serverInfo.ConnectTimeoutMs = snapshot.connectTimeoutMs;
    serverInfo.BackupDialDelayMs = snapshot.backupDialDelayMs;
    serverInfo.TotalLoginTimeoutMs = snapshot.totalLoginTimeoutMs;
    serverInfo.Host = endpoint.host;
    serverInfo.Port = endpoint.port;
    serverInfo.ServerName = endpoint.serverName;
    return serverInfo;
}
} // namespace

bool AppChatConnectionPolicy::hasEndpointKind(const QVector<ChatEndpoint>& endpoints, ChatTransportKind kind)
{
    ChatEndpoint endpoint;
    return findEndpoint(endpoints, kind, endpoint);
}

AppChatConnectionPolicy::AppChatConnectionDecision
AppChatConnectionPolicy::evaluateLoginTcpFallback(const AppChatConnectionSnapshot& snapshot)
{
    AppChatConnectionDecision decision;
    if (snapshot.loginTcpFallbackAttempted || !snapshot.isLoginPage || !snapshot.busy || snapshot.uid <= 0 ||
        invalidChatTicket(snapshot))
    {
        return decision;
    }

    ChatEndpoint tcpEndpoint;
    if (!findEndpoint(snapshot.endpoints, ChatTransportKind::Tcp, tcpEndpoint))
    {
        return decision;
    }

    decision.allowed = true;
    decision.selectedEndpoint = tcpEndpoint;
    decision.serverInfo = buildServerInfo(snapshot, tcpEndpoint, ChatTransportKind::Tcp, ChatTransportKind::Tcp);
    return decision;
}

AppChatConnectionPolicy::AppChatConnectionDecision
AppChatConnectionPolicy::evaluateReconnect(const AppChatConnectionSnapshot& snapshot)
{
    AppChatConnectionDecision decision;
    if (!snapshot.isChatPage || snapshot.uid <= 0 || invalidChatTicket(snapshot) || snapshot.endpoints.isEmpty() ||
        snapshot.reconnectAttempts >= kChatReconnectMaxAttempts)
    {
        return decision;
    }

    const ChatTransportKind preferredTransport =
        hasEndpointKind(snapshot.endpoints, ChatTransportKind::Quic) ? ChatTransportKind::Quic : ChatTransportKind::Tcp;

    ChatEndpoint selectedEndpoint;
    if (!findEndpoint(snapshot.endpoints, preferredTransport, selectedEndpoint) &&
        !findEndpoint(snapshot.endpoints, ChatTransportKind::Tcp, selectedEndpoint))
    {
        return decision;
    }

    decision.allowed = true;
    decision.selectedEndpoint = selectedEndpoint;
    decision.serverInfo = buildServerInfo(snapshot, selectedEndpoint, preferredTransport, ChatTransportKind::Tcp);
    return decision;
}
