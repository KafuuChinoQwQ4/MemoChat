#include "AppController.h"
#include "ConversationSyncService.h"
#include "usermgr.h"

#include <QSettings>
#include <QVariantMap>
#include <QtGlobal>

namespace {
constexpr qint64 kDefaultAdminPermBits =
    (1LL << 0) | (1LL << 1) | (1LL << 2) | (1LL << 4) | (1LL << 5);
constexpr qint64 kOwnerPermBits = kDefaultAdminPermBits | (1LL << 3) | (1LL << 6);
}

void AppController::clearCurrentGroupConversation(qint64 groupId)
{
    const qint64 targetGroupId = groupId > 0 ? groupId : _current_group_id;
    if (targetGroupId > 0) {
        const int dialogUid = ConversationSyncService::resolveGroupDialogUid(_group_uid_map, targetGroupId);
        _dialog_mention_map.remove(dialogUid);
        _dialog_list_model.clearMention(dialogUid);
        _dialog_list_model.removeByUid(dialogUid);
        _group_list_model.removeByUid(dialogUid);
        _group_uid_map.remove(dialogUid);
    }
    if (targetGroupId <= 0 || _current_group_id == targetGroupId) {
        setCurrentGroup(0, QString());
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        _group_history_loading = false;
        setPendingReplyContext(QString(), QString(), QString());
        _message_model.clear();
        setCurrentChatPeerName(QString());
        setCurrentChatPeerIcon(QStringLiteral("qrc:/res/head_1.jpg"));
        setCurrentDraftText(QString());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        setCanLoadMorePrivateHistory(false);
        emitCurrentDialogUidChangedIfNeeded();
    }
}

int AppController::currentDialogUid() const
{
    if (_current_group_id > 0) {
        return ConversationSyncService::makeGroupDialogUid(_current_group_id);
    }
    if (_current_chat_uid > 0) {
        return _current_chat_uid;
    }
    return 0;
}

void AppController::emitCurrentDialogUidChangedIfNeeded()
{
    const int dialogUid = currentDialogUid();
    if (_last_emitted_dialog_uid == dialogUid) {
        return;
    }
    _last_emitted_dialog_uid = dialogUid;
    emit currentDialogUidChanged();
}

bool AppController::resolveDialogTarget(int dialogUid, QString &dialogType, int &peerUid, qint64 &groupId) const
{
    dialogType.clear();
    peerUid = 0;
    groupId = 0;
    if (dialogUid == 0) {
        return false;
    }
    if (dialogUid > 0) {
        dialogType = "private";
        peerUid = dialogUid;
        return true;
    }

    const qint64 candidateGroupId = ConversationSyncService::groupIdForDialogUid(_group_uid_map, dialogUid);
    if (candidateGroupId <= 0) {
        return false;
    }
    dialogType = "group";
    groupId = candidateGroupId;
    return true;
}

qint64 AppController::currentGroupPermissionBitsRaw() const
{
    if (_current_group_id <= 0) {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
    if (!groupInfo) {
        return 0;
    }
    if (groupInfo->_role >= 3) {
        return kOwnerPermBits;
    }
    if (groupInfo->_role < 2) {
        return 0;
    }
    if (groupInfo->_permission_bits <= 0) {
        return kDefaultAdminPermBits;
    }
    return groupInfo->_permission_bits;
}

bool AppController::hasCurrentGroupPermission(qint64 permissionBit) const
{
    if (permissionBit <= 0) {
        return false;
    }
    return (currentGroupPermissionBitsRaw() & permissionBit) != 0;
}

qint64 AppController::latestGroupCreatedAt(qint64 groupId) const
{
    if (groupId <= 0) {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo) {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto &one : groupInfo->_chat_msgs) {
        if (one) {
            latestTs = qMax(latestTs, one->_created_at);
        }
    }
    return latestTs;
}

qint64 AppController::latestPrivatePeerCreatedAt(int peerUid) const
{
    if (peerUid <= 0) {
        return 0;
    }
    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (!friendInfo) {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto &one : friendInfo->_chat_msgs) {
        if (one && one->_from_uid == peerUid) {
            latestTs = qMax(latestTs, one->_created_at);
        }
    }
    return latestTs;
}

void AppController::syncCurrentDialogDraft()
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0) {
        setCurrentDraftText(QString());
        setCurrentPendingAttachments(QVariantList());
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        setPendingReplyContext(QString(), QString(), QString());
        return;
    }
    setCurrentDraftText(_dialog_draft_map.value(dialogUid));
    syncCurrentPendingAttachments();
    const bool pinned = _dialog_local_pinned_set.contains(dialogUid)
        || _dialog_server_pinned_map.value(dialogUid, 0) > 0;
    setCurrentDialogPinned(pinned);
    setCurrentDialogMuted(_dialog_server_mute_map.value(dialogUid, 0) > 0);
}

void AppController::syncCurrentPendingAttachments()
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0) {
        setCurrentPendingAttachments(QVariantList());
        return;
    }
    setCurrentPendingAttachments(_dialog_pending_attachment_map.value(dialogUid));
}

void AppController::loadDraftStore(int ownerUid)
{
    _dialog_draft_map.clear();
    _dialog_local_pinned_set.clear();
    _dialog_server_pinned_map.clear();
    _dialog_server_mute_map.clear();
    _dialog_mention_map.clear();
    if (ownerUid <= 0) {
        return;
    }

    QSettings settings("MemoChat", "MemoChatQml");
    const QVariantMap serialized = settings.value(QString("chat_drafts/%1").arg(ownerUid)).toMap();
    for (auto it = serialized.cbegin(); it != serialized.cend(); ++it) {
        bool ok = false;
        const int dialogUid = it.key().toInt(&ok);
        if (!ok) {
            continue;
        }
        const QString draftText = it.value().toString();
        if (draftText.trimmed().isEmpty()) {
            continue;
        }
        _dialog_draft_map.insert(dialogUid, draftText);
    }

    const QStringList pinnedList = settings.value(QString("chat_pinned/%1").arg(ownerUid)).toStringList();
    for (const QString &entry : pinnedList) {
        bool ok = false;
        const int dialogUid = entry.toInt(&ok);
        if (ok && dialogUid != 0) {
            _dialog_local_pinned_set.insert(dialogUid);
        }
    }
}

void AppController::saveDraftStore(int ownerUid) const
{
    if (ownerUid <= 0) {
        return;
    }

    QVariantMap serialized;
    for (auto it = _dialog_draft_map.cbegin(); it != _dialog_draft_map.cend(); ++it) {
        if (it.value().trimmed().isEmpty()) {
            continue;
        }
        serialized.insert(QString::number(it.key()), it.value());
    }

    QSettings settings("MemoChat", "MemoChatQml");
    settings.setValue(QString("chat_drafts/%1").arg(ownerUid), serialized);
    QStringList pinnedList;
    pinnedList.reserve(_dialog_local_pinned_set.size());
    for (int uid : _dialog_local_pinned_set) {
        pinnedList.push_back(QString::number(uid));
    }
    settings.setValue(QString("chat_pinned/%1").arg(ownerUid), pinnedList);
}

void AppController::applyDraftToDialogModel(int dialogUid, const QString &draftText)
{
    const int idx = _dialog_list_model.indexOfUid(dialogUid);
    if (idx < 0) {
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
