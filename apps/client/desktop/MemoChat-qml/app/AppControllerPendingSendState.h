#pragma once

#include <QVariantList>
#include <QtGlobal>

struct AppPendingSendQueueState
{
    QVariantList queue;
    int dialogUid = 0;
    int chatUid = 0;
    qint64 groupId = 0;
    int totalCount = 0;

    void reset()
    {
        queue.clear();
        dialogUid = 0;
        chatUid = 0;
        groupId = 0;
        totalCount = 0;
    }
};
