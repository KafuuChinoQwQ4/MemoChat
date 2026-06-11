#include "AppController.h"
#include "ConversationSyncService.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QtGlobal>

namespace
{
constexpr qint64 kDefaultAdminPermBits = (1LL << 0) | (1LL << 1) | (1LL << 2) | (1LL << 4) | (1LL << 5);
constexpr qint64 kOwnerPermBits = kDefaultAdminPermBits | (1LL << 3) | (1LL << 6);
} // namespace

void AppController::clearCurrentGroupConversation(qint64 groupId)
{
    ChatGroupConversationClearRequest request;
    request.groupId = groupId;
    request.currentGroupId = currentGroupId();

    ChatGroupConversationClearPort port;
    port.dialogUidForGroup = [this](qint64 targetGroupId)
    {
        return _features.chatFeatureController.resolveGroupDialogUid(targetGroupId);
    };
    port.removeGroupByDialogUid = [this](int dialogUid)
    {
        _features.groupController.removeGroupById(dialogUid);
    };
    port.removeGroupDialogMapping = [this](int dialogUid)
    {
        _features.chatFeatureController.removeGroupDialogUid(dialogUid);
    };
    port.clearCurrentGroup = [this]()
    {
        setCurrentGroup(0, QString());
    };
    port.setGroupHistoryLoading = [this](bool loading)
    {
        _shell_state.loadingState().groupHistoryLoading = loading;
    };
    port.setPendingReplyContext = [this](const QString& msgId, const QString& senderName, const QString& previewText)
    {
        setPendingReplyContext(msgId, senderName, previewText);
    };
    port.clearMessageModel = [this]()
    {
        _features.chatFeatureController.clearMessageModel();
    };
    port.resetCurrentPeer = [this]()
    {
        setCurrentChatPeerName(QString());
        setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.png"));
    };
    port.setCurrentDraftText = [this](const QString& text)
    {
        setCurrentDraftText(text);
    };
    port.setCurrentDialogPinned = [this](bool pinned)
    {
        setCurrentDialogPinned(pinned);
    };
    port.setCurrentDialogMuted = [this](bool muted)
    {
        setCurrentDialogMuted(muted);
    };
    port.setCanLoadMorePrivateHistory = [this](bool canLoad)
    {
        setCanLoadMorePrivateHistory(canLoad);
    };
    port.emitCurrentDialogUidChangedIfNeeded = [this]()
    {
        emitCurrentDialogUidChangedIfNeeded();
    };

    _features.chatFeatureController.clearGroupConversation(request, port);
}

int AppController::currentDialogUid() const
{
    if (currentGroupId() > 0)
    {
        return ConversationSyncService::makeGroupDialogUid(currentGroupId());
    }
    if (_chat_state.uid > 0)
    {
        return _chat_state.uid;
    }
    return 0;
}

qint64 AppController::currentGroupId() const
{
    return _features.groupController.currentGroupId();
}

void AppController::emitCurrentDialogUidChangedIfNeeded()
{
    const int dialogUid = currentDialogUid();
    if (!_features.chatFeatureController.updateLastEmittedDialogUid(dialogUid))
    {
        return;
    }
    emit currentDialogUidChanged();
}

qint64 AppController::groupIdForDialogUid(int dialogUid) const
{
    return _features.chatFeatureController.groupIdForDialogUid(dialogUid);
}

qint64 AppController::currentGroupPermissionBitsRaw() const
{
    return _features.groupController.currentGroupPermissionBitsRaw();
}

bool AppController::hasCurrentGroupPermission(qint64 permissionBit) const
{
    if (permissionBit <= 0)
    {
        return false;
    }
    return (currentGroupPermissionBitsRaw() & permissionBit) != 0;
}

void AppController::syncCurrentDialogDraft()
{
    const int dialogUid = currentDialogUid();
    const ChatDialogRuntimeSnapshot snapshot = _features.chatFeatureController.snapshotForDialog(dialogUid);
    if (!snapshot.hasDialog)
    {
        setCurrentDraftText(QString());
        setCurrentPendingAttachments(QVariantList());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        setPendingReplyContext(QString(), QString(), QString());
        return;
    }
    setCurrentDraftText(snapshot.draftText);
    setCurrentPendingAttachments(snapshot.pendingAttachments);
    setCurrentDialogPinned(snapshot.pinned);
    setCurrentDialogMuted(snapshot.muted);
}

void AppController::syncCurrentPendingAttachments()
{
    const int dialogUid = currentDialogUid();
    const ChatDialogRuntimeSnapshot snapshot = _features.chatFeatureController.snapshotForDialog(dialogUid);
    if (!snapshot.hasDialog)
    {
        setCurrentPendingAttachments(QVariantList());
        return;
    }
    setCurrentPendingAttachments(snapshot.pendingAttachments);
}

void AppController::loadDraftStore(int ownerUid)
{
    _features.chatFeatureController.loadPersistentDialogStore(ownerUid);
}

void AppController::saveDraftStore(int ownerUid) const
{
    _features.chatFeatureController.savePersistentDialogStore(ownerUid);
}
