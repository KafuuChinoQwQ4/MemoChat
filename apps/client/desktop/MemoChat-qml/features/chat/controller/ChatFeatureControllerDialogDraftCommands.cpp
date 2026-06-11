#include "ChatFeatureControllerDialogRuntimeInternal.h"

using namespace memochat::chat::dialog_runtime;

ChatDialogDraftResult ChatFeatureController::updateDraftForDialog(int dialogUid, const QString& text)
{
    ChatDialogDraftResult result;
    result.dialogUid = dialogUid;
    result.valid = dialogUid != 0;
    if (!result.valid)
    {
        return result;
    }

    result.draftText = normalizeDraftText(text);
    if (result.draftText.isEmpty())
    {
        _dialogRuntimeState.draftMap.remove(dialogUid);
    }
    else
    {
        _dialogRuntimeState.draftMap.insert(dialogUid, result.draftText);
    }
    return result;
}

ChatDialogDraftResult ChatFeatureController::clearDraftForDialog(int dialogUid)
{
    ChatDialogDraftResult result;
    result.dialogUid = dialogUid;
    result.valid = dialogUid != 0;
    if (result.valid)
    {
        _dialogRuntimeState.draftMap.remove(dialogUid);
    }
    return result;
}

ChatDialogPinnedResult ChatFeatureController::togglePinnedForDialog(int dialogUid)
{
    ChatDialogPinnedResult result;
    result.dialogUid = dialogUid;
    result.valid = dialogUid != 0;
    if (!result.valid)
    {
        return result;
    }

    const bool currentlyPinned = _dialogRuntimeState.localPinnedSet.contains(dialogUid) ||
                                 _dialogRuntimeState.serverPinnedMap.value(dialogUid, 0) > 0;
    result.pinned = !currentlyPinned;
    if (!result.pinned)
    {
        _dialogRuntimeState.localPinnedSet.remove(dialogUid);
        _dialogRuntimeState.serverPinnedMap.insert(dialogUid, 0);
        result.pinnedRank = 0;
    }
    else
    {
        result.pinnedRank = static_cast<int>(QDateTime::currentSecsSinceEpoch());
        _dialogRuntimeState.localPinnedSet.insert(dialogUid);
        _dialogRuntimeState.serverPinnedMap.insert(dialogUid, result.pinnedRank);
    }
    return result;
}

ChatDialogMutedResult ChatFeatureController::toggleMutedForDialog(int dialogUid)
{
    ChatDialogMutedResult result;
    result.dialogUid = dialogUid;
    result.valid = dialogUid != 0;
    if (!result.valid)
    {
        return result;
    }

    result.muted = _dialogRuntimeState.serverMuteMap.value(dialogUid, 0) <= 0;
    result.muteState = result.muted ? 1 : 0;
    result.draftText = _dialogRuntimeState.draftMap.value(dialogUid);
    _dialogRuntimeState.serverMuteMap.insert(dialogUid, result.muteState);
    return result;
}

ChatDialogCommandResult ChatFeatureController::updateDialogDraft(const ChatDialogCommandRequest& request,
                                                                 ChatDialogCommandDependencies dependencies)
{
    if (!dependencies.dialogListModel)
    {
        dependencies.dialogListModel = &_dialogListModel;
    }

    ChatDialogCommandResult result;
    result.dialogUid = request.dialogUid;
    const ChatDialogDraftResult draftResult = updateDraftForDialog(request.dialogUid, request.draftText);
    result.success =
        draftResult.valid && request.selfUid > 0 && (request.target.isPrivate() || request.target.isGroup());
    if (!draftResult.valid || request.selfUid <= 0 || (!request.target.isPrivate() && !request.target.isGroup()))
    {
        result.success = false;
        return result;
    }

    if (_dialogRuntimePort.syncCurrentDraftText)
    {
        _dialogRuntimePort.syncCurrentDraftText(draftResult.draftText);
        result.currentDraftUpdated = true;
    }
    if (dependencies.dialogListModel != nullptr)
    {
        result.dialogModelUpdated =
            applyDraftToDialogModel(*dependencies.dialogListModel, request.dialogUid, draftResult.draftText);
    }
    result.persistentStoreSaved = invokeSavePersistentDialogStore(request.ownerUid, dependencies);
    result.compactPayload = buildDraftSyncPayload(request.selfUid,
                                                  request.target.dialogType,
                                                  request.target.peerUid,
                                                  request.target.groupId,
                                                  draftResult.draftText);
    result.dispatched = dispatchDialogPayload(kSyncDraftRequestId, result.compactPayload, dependencies);
    return result;
}

ChatDialogCommandResult ChatFeatureController::updateCurrentDraft(const QString& text)
{
    const ChatDialogCommandSnapshot snapshot =
        _dialogCommandPort.snapshot ? _dialogCommandPort.snapshot() : ChatDialogCommandSnapshot{};
    if (snapshot.currentDialogUid == 0)
    {
        return {};
    }
    const ChatDialogTarget target = currentDialogTarget(snapshot);
    const ChatDialogCommandRequest request = commandRequestForDialog(snapshot, snapshot.currentDialogUid, target, text);
    return updateDialogDraft(request, commandDependenciesFromPort(_dialogCommandPort));
}

ChatDialogCommandResult ChatFeatureController::clearDialogDraft(const ChatDialogCommandRequest& request,
                                                                ChatDialogCommandDependencies dependencies)
{
    if (!dependencies.dialogListModel)
    {
        dependencies.dialogListModel = &_dialogListModel;
    }

    ChatDialogCommandResult result;
    result.dialogUid = request.dialogUid;
    const ChatDialogDraftResult draftResult = clearDraftForDialog(request.dialogUid);
    result.success =
        draftResult.valid && request.selfUid > 0 && (request.target.isPrivate() || request.target.isGroup());
    if (!result.success)
    {
        return result;
    }

    if (dependencies.dialogListModel != nullptr)
    {
        result.dialogModelUpdated =
            applyDraftToDialogModel(*dependencies.dialogListModel, request.dialogUid, QString());
    }
    if (request.dialogUid == request.currentDialogUid && _dialogRuntimePort.syncCurrentDraftText)
    {
        _dialogRuntimePort.syncCurrentDraftText(QString());
        result.currentDraftUpdated = true;
    }
    result.persistentStoreSaved = invokeSavePersistentDialogStore(request.ownerUid, dependencies);
    result.compactPayload = buildDraftSyncPayload(request.selfUid,
                                                  request.target.dialogType,
                                                  request.target.peerUid,
                                                  request.target.groupId,
                                                  QString());
    result.dispatched = dispatchDialogPayload(kSyncDraftRequestId, result.compactPayload, dependencies);
    return result;
}

ChatDialogCommandResult ChatFeatureController::clearDialogDraftByUid(int dialogUid)
{
    const ChatDialogCommandSnapshot snapshot =
        _dialogCommandPort.snapshot ? _dialogCommandPort.snapshot() : ChatDialogCommandSnapshot{};
    const ChatDialogTarget target = targetForDialogUid(_dialogCommandPort, dialogUid);
    const ChatDialogCommandRequest request = commandRequestForDialog(snapshot, dialogUid, target);
    return clearDialogDraft(request, commandDependenciesFromPort(_dialogCommandPort));
}
