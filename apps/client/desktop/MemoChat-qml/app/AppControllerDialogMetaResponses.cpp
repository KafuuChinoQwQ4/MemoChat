#include "AppController.h"
#include "ConversationSyncService.h"
#include "usermgr.h"

#include <QJsonObject>
#include <QMap>
#include <QtGlobal>

namespace
{

int dialogUidFromPayload(const QJsonObject& payload, QMap<int, qint64>& dialogUidMap)
{
    const QString dialogType = payload.value("dialog_type").toString();
    if (dialogType == "group")
    {
        const qint64 groupId = payload.value("group_id").toVariant().toLongLong();
        if (groupId > 0)
        {
            return ConversationSyncService::resolveGroupDialogUid(dialogUidMap, groupId);
        }
    }
    else if (dialogType == "private")
    {
        return payload.value("peer_uid").toInt();
    }
    return 0;
}

} // namespace

void AppController::handleDialogMetaRsp(ReqId reqId, const QJsonObject& payload)
{
    const int dialogUid = dialogUidFromPayload(payload, _group_state.dialogUidMap);
    if (dialogUid == 0)
    {
        return;
    }

    if (reqId == ID_SYNC_DRAFT_RSP)
    {
        const QString draftText = payload.value("draft_text").toString();
        if (draftText.trimmed().isEmpty())
        {
            _dialog_state.draftMap.remove(dialogUid);
        }
        else
        {
            _dialog_state.draftMap.insert(dialogUid, draftText);
        }
        const int muteState = payload.value("mute_state").toInt(_dialog_state.serverMuteMap.value(dialogUid, 0));
        _dialog_state.serverMuteMap.insert(dialogUid, muteState > 0 ? 1 : 0);
        const int idx = _dialog_list_model.indexOfUid(dialogUid);
        if (idx >= 0)
        {
            const QVariantMap item = _dialog_list_model.get(idx);
            _dialog_list_model.setDialogMeta(dialogUid,
                                             item.value("dialogType").toString(),
                                             item.value("unreadCount").toInt(),
                                             item.value("pinnedRank").toInt(),
                                             draftText,
                                             item.value("lastMsgTs").toLongLong(),
                                             muteState);
        }
        else
        {
            applyDraftToDialogModel(dialogUid, draftText);
        }
        if (dialogUid == currentDialogUid())
        {
            setCurrentDraftText(draftText);
            setCurrentDialogMuted(muteState > 0);
        }
        const int ownerUid = _gateway.userMgr()->GetUid();
        if (ownerUid > 0)
        {
            saveDraftStore(ownerUid);
        }
        return;
    }

    if (reqId == ID_PIN_DIALOG_RSP)
    {
        const int pinnedRank = qMax(0, payload.value("pinned_rank").toInt(0));
        _dialog_state.serverPinnedMap.insert(dialogUid, pinnedRank);
        if (pinnedRank > 0)
        {
            _dialog_state.localPinnedSet.insert(dialogUid);
        }
        else
        {
            _dialog_state.localPinnedSet.remove(dialogUid);
        }
        if (dialogUid == currentDialogUid())
        {
            setCurrentDialogPinned(pinnedRank > 0);
        }
        const int ownerUid = _gateway.userMgr()->GetUid();
        if (ownerUid > 0)
        {
            saveDraftStore(ownerUid);
        }
        refreshDialogModel();
    }
}
