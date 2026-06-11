#include "ChatFeatureControllerDialogRuntimeInternal.h"

using namespace memochat::chat::dialog_runtime;

ChatDialogCommandResult ChatFeatureController::toggleDialogPinned(const ChatDialogCommandRequest& request,
                                                                  ChatDialogCommandDependencies dependencies)
{
    ChatDialogCommandResult result;
    result.dialogUid = request.dialogUid;
    const ChatDialogPinnedResult pinnedResult = togglePinnedForDialog(request.dialogUid);
    result.success =
        pinnedResult.valid && request.selfUid > 0 && (request.target.isPrivate() || request.target.isGroup());
    if (!result.success)
    {
        return result;
    }

    if (request.dialogUid == request.currentDialogUid && _dialogRuntimePort.syncCurrentDialogPinned)
    {
        _dialogRuntimePort.syncCurrentDialogPinned(pinnedResult.pinned);
        result.currentPinnedUpdated = true;
    }
    result.persistentStoreSaved = invokeSavePersistentDialogStore(request.ownerUid, dependencies);
    result.compactPayload = buildPinDialogPayload(request.selfUid,
                                                  request.target.dialogType,
                                                  request.target.peerUid,
                                                  request.target.groupId,
                                                  pinnedResult.pinnedRank);
    result.dispatched = dispatchDialogPayload(kPinDialogRequestId, result.compactPayload, dependencies);
    if (dependencies.refreshDialogModel)
    {
        dependencies.refreshDialogModel();
        result.dialogModelRefreshed = true;
    }
    return result;
}

ChatDialogCommandResult ChatFeatureController::toggleCurrentDialogPinned()
{
    const ChatDialogCommandSnapshot snapshot =
        _dialogCommandPort.snapshot ? _dialogCommandPort.snapshot() : ChatDialogCommandSnapshot{};
    return toggleDialogPinnedByUid(snapshot.currentDialogUid);
}

ChatDialogCommandResult ChatFeatureController::toggleDialogPinnedByUid(int dialogUid)
{
    const ChatDialogCommandSnapshot snapshot =
        _dialogCommandPort.snapshot ? _dialogCommandPort.snapshot() : ChatDialogCommandSnapshot{};
    const ChatDialogTarget target = targetForDialogUid(_dialogCommandPort, dialogUid);
    const ChatDialogCommandRequest request = commandRequestForDialog(snapshot, dialogUid, target);
    return toggleDialogPinned(request, commandDependenciesFromPort(_dialogCommandPort));
}

ChatDialogCommandResult ChatFeatureController::toggleDialogMuted(const ChatDialogCommandRequest& request,
                                                                 ChatDialogCommandDependencies dependencies)
{
    if (!dependencies.dialogListModel)
    {
        dependencies.dialogListModel = &_dialogListModel;
    }

    ChatDialogCommandResult result;
    result.dialogUid = request.dialogUid;
    const ChatDialogMutedResult mutedResult = toggleMutedForDialog(request.dialogUid);
    result.success =
        mutedResult.valid && request.selfUid > 0 && (request.target.isPrivate() || request.target.isGroup());
    if (!result.success)
    {
        return result;
    }

    if (request.dialogUid == request.currentDialogUid && _dialogRuntimePort.syncCurrentDialogMuted)
    {
        _dialogRuntimePort.syncCurrentDialogMuted(mutedResult.muted);
        result.currentMutedUpdated = true;
    }
    if (dependencies.dialogListModel != nullptr)
    {
        result.dialogModelUpdated =
            applyMuteToDialogModel(*dependencies.dialogListModel, request.dialogUid, mutedResult.muteState);
    }
    result.compactPayload = buildDraftSyncPayload(request.selfUid,
                                                  request.target.dialogType,
                                                  request.target.peerUid,
                                                  request.target.groupId,
                                                  mutedResult.draftText,
                                                  mutedResult.muteState);
    result.dispatched = dispatchDialogPayload(kSyncDraftRequestId, result.compactPayload, dependencies);
    return result;
}

ChatDialogCommandResult ChatFeatureController::toggleCurrentDialogMuted()
{
    const ChatDialogCommandSnapshot snapshot =
        _dialogCommandPort.snapshot ? _dialogCommandPort.snapshot() : ChatDialogCommandSnapshot{};
    return toggleDialogMutedByUid(snapshot.currentDialogUid);
}

