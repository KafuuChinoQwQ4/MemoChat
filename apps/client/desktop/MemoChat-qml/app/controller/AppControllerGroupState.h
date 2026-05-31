#pragma once

#include <QHash>
#include <QMap>
#include <QString>
#include <QtGlobal>

struct AppGroupRuntimeState
{
    qint64 currentId = 0;
    QString currentName;
    QString currentCode;
    QMap<int, qint64> dialogUidMap;
    QHash<QString, qint64> pendingMsgGroupMap;
    qint64 historyBeforeSeq = 0;
    bool historyHasMore = true;
};
