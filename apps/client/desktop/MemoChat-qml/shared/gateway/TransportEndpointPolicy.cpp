#include "TransportEndpointPolicy.h"

namespace
{
bool sameEndpoint(const ChatEndpoint& a, const ChatEndpoint& b)
{
    return a.transport == b.transport && a.host.compare(b.host, Qt::CaseInsensitive) == 0 && a.port == b.port;
}

void appendUniqueEndpoint(QVector<ChatEndpoint>& endpoints, const ChatEndpoint& endpoint)
{
    if (endpoint.host.trimmed().isEmpty() || endpoint.port.trimmed().isEmpty())
    {
        return;
    }
    for (const auto& existing : endpoints)
    {
        if (sameEndpoint(existing, endpoint))
        {
            return;
        }
    }
    endpoints.push_back(endpoint);
}
} // namespace

QString transportKindName(ChatTransportKind kind)
{
    return kind == ChatTransportKind::Quic ? QStringLiteral("quic") : QStringLiteral("tcp");
}

ChatEndpoint resolveChatEndpoint(const ServerInfo& serverInfo, ChatTransportKind kind)
{
    for (const auto& endpoint : serverInfo.Endpoints)
    {
        if (endpoint.transport == kind)
        {
            return endpoint;
        }
    }

    ChatEndpoint fallback;
    fallback.transport = kind;
    fallback.host = serverInfo.Host;
    fallback.port = serverInfo.Port;
    fallback.serverName = serverInfo.ServerName;
    return fallback;
}

QVector<ChatEndpoint> buildCandidateChatEndpoints(const ServerInfo& serverInfo)
{
    QVector<ChatEndpoint> candidates;
    const auto appendByKind = [&candidates, &serverInfo](ChatTransportKind kind)
    {
        for (const auto& endpoint : serverInfo.Endpoints)
        {
            if (endpoint.transport == kind)
            {
                appendUniqueEndpoint(candidates, endpoint);
            }
        }
    };

    appendByKind(serverInfo.PreferredTransport);
    if (serverInfo.FallbackTransport != serverInfo.PreferredTransport)
    {
        appendByKind(serverInfo.FallbackTransport);
    }
    for (const auto& endpoint : serverInfo.Endpoints)
    {
        appendUniqueEndpoint(candidates, endpoint);
    }

    if (candidates.isEmpty() && !serverInfo.Host.trimmed().isEmpty() && !serverInfo.Port.trimmed().isEmpty())
    {
        ChatEndpoint fallback;
        fallback.transport = serverInfo.PreferredTransport;
        fallback.host = serverInfo.Host;
        fallback.port = serverInfo.Port;
        fallback.serverName = serverInfo.ServerName;
        appendUniqueEndpoint(candidates, fallback);
    }
    return candidates;
}
