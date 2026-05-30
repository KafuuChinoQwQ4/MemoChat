#include "FriendListModel.h"

void FriendListModel::clear()
{
    if (_items.empty())
    {
        return;
    }

    beginResetModel();
    _items.clear();
    endResetModel();
    emit countChanged();
}

void FriendListModel::setFriends(const std::vector<std::shared_ptr<FriendInfo>>& friends)
{
    beginResetModel();
    _items.clear();
    _items.reserve(friends.size());
    for (const auto& friendInfo : friends)
    {
        if (!friendInfo)
        {
            continue;
        }

        FriendEntry entry;
        entry.uid = friendInfo->_uid;
        entry.userId = friendInfo->_user_id;
        entry.name = friendInfo->_name;
        entry.nick = friendInfo->_nick;
        entry.icon = normalizeIcon(friendInfo->_icon);
        entry.desc = friendInfo->_desc;
        entry.lastMsg = friendInfo->_last_msg;
        entry.sex = friendInfo->_sex;
        entry.back = friendInfo->_back;
        entry.dialogType = friendInfo->_dialog_type;
        entry.unreadCount = friendInfo->_unread_count;
        entry.pinnedRank = friendInfo->_pinned_rank;
        entry.draftText = friendInfo->_draft_text;
        entry.lastMsgTs = friendInfo->_last_msg_ts;
        entry.muteState = friendInfo->_mute_state;
        entry.mentionCount = friendInfo->_mention_count;
        _items.push_back(entry);
    }
    endResetModel();
    emit countChanged();
}

void FriendListModel::appendFriends(const std::vector<std::shared_ptr<FriendInfo>>& friends)
{
    if (friends.empty())
    {
        return;
    }

    for (const auto& friendInfo : friends)
    {
        upsertFriend(friendInfo);
    }
}

void FriendListModel::upsertFriend(const std::shared_ptr<FriendInfo>& friendInfo)
{
    if (!friendInfo)
    {
        return;
    }

    FriendEntry entry;
    entry.uid = friendInfo->_uid;
    entry.userId = friendInfo->_user_id;
    entry.name = friendInfo->_name;
    entry.nick = friendInfo->_nick;
    entry.icon = normalizeIcon(friendInfo->_icon);
    entry.desc = friendInfo->_desc;
    entry.lastMsg = friendInfo->_last_msg;
    entry.sex = friendInfo->_sex;
    entry.back = friendInfo->_back;
    entry.dialogType = friendInfo->_dialog_type;
    entry.unreadCount = friendInfo->_unread_count;
    entry.pinnedRank = friendInfo->_pinned_rank;
    entry.draftText = friendInfo->_draft_text;
    entry.lastMsgTs = friendInfo->_last_msg_ts;
    entry.muteState = friendInfo->_mute_state;
    entry.mentionCount = friendInfo->_mention_count;
    upsert(entry);
}

void FriendListModel::upsertFriend(const std::shared_ptr<AuthInfo>& authInfo)
{
    if (!authInfo)
    {
        return;
    }

    FriendEntry entry;
    entry.uid = authInfo->_uid;
    entry.userId = authInfo->_user_id;
    entry.name = authInfo->_name;
    entry.nick = authInfo->_nick;
    entry.icon = normalizeIcon(authInfo->_icon);
    entry.sex = authInfo->_sex;
    entry.back = authInfo->_nick;
    entry.dialogType = QStringLiteral("private");
    upsert(entry);
}

void FriendListModel::upsertFriend(const std::shared_ptr<AuthRsp>& authRsp)
{
    if (!authRsp)
    {
        return;
    }

    FriendEntry entry;
    entry.uid = authRsp->_uid;
    entry.userId = authRsp->_user_id;
    entry.name = authRsp->_name;
    entry.nick = authRsp->_nick;
    entry.icon = normalizeIcon(authRsp->_icon);
    entry.sex = authRsp->_sex;
    entry.back = authRsp->_nick;
    entry.dialogType = QStringLiteral("private");
    upsert(entry);
}

void FriendListModel::upsertBatch(const std::vector<std::shared_ptr<FriendInfo>>& friends, bool resetFirst)
{
    if (friends.empty())
    {
        if (resetFirst)
        {
            clear();
        }
        return;
    }

    if (resetFirst)
    {
        beginResetModel();
        _items.clear();
        _items.reserve(friends.size());
        for (const auto& friendInfo : friends)
        {
            if (!friendInfo)
            {
                continue;
            }

            FriendEntry entry;
            entry.uid = friendInfo->_uid;
            entry.userId = friendInfo->_user_id;
            entry.name = friendInfo->_name;
            entry.nick = friendInfo->_nick;
            entry.icon = normalizeIcon(friendInfo->_icon);
            entry.desc = friendInfo->_desc;
            entry.lastMsg = friendInfo->_last_msg;
            entry.sex = friendInfo->_sex;
            entry.back = friendInfo->_back;
            entry.dialogType = friendInfo->_dialog_type;
            entry.unreadCount = friendInfo->_unread_count;
            entry.pinnedRank = friendInfo->_pinned_rank;
            entry.draftText = friendInfo->_draft_text;
            entry.lastMsgTs = friendInfo->_last_msg_ts;
            entry.muteState = friendInfo->_mute_state;
            entry.mentionCount = friendInfo->_mention_count;
            _items.push_back(entry);
        }
        endResetModel();
        emit countChanged();
        return;
    }

    bool changed = false;

    for (const auto& friendInfo : friends)
    {
        if (!friendInfo)
        {
            continue;
        }

        FriendEntry entry;
        entry.uid = friendInfo->_uid;
        entry.userId = friendInfo->_user_id;
        entry.name = friendInfo->_name;
        entry.nick = friendInfo->_nick;
        entry.icon = normalizeIcon(friendInfo->_icon);
        entry.desc = friendInfo->_desc;
        entry.lastMsg = friendInfo->_last_msg;
        entry.sex = friendInfo->_sex;
        entry.back = friendInfo->_back;
        entry.dialogType = friendInfo->_dialog_type;
        entry.unreadCount = friendInfo->_unread_count;
        entry.pinnedRank = friendInfo->_pinned_rank;
        entry.draftText = friendInfo->_draft_text;
        entry.lastMsgTs = friendInfo->_last_msg_ts;
        entry.muteState = friendInfo->_mute_state;
        entry.mentionCount = friendInfo->_mention_count;

        const int existingIdx = indexOfUid(entry.uid);
        if (existingIdx >= 0)
        {
            _items[static_cast<size_t>(existingIdx)] = entry;
            emit dataChanged(index(existingIdx, 0), index(existingIdx, 0));
        }
        else
        {
            const int insertRow = rowCount();
            beginInsertRows(QModelIndex(), insertRow, insertRow);
            _items.push_back(entry);
            endInsertRows();
        }
        changed = true;
    }

    if (changed)
    {
        emit countChanged();
    }
}
