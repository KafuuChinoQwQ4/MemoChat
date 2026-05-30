#include "AppController.h"
#include "ConversationSyncService.h"
#include "usermgr.h"

#include <QSettings>
#include <QVariantMap>
#include <QtGlobal>

namespace
{
constexpr qint64 kDefaultAdminPermBits = (1LL << 0) | (1LL << 1) | (1LL << 2) | (1LL << 4) | (1LL << 5);
constexpr qint64 kOwnerPermBits = kDefaultAdminPermBits | (1LL << 3) | (1LL << 6);
} // namespace

void AppController::clearCurrentGroupConversation(qint64 groupId)
{
    const qint64 targetGroupId = groupId > 0 ? groupId : _group_state.currentId;
    if (targetGroupId > 0)
    {
        const int dialogUid = ConversationSyncService::resolveGroupDialogUid(_group_state.dialogUidMap, targetGroupId);
        _dialog_state.mentionMap.remove(dialogUid);
        _dialog_list_model.clearMention(dialogUid);
        _dialog_list_model.removeByUid(dialogUid);
        _group_list_model.removeByUid(dialogUid);
        _group_state.dialogUidMap.remove(dialogUid);
    }
    if (targetGroupId <= 0 || _group_state.currentId == targetGroupId)
    {
        setCurrentGroup(0, QString());
        _group_state.historyBeforeSeq = 0;
        _group_state.historyHasMore = true;
        _loading_state.groupHistoryLoading = false;
        setPendingReplyContext(QString(), QString(), QString());
        _message_model.clear();
        setCurrentChatPeerName(QString());
        setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.png"));
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        setCanLoadMorePrivateHistory(false);
        emitCurrentDialogUidChangedIfNeeded();
    }
}

int AppController::currentDialogUid() const
{
    if (_group_state.currentId > 0)
    {
        return ConversationSyncService::makeGroupDialogUid(_group_state.currentId);
    }
    if (_chat_state.uid > 0)
    {
        return _chat_state.uid;
    }
    return 0;
}

void AppController::emitCurrentDialogUidChangedIfNeeded()
{
    const int dialogUid = currentDialogUid();
    if (_dialog_state.lastEmittedUid == dialogUid)
    {
        return;
    }
    _dialog_state.lastEmittedUid = dialogUid;
    emit currentDialogUidChanged();
}

bool AppController::resolveDialogTarget(int dialogUid, QString& dialogType, int& peerUid, qint64& groupId) const
{
    dialogType.clear();
    peerUid = 0;
    groupId = 0;
    if (dialogUid == 0)
    {
        return false;
    }
    if (dialogUid > 0)
    {
        dialogType = "private";
        peerUid = dialogUid;
        return true;
    }

    const qint64 candidateGroupId = ConversationSyncService::groupIdForDialogUid(_group_state.dialogUidMap, dialogUid);
    if (candidateGroupId <= 0)
    {
        return false;
    }
    dialogType = "group";
    groupId = candidateGroupId;
    return true;
}

qint64 AppController::currentGroupPermissionBitsRaw() const
{
    if (_group_state.currentId <= 0)
    {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_group_state.currentId);
    if (!groupInfo)
    {
        return 0;
    }
    if (groupInfo->_role >= 3)
    {
        return kOwnerPermBits;
    }
    if (groupInfo->_role < 2)
    {
        return 0;
    }
    if (groupInfo->_permission_bits <= 0)
    {
        return kDefaultAdminPermBits;
    }
    return groupInfo->_permission_bits;
}

bool AppController::hasCurrentGroupPermission(qint64 permissionBit) const
{
    if (permissionBit <= 0)
    {
        return false;
    }
    return (currentGroupPermissionBitsRaw() & permissionBit) != 0;
}

qint64 AppController::latestGroupCreatedAt(qint64 groupId) const
{
    if (groupId <= 0)
    {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo)
    {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto& one : groupInfo->_chat_msgs)
    {
        if (one)
        {
            latestTs = qMax(latestTs, one->_created_at);
        }
    }
    return latestTs;
}

