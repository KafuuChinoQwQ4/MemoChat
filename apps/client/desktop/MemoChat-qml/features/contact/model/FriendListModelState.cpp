#include "FriendListModel.h"

#include <QtGlobal>

void FriendListModel::updateLastMessage(int uid, const QString& lastMsg, qint64 lastMsgTs)
{
    const int idx = indexOfUid(uid);
    if (idx < 0)
    {
        return;
    }

    FriendEntry& entry = _items[static_cast<size_t>(idx)];
    bool changed = false;
    if (entry.lastMsg != lastMsg)
    {
        entry.lastMsg = lastMsg;
        changed = true;
    }
    if (lastMsgTs > 0 && entry.lastMsgTs != lastMsgTs)
    {
        entry.lastMsgTs = lastMsgTs;
        changed = true;
    }
    if (!changed)
    {
        return;
    }

    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {LastMsgRole, LastMsgTsRole});
}

void FriendListModel::incrementUnread(int uid, int delta)
{
    if (delta <= 0)
    {
        return;
    }
    const int idx = indexOfUid(uid);
    if (idx < 0)
    {
        return;
    }

    FriendEntry& entry = _items[static_cast<size_t>(idx)];
    const int next = entry.unreadCount + delta;
    if (entry.unreadCount == next)
    {
        return;
    }
    entry.unreadCount = next;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {UnreadCountRole});
}

void FriendListModel::clearUnread(int uid)
{
    const int idx = indexOfUid(uid);
    if (idx < 0)
    {
        return;
    }
    FriendEntry& entry = _items[static_cast<size_t>(idx)];
    if (entry.unreadCount == 0)
    {
        return;
    }
    entry.unreadCount = 0;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {UnreadCountRole});
}

void FriendListModel::incrementMention(int uid, int delta)
{
    if (delta <= 0)
    {
        return;
    }
    const int idx = indexOfUid(uid);
    if (idx < 0)
    {
        return;
    }

    FriendEntry& entry = _items[static_cast<size_t>(idx)];
    const int next = entry.mentionCount + delta;
    if (entry.mentionCount == next)
    {
        return;
    }
    entry.mentionCount = next;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {MentionCountRole});
}

void FriendListModel::clearMention(int uid)
{
    const int idx = indexOfUid(uid);
    if (idx < 0)
    {
        return;
    }
    FriendEntry& entry = _items[static_cast<size_t>(idx)];
    if (entry.mentionCount == 0)
    {
        return;
    }
    entry.mentionCount = 0;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {MentionCountRole});
}

void FriendListModel::setMentionCount(int uid, int count)
{
    const int idx = indexOfUid(uid);
    if (idx < 0)
    {
        return;
    }
    const int normalized = qMax(0, count);
    FriendEntry& entry = _items[static_cast<size_t>(idx)];
    if (entry.mentionCount == normalized)
    {
        return;
    }
    entry.mentionCount = normalized;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {MentionCountRole});
}

void FriendListModel::removeByUid(int uid)
{
    const int idx = indexOfUid(uid);
    if (idx < 0)
    {
        return;
    }

    beginRemoveRows(QModelIndex(), idx, idx);
    _items.erase(_items.begin() + idx);
    endRemoveRows();
    emit countChanged();
}

void FriendListModel::setDialogMeta(int uid,
                                    const QString& dialogType,
                                    int unreadCount,
                                    int pinnedRank,
                                    const QString& draftText,
                                    qint64 lastMsgTs,
                                    int muteState)
{
    const int idx = indexOfUid(uid);
    if (idx < 0)
    {
        return;
    }
    FriendEntry& entry = _items[static_cast<size_t>(idx)];
    bool changed = false;
    if (!dialogType.isEmpty() && entry.dialogType != dialogType)
    {
        entry.dialogType = dialogType;
        changed = true;
    }
    if (entry.unreadCount != unreadCount)
    {
        entry.unreadCount = unreadCount < 0 ? 0 : unreadCount;
        changed = true;
    }
    if (entry.pinnedRank != pinnedRank)
    {
        entry.pinnedRank = pinnedRank;
        changed = true;
    }
    if (entry.draftText != draftText)
    {
        entry.draftText = draftText;
        changed = true;
    }
    if (lastMsgTs > 0 && entry.lastMsgTs != lastMsgTs)
    {
        entry.lastMsgTs = lastMsgTs;
        changed = true;
    }
    if (entry.muteState != muteState)
    {
        entry.muteState = muteState;
        changed = true;
    }
    if (!changed)
    {
        return;
    }
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex,
                     modelIndex,
                     {DialogTypeRole, UnreadCountRole, PinnedRankRole, DraftTextRole, LastMsgTsRole, MuteStateRole});
}
