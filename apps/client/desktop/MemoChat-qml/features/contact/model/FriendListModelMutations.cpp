#include "FriendListModel.h"

#include <utility>

FriendListModel::FriendEntry FriendListModel::toEntry(const std::shared_ptr<FriendInfo>& friendInfo) const
{
    FriendEntry entry;
    if (!friendInfo)
    {
        return entry;
    }
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
    refreshDerivedFields(entry);
    return entry;
}

FriendListModel::FriendEntry FriendListModel::mergeWithCurrentEntry(const FriendEntry& entry) const
{
    const int idx = indexOfUid(entry.uid);
    if (idx < 0)
    {
        return entry;
    }
    return mergeSparseEntry(entry, _items[static_cast<size_t>(idx)]);
}

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
    std::vector<FriendEntry> nextItems;
    nextItems.reserve(friends.size());
    for (const auto& friendInfo : friends)
    {
        if (!friendInfo)
        {
            continue;
        }

        nextItems.push_back(mergeWithCurrentEntry(toEntry(friendInfo)));
    }
    if (_contact_section_ordering_enabled)
    {
        sortForContactSections(nextItems);
    }

    beginResetModel();
    _items = std::move(nextItems);
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

void FriendListModel::setContactSectionOrderingEnabled(bool enabled)
{
    if (_contact_section_ordering_enabled == enabled)
    {
        return;
    }

    _contact_section_ordering_enabled = enabled;
    if (_items.empty())
    {
        return;
    }

    beginResetModel();
    if (_contact_section_ordering_enabled)
    {
        sortForContactSections(_items);
    }
    else
    {
        for (FriendEntry& entry : _items)
        {
            refreshDerivedFields(entry);
        }
    }
    endResetModel();
}

void FriendListModel::upsertFriend(const std::shared_ptr<FriendInfo>& friendInfo)
{
    if (!friendInfo)
    {
        return;
    }

    upsert(toEntry(friendInfo));
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
    refreshDerivedFields(entry);
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
    refreshDerivedFields(entry);
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
        std::vector<FriendEntry> nextItems;
        nextItems.reserve(friends.size());
        for (const auto& friendInfo : friends)
        {
            if (!friendInfo)
            {
                continue;
            }

            nextItems.push_back(mergeWithCurrentEntry(toEntry(friendInfo)));
        }
        if (_contact_section_ordering_enabled)
        {
            sortForContactSections(nextItems);
        }

        beginResetModel();
        _items = std::move(nextItems);
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

        upsert(toEntry(friendInfo));
        changed = true;
    }

    if (changed)
    {
        emit countChanged();
    }
}
