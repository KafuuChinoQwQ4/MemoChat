#include "ChatFeatureControllerDialogRuntimeInternal.h"

using namespace memochat::chat::dialog_runtime;

ChatDialogMetaResponseResult
ChatFeatureController::handleDialogMetaResponse(const ChatDialogMetaResponseRequest& request,
                                                ChatDialogMetaResponseDependencies dependencies)
{
    if (!dependencies.dialogListModel)
    {
        dependencies.dialogListModel = &_dialogListModel;
    }

    ChatDialogMetaResponseResult result;
    result.dialogUid = dialogUidFromMetaPayload(request.payload, request.groupDialogUidMap);
    result.currentDialog = result.dialogUid != 0 && result.dialogUid == request.currentDialogUid;
    if (result.dialogUid == 0)
    {
        return result;
    }

    if (request.reqId == kSyncDraftResponseId)
    {
        result.draftPath = true;
        result.draftText = request.payload.value(QStringLiteral("draft_text")).toString();
        result.muteState =
            request.payload.value(QStringLiteral("mute_state")).toInt(muteStateForDialog(result.dialogUid));
        const ChatDialogDraftResult draftResult =
            applyRemoteDraftMeta(result.dialogUid, result.draftText, result.muteState);
        result.runtimeUpdated = draftResult.valid;
        result.success = draftResult.valid;
        if (!result.success)
        {
            return result;
        }

        if (dependencies.dialogListModel)
        {
            result.dialogModelUpdated =
                applyDraftToDialogModel(*dependencies.dialogListModel, result.dialogUid, result.draftText);
            result.dialogModelUpdated =
                applyMuteToDialogModel(*dependencies.dialogListModel, result.dialogUid, result.muteState) ||
                result.dialogModelUpdated;
        }
        if (result.currentDialog)
        {
            if (dependencies.syncCurrentDraftText)
            {
                dependencies.syncCurrentDraftText(result.draftText);
                result.currentDraftUpdated = true;
            }
            if (dependencies.syncCurrentDialogMuted)
            {
                dependencies.syncCurrentDialogMuted(result.muteState > 0);
                result.currentMutedUpdated = true;
            }
        }
        if (request.ownerUid > 0 && dependencies.savePersistentDialogStore)
        {
            dependencies.savePersistentDialogStore(request.ownerUid);
            result.persistentStoreSaved = true;
        }
        return result;
    }

    if (request.reqId == kPinDialogResponseId)
    {
        result.pinPath = true;
        result.pinnedRank = qMax(0, request.payload.value(QStringLiteral("pinned_rank")).toInt(0));
        const ChatDialogPinnedResult pinnedResult = applyRemotePinnedMeta(result.dialogUid, result.pinnedRank);
        result.runtimeUpdated = pinnedResult.valid;
        result.success = pinnedResult.valid;
        if (!result.success)
        {
            return result;
        }
        if (result.currentDialog && dependencies.syncCurrentDialogPinned)
        {
            dependencies.syncCurrentDialogPinned(pinnedResult.pinned);
            result.currentPinnedUpdated = true;
        }
        if (request.ownerUid > 0 && dependencies.savePersistentDialogStore)
        {
            dependencies.savePersistentDialogStore(request.ownerUid);
            result.persistentStoreSaved = true;
        }
        if (dependencies.refreshDialogModel)
        {
            dependencies.refreshDialogModel();
            result.dialogModelRefreshed = true;
        }
        return result;
    }

    return result;
}

ChatDialogDraftResult
ChatFeatureController::applyRemoteDraftMeta(int dialogUid, const QString& draftText, int muteState)
{
    ChatDialogDraftResult result;
    result.dialogUid = dialogUid;
    result.valid = dialogUid != 0;
    result.draftText = draftText;
    if (!result.valid)
    {
        return result;
    }

    const QString storedDraft = normalizeStoredDraftText(draftText);
    if (storedDraft.isEmpty())
    {
        _dialogRuntimeState.draftMap.remove(dialogUid);
    }
    else
    {
        _dialogRuntimeState.draftMap.insert(dialogUid, storedDraft);
    }
    _dialogRuntimeState.serverMuteMap.insert(dialogUid, muteState > 0 ? 1 : 0);
    return result;
}

ChatDialogPinnedResult ChatFeatureController::applyRemotePinnedMeta(int dialogUid, int pinnedRank)
{
    ChatDialogPinnedResult result;
    result.dialogUid = dialogUid;
    result.valid = dialogUid != 0;
    if (!result.valid)
    {
        return result;
    }

    result.pinnedRank = qMax(0, pinnedRank);
    result.pinned = result.pinnedRank > 0;
    _dialogRuntimeState.serverPinnedMap.insert(dialogUid, result.pinnedRank);
    if (result.pinned)
    {
        _dialogRuntimeState.localPinnedSet.insert(dialogUid);
    }
    else
    {
        _dialogRuntimeState.localPinnedSet.remove(dialogUid);
    }
    return result;
}

QByteArray ChatFeatureController::buildDraftSyncPayload(int selfUid,
                                                        const QString& dialogType,
                                                        int peerUid,
                                                        qint64 groupId,
                                                        const QString& draftText,
                                                        int muteState)
{
    if (selfUid <= 0)
    {
        return {};
    }

    QJsonObject obj;
    obj[QStringLiteral("fromuid")] = selfUid;
    obj[QStringLiteral("dialog_type")] = dialogType;
    if (dialogType == QStringLiteral("group"))
    {
        if (groupId <= 0)
        {
            return {};
        }
        obj[QStringLiteral("groupid")] = static_cast<qint64>(groupId);
    }
    else if (dialogType == QStringLiteral("private"))
    {
        if (peerUid <= 0)
        {
            return {};
        }
        obj[QStringLiteral("peer_uid")] = peerUid;
    }
    else
    {
        return {};
    }

    obj[QStringLiteral("draft_text")] = draftText;
    if (muteState >= 0)
    {
        obj[QStringLiteral("mute_state")] = muteState;
    }
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QByteArray ChatFeatureController::buildPinDialogPayload(int selfUid,
                                                        const QString& dialogType,
                                                        int peerUid,
                                                        qint64 groupId,
                                                        int pinnedRank)
{
    if (selfUid <= 0)
    {
        return {};
    }

    QJsonObject obj;
    obj[QStringLiteral("fromuid")] = selfUid;
    obj[QStringLiteral("dialog_type")] = dialogType;
    if (dialogType == QStringLiteral("group"))
    {
        if (groupId <= 0)
        {
            return {};
        }
        obj[QStringLiteral("groupid")] = static_cast<qint64>(groupId);
    }
    else if (dialogType == QStringLiteral("private"))
    {
        if (peerUid <= 0)
        {
            return {};
        }
        obj[QStringLiteral("peer_uid")] = peerUid;
    }
    else
    {
        return {};
    }

    obj[QStringLiteral("pinned_rank")] = pinnedRank;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
