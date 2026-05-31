#include "AppController.h"
#include "ConversationSyncService.h"

#include <QDebug>

void AppController::selectDialogByUid(int uid)
{
    if (uid == 0)
    {
        return;
    }

    qInfo() << "Selecting dialog by uid:" << uid << "current chat uid:" << _chat_state.uid
            << "current group id:" << _group_state.currentId;

    if (uid > 0)
    {
        selectChatByUid(uid);
        return;
    }

    const int groupIndex = _group_list_model.indexOfUid(uid);
    if (groupIndex >= 0)
    {
        selectGroupIndex(groupIndex);
        return;
    }

    const qint64 groupId = ConversationSyncService::groupIdForDialogUid(_group_state.dialogUidMap, uid);
    if (groupId > 0)
    {
        selectGroupByDialogUid(uid, groupId);
    }
}

void AppController::showApplyRequests()
{
    ensureApplyInitialized();
    setContactPane(ApplyRequestPane);
    setAuthStatus(QString(), false);
}

void AppController::jumpChatWithCurrentContact()
{
    if (_contact_state.uid <= 0)
    {
        return;
    }

    selectChatByUid(_contact_state.uid);
    switchChatTab(ChatTabPage);
}