qint64 AppController::latestPrivatePeerCreatedAt(int peerUid) const
{
    if (peerUid <= 0)
    {
        return 0;
    }
    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (!friendInfo)
    {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto& one : friendInfo->_chat_msgs)
    {
        if (one && one->_from_uid == peerUid)
        {
            latestTs = qMax(latestTs, one->_created_at);
        }
    }
    return latestTs;
}

void AppController::syncCurrentDialogDraft()
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0)
    {
        setCurrentDraftText(QString());
        setCurrentPendingAttachments(QVariantList());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        setPendingReplyContext(QString(), QString(), QString());
        return;
    }
    setCurrentDraftText(_dialog_state.draftMap.value(dialogUid));
    syncCurrentPendingAttachments();
    const bool pinned =
        _dialog_state.localPinnedSet.contains(dialogUid) || _dialog_state.serverPinnedMap.value(dialogUid, 0) > 0;
    setCurrentDialogPinned(pinned);
    setCurrentDialogMuted(_dialog_state.serverMuteMap.value(dialogUid, 0) > 0);
}

void AppController::syncCurrentPendingAttachments()
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0)
    {
        setCurrentPendingAttachments(QVariantList());
        return;
    }
    setCurrentPendingAttachments(_dialog_state.pendingAttachmentMap.value(dialogUid));
}

void AppController::loadDraftStore(int ownerUid)
{
    _dialog_state.draftMap.clear();
    _dialog_state.localPinnedSet.clear();
    _dialog_state.serverPinnedMap.clear();
    _dialog_state.serverMuteMap.clear();
    _dialog_state.mentionMap.clear();
    if (ownerUid <= 0)
    {
        return;
    }

    QSettings settings("MemoChat", "MemoChatQml");
    const QVariantMap serialized = settings.value(QString("chat_drafts/%1").arg(ownerUid)).toMap();
    for (auto it = serialized.cbegin(); it != serialized.cend(); ++it)
    {
        bool ok = false;
        const int dialogUid = it.key().toInt(&ok);
        if (!ok)
        {
            continue;
        }
        const QString draftText = it.value().toString();
        if (draftText.trimmed().isEmpty())
        {
            continue;
        }
        _dialog_state.draftMap.insert(dialogUid, draftText);
    }

    const QStringList pinnedList = settings.value(QString("chat_pinned/%1").arg(ownerUid)).toStringList();
    for (const QString& entry : pinnedList)
    {
        bool ok = false;
        const int dialogUid = entry.toInt(&ok);
        if (ok && dialogUid != 0)
        {
            _dialog_state.localPinnedSet.insert(dialogUid);
        }
    }
}

void AppController::saveDraftStore(int ownerUid) const
{
    if (ownerUid <= 0)
    {
        return;
    }

    QVariantMap serialized;
    for (auto it = _dialog_state.draftMap.cbegin(); it != _dialog_state.draftMap.cend(); ++it)
    {
        if (it.value().trimmed().isEmpty())
        {
            continue;
        }
        serialized.insert(QString::number(it.key()), it.value());
    }

    QSettings settings("MemoChat", "MemoChatQml");
    settings.setValue(QString("chat_drafts/%1").arg(ownerUid), serialized);
    QStringList pinnedList;
    pinnedList.reserve(_dialog_state.localPinnedSet.size());
    for (int uid : _dialog_state.localPinnedSet)
    {
        pinnedList.push_back(QString::number(uid));
    }
    settings.setValue(QString("chat_pinned/%1").arg(ownerUid), pinnedList);
}

void AppController::applyDraftToDialogModel(int dialogUid, const QString& draftText)
{
    const int idx = _dialog_list_model.indexOfUid(dialogUid);
    if (idx < 0)
    {
        return;
    }
    const QVariantMap item = _dialog_list_model.get(idx);
    _dialog_list_model.setDialogMeta(dialogUid,
                                     item.value("dialogType").toString(),
                                     item.value("unreadCount").toInt(),
                                     item.value("pinnedRank").toInt(),
                                     draftText,
                                     item.value("lastMsgTs").toLongLong(),
                                     item.value("muteState").toInt());
}