ChatDialogCommandResult ChatFeatureController::toggleDialogMutedByUid(int dialogUid)
{
    const ChatDialogCommandSnapshot snapshot =
        _dialogCommandPort.snapshot ? _dialogCommandPort.snapshot() : ChatDialogCommandSnapshot{};
    const ChatDialogTarget target = targetForDialogUid(_dialogCommandPort, dialogUid);
    const ChatDialogCommandRequest request = commandRequestForDialog(snapshot, dialogUid, target);
    return toggleDialogMuted(request, commandDependenciesFromPort(_dialogCommandPort));
}

ChatMarkDialogReadResult ChatFeatureController::markDialogRead(const ChatMarkDialogReadRequest& request,
                                                               ChatMarkDialogReadDependencies dependencies)
{
    if (!dependencies.dialogListModel)
    {
        dependencies.dialogListModel = &_dialogListModel;
    }
    if (!dependencies.privateChatListModel)
    {
        dependencies.privateChatListModel = &_chatListModel;
    }

    ChatMarkDialogReadResult result;
    result.dialogUid = request.dialogUid;
    result.success = request.dialogUid != 0;
    if (!result.success || dependencies.dialogListModel == nullptr)
    {
        return result;
    }

    dependencies.dialogListModel->clearUnread(request.dialogUid);
    dependencies.dialogListModel->clearMention(request.dialogUid);
    _dialogRuntimeState.mentionMap.remove(request.dialogUid);
    result.dialogUnreadCleared = true;
    result.mentionCleared = true;

    if (request.target.isPrivate())
    {
        if (dependencies.privateChatListModel != nullptr)
        {
            dependencies.privateChatListModel->clearUnread(request.target.peerUid);
            result.privateUnreadCleared = true;
        }
        result.readAckTs = qMax<qint64>(0, request.latestPrivatePeerTs);
        ChatReadAckCommand readAckRequest;
        readAckRequest.selfUid = request.selfUid;
        readAckRequest.peerUid = request.target.peerUid;
        readAckRequest.readTs = result.readAckTs;
        const ChatReadAckDispatchPort readAckPort =
            dependencies.readAckDispatchPort.dispatchPayload ? dependencies.readAckDispatchPort : _readAckPort.dispatch;
        const ChatReadAckCommandResult readAckResult = sendPrivateReadAck(readAckRequest, readAckPort);
        result.privateReadAckDispatched = readAckResult.dispatched;
        return result;
    }

    const bool groupLikeDialog = request.target.isGroup() || request.dialogUid < 0;
    if (groupLikeDialog && dependencies.groupListModel != nullptr)
    {
        dependencies.groupListModel->clearUnread(request.dialogUid);
        result.groupUnreadCleared = true;
    }
    if (!request.target.isGroup())
    {
        result.success = false;
        return result;
    }
    result.readAckTs = qMax<qint64>(0, request.latestGroupTs);
    ChatReadAckCommand readAckRequest;
    readAckRequest.selfUid = request.selfUid;
    readAckRequest.groupId = request.target.groupId;
    readAckRequest.readTs = result.readAckTs;
    const ChatReadAckDispatchPort readAckPort =
        dependencies.readAckDispatchPort.dispatchPayload ? dependencies.readAckDispatchPort : _readAckPort.dispatch;
    const ChatReadAckCommandResult readAckResult = sendGroupReadAck(readAckRequest, readAckPort);
    result.groupReadAckDispatched = readAckResult.dispatched;
    return result;
}

ChatMarkDialogReadResult ChatFeatureController::markDialogReadByUid(int dialogUid)
{
    if (dialogUid == 0)
    {
        return {};
    }
    const ChatDialogCommandSnapshot snapshot =
        _dialogCommandPort.snapshot ? _dialogCommandPort.snapshot() : ChatDialogCommandSnapshot{};
    ChatMarkDialogReadRequest request;
    request.selfUid = snapshot.selfUid;
    request.dialogUid = dialogUid;
    request.target = targetForDialogUid(_dialogCommandPort, dialogUid);
    if (request.target.isPrivate())
    {
        request.latestPrivatePeerTs = latestPrivatePeerCreatedAt(_dialogCommandPort, request.target.peerUid);
    }
    else if (request.target.isGroup())
    {
        request.latestGroupTs = latestGroupCreatedAt(_dialogCommandPort, request.target.groupId);
    }
    return markDialogRead(request, markReadDependenciesFromPort(_dialogCommandPort, _readAckPort));
}
