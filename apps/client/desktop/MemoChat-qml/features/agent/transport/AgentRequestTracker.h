#ifndef AGENTREQUESTTRACKER_H
#define AGENTREQUESTTRACKER_H

#include "AgentRequestTypes.h"
#include "global.h"

#include <QHash>
#include <optional>

class AgentRequestTracker
{
public:
    void track(ReqId id, AgentRequestKind kind, const QString& messageId = QString(), int uid = 0);
    std::optional<AgentRequestRecord> take(ReqId id);
    bool contains(ReqId id) const;
    void clear();

private:
    QHash<int, AgentRequestRecord> _records;
};

#endif // AGENTREQUESTTRACKER_H
