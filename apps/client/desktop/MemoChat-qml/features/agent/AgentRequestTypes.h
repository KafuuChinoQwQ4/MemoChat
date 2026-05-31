#ifndef AGENTREQUESTTYPES_H
#define AGENTREQUESTTYPES_H

#include <QString>

enum class AgentRequestKind
{
    ChatMessage,
    ListSessions,
    CreateSession,
    DeleteSession,
    History,
    ModelList,
    ApiProviderRegister,
    ApiProviderDelete,
    Summary,
    Suggest,
    Translate,
    KnowledgeUpload,
    KnowledgeSearch,
    KnowledgeList,
    KnowledgeDelete,
    MemoryList,
    MemoryCreate,
    MemoryDelete,
    TaskList,
    TaskCreate,
    TaskCancel,
    TaskResume
};

struct AgentRequestRecord
{
    AgentRequestKind kind = AgentRequestKind::ChatMessage;
    QString messageId;
};

inline bool isKnowledgeRequest(AgentRequestKind kind)
{
    return kind == AgentRequestKind::KnowledgeUpload || kind == AgentRequestKind::KnowledgeSearch ||
           kind == AgentRequestKind::KnowledgeList || kind == AgentRequestKind::KnowledgeDelete;
}

inline bool isMemoryRequest(AgentRequestKind kind)
{
    return kind == AgentRequestKind::MemoryList || kind == AgentRequestKind::MemoryCreate ||
           kind == AgentRequestKind::MemoryDelete;
}

inline bool isAgentTaskRequest(AgentRequestKind kind)
{
    return kind == AgentRequestKind::TaskList || kind == AgentRequestKind::TaskCreate ||
           kind == AgentRequestKind::TaskCancel || kind == AgentRequestKind::TaskResume;
}

inline bool isApiProviderRequest(AgentRequestKind kind)
{
    return kind == AgentRequestKind::ApiProviderRegister || kind == AgentRequestKind::ApiProviderDelete;
}

inline QString agentRequestFeatureType(AgentRequestKind kind)
{
    switch (kind)
    {
        case AgentRequestKind::Summary:
            return QStringLiteral("summary");
        case AgentRequestKind::Suggest:
            return QStringLiteral("suggest");
        case AgentRequestKind::Translate:
            return QStringLiteral("translate");
        default:
            return QString();
    }
}

#endif // AGENTREQUESTTYPES_H
