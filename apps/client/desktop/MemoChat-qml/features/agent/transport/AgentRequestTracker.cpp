#include "AgentRequestTracker.h"

void AgentRequestTracker::track(ReqId id, AgentRequestKind kind, const QString& messageId, int uid)
{
    _records.insert(static_cast<int>(id), AgentRequestRecord{kind, messageId, uid});
}

std::optional<AgentRequestRecord> AgentRequestTracker::take(ReqId id)
{
    const int key = static_cast<int>(id);
    const auto it = _records.find(key);
    if (it == _records.end())
    {
        return std::nullopt;
    }

    AgentRequestRecord record = it.value();
    _records.erase(it);
    return record;
}

bool AgentRequestTracker::contains(ReqId id) const
{
    return _records.contains(static_cast<int>(id));
}

void AgentRequestTracker::clear()
{
    _records.clear();
}
