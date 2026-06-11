#include "ChatFeatureControllerDialogRuntimeInternal.h"

using namespace memochat::chat::dialog_runtime;

DialogDecorationState ChatFeatureController::dialogDecorationState() const
{
    return DialogDecorationState{&_dialogRuntimeState.draftMap,
                                 &_dialogRuntimeState.mentionMap,
                                 &_dialogRuntimeState.serverPinnedMap,
                                 &_dialogRuntimeState.serverMuteMap,
                                 &_dialogRuntimeState.localPinnedSet};
}

DialogDecorationState ChatFeatureController::dialogDecorationStateWithoutServerMeta() const
{
    return DialogDecorationState{&_dialogRuntimeState.draftMap,
                                 &_dialogRuntimeState.mentionMap,
                                 nullptr,
                                 nullptr,
                                 &_dialogRuntimeState.localPinnedSet};
}

void ChatFeatureController::resetServerDialogMeta()
{
    _dialogRuntimeState.serverPinnedMap.clear();
    _dialogRuntimeState.serverMuteMap.clear();
}

void ChatFeatureController::seedServerDialogMeta(int dialogUid, int pinnedRank, int muteState)
{
    if (dialogUid == 0)
    {
        return;
    }
    _dialogRuntimeState.serverPinnedMap.insert(dialogUid, qMax(0, pinnedRank));
    _dialogRuntimeState.serverMuteMap.insert(dialogUid, muteState > 0 ? 1 : 0);
}

void ChatFeatureController::removeMentionForDialog(int dialogUid)
{
    if (dialogUid != 0)
    {
        _dialogRuntimeState.mentionMap.remove(dialogUid);
    }
}

void ChatFeatureController::setMentionCountForDialog(int dialogUid, int count)
{
    if (dialogUid == 0)
    {
        return;
    }
    if (count <= 0)
    {
        _dialogRuntimeState.mentionMap.remove(dialogUid);
        return;
    }
    _dialogRuntimeState.mentionMap.insert(dialogUid, count);
}

void ChatFeatureController::clearGroupUnreadAndMention(FriendListModel& dialogListModel, int dialogUid)
{
    ConversationSyncService::clearGroupUnreadAndMention(dialogListModel, _dialogRuntimeState.mentionMap, dialogUid);
}

void ChatFeatureController::incrementGroupUnreadAndMention(FriendListModel& dialogListModel,
                                                           int dialogUid,
                                                           bool mentioned)
{
    ConversationSyncService::incrementGroupUnreadAndMention(dialogListModel,
                                                            _dialogRuntimeState.mentionMap,
                                                            dialogUid,
                                                            mentioned);
}

void ChatFeatureController::clearMentionState()
{
    _dialogRuntimeState.mentionMap.clear();
}

void ChatFeatureController::clearPendingAttachmentStore()
{
    _dialogRuntimeState.pendingAttachmentMap.clear();
}

void ChatFeatureController::clearDialogDecorationStore()
{
    _dialogRuntimeState.draftMap.clear();
    _dialogRuntimeState.pendingAttachmentMap.clear();
    _dialogRuntimeState.localPinnedSet.clear();
    _dialogRuntimeState.serverPinnedMap.clear();
    _dialogRuntimeState.serverMuteMap.clear();
    _dialogRuntimeState.mentionMap.clear();
}

void ChatFeatureController::removeDialogDecoration(int dialogUid)
{
    if (dialogUid == 0)
    {
        return;
    }
    _dialogRuntimeState.draftMap.remove(dialogUid);
    _dialogRuntimeState.mentionMap.remove(dialogUid);
    _dialogRuntimeState.localPinnedSet.remove(dialogUid);
    _dialogRuntimeState.serverPinnedMap.remove(dialogUid);
    _dialogRuntimeState.serverMuteMap.remove(dialogUid);
}

void ChatFeatureController::loadPersistentDialogStore(int ownerUid)
{
    _dialogRuntimeState.draftMap.clear();
    _dialogRuntimeState.localPinnedSet.clear();
    _dialogRuntimeState.serverPinnedMap.clear();
    _dialogRuntimeState.serverMuteMap.clear();
    _dialogRuntimeState.mentionMap.clear();
    if (ownerUid <= 0)
    {
        return;
    }

    QSettings settings(QStringLiteral("MemoChat"), QStringLiteral("MemoChatQml"));
    const QVariantMap serialized = settings.value(QStringLiteral("chat_drafts/%1").arg(ownerUid)).toMap();
    for (auto it = serialized.cbegin(); it != serialized.cend(); ++it)
    {
        bool ok = false;
        const int dialogUid = it.key().toInt(&ok);
        if (!ok)
        {
            continue;
        }
        const QString draftText = normalizeStoredDraftText(it.value().toString());
        if (!draftText.isEmpty())
        {
            _dialogRuntimeState.draftMap.insert(dialogUid, draftText);
        }
    }

    const QStringList pinnedList = settings.value(QStringLiteral("chat_pinned/%1").arg(ownerUid)).toStringList();
    for (const QString& entry : pinnedList)
    {
        bool ok = false;
        const int dialogUid = entry.toInt(&ok);
        if (ok && dialogUid != 0)
        {
            _dialogRuntimeState.localPinnedSet.insert(dialogUid);
        }
    }
}

void ChatFeatureController::savePersistentDialogStore(int ownerUid) const
{
    if (ownerUid <= 0)
    {
        return;
    }

    QVariantMap serialized;
    for (auto it = _dialogRuntimeState.draftMap.cbegin(); it != _dialogRuntimeState.draftMap.cend(); ++it)
    {
        if (it.value().trimmed().isEmpty())
        {
            continue;
        }
        serialized.insert(QString::number(it.key()), it.value());
    }

    QSettings settings(QStringLiteral("MemoChat"), QStringLiteral("MemoChatQml"));
    settings.setValue(QStringLiteral("chat_drafts/%1").arg(ownerUid), serialized);

    QStringList pinnedList;
    pinnedList.reserve(_dialogRuntimeState.localPinnedSet.size());
    for (int uid : _dialogRuntimeState.localPinnedSet)
    {
        pinnedList.push_back(QString::number(uid));
    }
    settings.setValue(QStringLiteral("chat_pinned/%1").arg(ownerUid), pinnedList);
}

QString ChatFeatureController::draftForDialog(int dialogUid) const
{
    return _dialogRuntimeState.draftMap.value(dialogUid);
}

int ChatFeatureController::muteStateForDialog(int dialogUid) const
{
    return _dialogRuntimeState.serverMuteMap.value(dialogUid, 0);
}

QVariantList ChatFeatureController::pendingAttachmentsForDialog(int dialogUid) const
{
    return _dialogRuntimeState.pendingAttachmentMap.value(dialogUid);
}

void ChatFeatureController::setPendingAttachmentsForDialog(int dialogUid, const QVariantList& attachments)
{
    if (dialogUid == 0)
    {
        return;
    }
    if (attachments.isEmpty())
    {
        _dialogRuntimeState.pendingAttachmentMap.remove(dialogUid);
        return;
    }
    _dialogRuntimeState.pendingAttachmentMap.insert(dialogUid, attachments);
}

void ChatFeatureController::clearPendingAttachmentsForDialog(int dialogUid)
{
    if (dialogUid != 0)
    {
        _dialogRuntimeState.pendingAttachmentMap.remove(dialogUid);
    }
}
