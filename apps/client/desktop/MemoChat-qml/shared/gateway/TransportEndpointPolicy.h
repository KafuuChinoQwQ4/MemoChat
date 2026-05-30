#ifndef TRANSPORTENDPOINTPOLICY_H
#define TRANSPORTENDPOINTPOLICY_H

#include <QVector>

#include "global.h"

QString transportKindName(ChatTransportKind kind);
ChatEndpoint resolveChatEndpoint(const ServerInfo& serverInfo, ChatTransportKind kind);
QVector<ChatEndpoint> buildCandidateChatEndpoints(const ServerInfo& serverInfo);

#endif // TRANSPORTENDPOINTPOLICY_H
