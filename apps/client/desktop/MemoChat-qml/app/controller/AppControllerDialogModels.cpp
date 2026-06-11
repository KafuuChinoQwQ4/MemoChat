#include "AppController.h"
#include "usermgr.h"

#include <QDateTime>
#include <QDebug>

void AppController::refreshDialogModel()
{
    const auto chats = _gateway.userMgr()->GetFriendListSnapshot();
    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    _features.chatFeatureController.replaceDialogListFromSnapshots(chats, groups);
    syncCurrentDialogDraft();
}

void AppController::refreshDialogModelIncremental()
{
    const qint64 startTs = QDateTime::currentMSecsSinceEpoch();

    const auto chats = _gateway.userMgr()->GetFriendListSnapshot();
    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    const ChatDialogSnapshotRefreshResult result =
        _features.chatFeatureController.upsertDialogListFromSnapshots(chats, groups);
    syncCurrentDialogDraft();

    const qint64 endTs = QDateTime::currentMSecsSinceEpoch();
    qInfo() << "[PERF] refreshDialogModelIncremental - chats:" << chats.size() << "| groups:" << groups.size()
            << "| dialogs:" << result.totalCount << "| total:" << (endTs - startTs) << "ms";
}
