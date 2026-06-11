#include "ConversationSyncService.h"

#include <QMap>
#include <QtGlobal>
#include <limits>

int ConversationSyncService::makeGroupDialogUid(qint64 groupId)
{
    if (groupId <= 0 || groupId > static_cast<qint64>(std::numeric_limits<int>::max()))
    {
        return 0;
    }
    return -static_cast<int>(groupId);
}

qint64 ConversationSyncService::groupIdForDialogUid(const QMap<int, qint64>& groupUidMap, int dialogUid)
{
    if (dialogUid >= 0)
    {
        return 0;
    }
    const qint64 mappedGroupId = groupUidMap.value(dialogUid, 0);
    if (mappedGroupId > 0)
    {
        return mappedGroupId;
    }
    return -static_cast<qint64>(dialogUid);
}

int ConversationSyncService::resolveGroupDialogUid(QMap<int, qint64>& groupUidMap, qint64 groupId)
{
    if (groupId <= 0)
    {
        return 0;
    }
    for (auto it = groupUidMap.cbegin(); it != groupUidMap.cend(); ++it)
    {
        if (it.value() == groupId)
        {
            return it.key();
        }
    }
    const int dialogUid = makeGroupDialogUid(groupId);
    if (dialogUid == 0)
    {
        return 0;
    }
    groupUidMap.insert(dialogUid, groupId);
    return dialogUid;
}
