#include "FriendListModel.h"
#include "IconPathUtils.h"
#include <QtGlobal>

FriendListModel::FriendListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FriendListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(_items.size());
}

QVariant FriendListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const FriendEntry &entry = _items[static_cast<size_t>(index.row())];
    switch (role) {
    case UidRole:
        return entry.uid;
    case UserIdRole:
        return entry.userId;
    case NameRole:
        return entry.name;
    case NickRole:
        return entry.nick;
    case IconRole:
        return entry.icon;
    case DescRole:
        return entry.desc;
    case LastMsgRole:
        return entry.lastMsg;
    case SexRole:
        return entry.sex;
    case BackRole:
        return entry.back;
    case DialogTypeRole:
        return entry.dialogType;
    case UnreadCountRole:
        return entry.unreadCount;
    case PinnedRankRole:
        return entry.pinnedRank;
    case DraftTextRole:
        return entry.draftText;
    case LastMsgTsRole:
        return entry.lastMsgTs;
    case MuteStateRole:
        return entry.muteState;
    case MentionCountRole:
        return entry.mentionCount;
    default:
        return {};
    }
}

QHash<int, QByteArray> FriendListModel::roleNames() const
{
    return {
        {UidRole, "uid"},
        {UserIdRole, "userId"},
        {NameRole, "name"},
        {NickRole, "nick"},
        {IconRole, "icon"},
        {DescRole, "desc"},
        {LastMsgRole, "lastMsg"},
        {SexRole, "sex"},
        {BackRole, "back"},
        {DialogTypeRole, "dialogType"},
        {UnreadCountRole, "unreadCount"},
        {PinnedRankRole, "pinnedRank"},
        {DraftTextRole, "draftText"},
        {LastMsgTsRole, "lastMsgTs"},
        {MuteStateRole, "muteState"},
        {MentionCountRole, "mentionCount"}
    };
}

int FriendListModel::count() const
{
    return rowCount();
}

void FriendListModel::clear()
{
    if (_items.empty()) {
        return;
    }

    beginResetModel();
    _items.clear();
    endResetModel();
    emit countChanged();
}

void FriendListModel::setFriends(const std::vector<std::shared_ptr<FriendInfo> > &friends)
{
    beginResetModel();
    _items.clear();
    _items.reserve(friends.size());
    for (const auto &friendInfo : friends) {
        if (!friendInfo) {
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

void FriendListModel::appendFriends(const std::vector<std::shared_ptr<FriendInfo> > &friends)
{
    if (friends.empty()) {
        return;
    }

    for (const auto &friendInfo : friends) {
        upsertFriend(friendInfo);
    }
}

void FriendListModel::upsertFriend(const std::shared_ptr<FriendInfo> &friendInfo)
{
    if (!friendInfo) {
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

void FriendListModel::upsertFriend(const std::shared_ptr<AuthInfo> &authInfo)
{
    if (!authInfo) {
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

void FriendListModel::upsertFriend(const std::shared_ptr<AuthRsp> &authRsp)
{
    if (!authRsp) {
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

void FriendListModel::upsertBatch(const std::vector<std::shared_ptr<FriendInfo>> &friends, bool resetFirst)
{
    if (friends.empty()) {
        if (resetFirst) {
            clear();
        }
        return;
    }

    if (resetFirst) {
        beginResetModel();
        _items.clear();
        _items.reserve(friends.size());
        for (const auto &friendInfo : friends) {
            if (!friendInfo) {
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

    std::vector<int> updatedIndices;
    updatedIndices.reserve(friends.size());
    std::vector<int> newIndices;
    newIndices.reserve(friends.size());

    for (const auto &friendInfo : friends) {
        if (!friendInfo) {
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
        if (existingIdx >= 0) {
            _items[static_cast<size_t>(existingIdx)] = entry;
            updatedIndices.push_back(existingIdx);
        } else {
            _items.push_back(entry);
            newIndices.push_back(static_cast<int>(_items.size()) - 1);
        }
    }

    if (!updatedIndices.empty()) {
        for (int idx : updatedIndices) {
            emit dataChanged(index(idx, 0), index(idx, 0));
        }
    }

    if (!newIndices.empty()) {
        beginInsertRows(QModelIndex(), newIndices.front(), newIndices.back());
        endInsertRows();
    }

    if (!updatedIndices.empty() || !newIndices.empty()) {
        emit countChanged();
    }
}

void FriendListModel::updateLastMessage(int uid, const QString &lastMsg, qint64 lastMsgTs)
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }

    FriendEntry &entry = _items[static_cast<size_t>(idx)];
    bool changed = false;
    if (entry.lastMsg != lastMsg) {
        entry.lastMsg = lastMsg;
        changed = true;
    }
    if (lastMsgTs > 0 && entry.lastMsgTs != lastMsgTs) {
        entry.lastMsgTs = lastMsgTs;
        changed = true;
    }
    if (!changed) {
        return;
    }

    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {LastMsgRole, LastMsgTsRole});
}

void FriendListModel::incrementUnread(int uid, int delta)
{
    if (delta <= 0) {
        return;
    }
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }

    FriendEntry &entry = _items[static_cast<size_t>(idx)];
    const int next = entry.unreadCount + delta;
    if (entry.unreadCount == next) {
        return;
    }
    entry.unreadCount = next;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {UnreadCountRole});
}

void FriendListModel::clearUnread(int uid)
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }
    FriendEntry &entry = _items[static_cast<size_t>(idx)];
    if (entry.unreadCount == 0) {
        return;
    }
    entry.unreadCount = 0;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {UnreadCountRole});
}

void FriendListModel::incrementMention(int uid, int delta)
{
    if (delta <= 0) {
        return;
    }
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }

    FriendEntry &entry = _items[static_cast<size_t>(idx)];
    const int next = entry.mentionCount + delta;
    if (entry.mentionCount == next) {
        return;
    }
    entry.mentionCount = next;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {MentionCountRole});
}

void FriendListModel::clearMention(int uid)
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }
    FriendEntry &entry = _items[static_cast<size_t>(idx)];
    if (entry.mentionCount == 0) {
        return;
    }
    entry.mentionCount = 0;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {MentionCountRole});
}

void FriendListModel::setMentionCount(int uid, int count)
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }
    const int normalized = qMax(0, count);
    FriendEntry &entry = _items[static_cast<size_t>(idx)];
    if (entry.mentionCount == normalized) {
        return;
    }
    entry.mentionCount = normalized;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {MentionCountRole});
}

void FriendListModel::removeByUid(int uid)
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), idx, idx);
    _items.erase(_items.begin() + idx);
    endRemoveRows();
    emit countChanged();
}

void FriendListModel::setDialogMeta(int uid, const QString &dialogType, int unreadCount,
                                    int pinnedRank, const QString &draftText, qint64 lastMsgTs, int muteState)
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }
    FriendEntry &entry = _items[static_cast<size_t>(idx)];
    bool changed = false;
    if (!dialogType.isEmpty() && entry.dialogType != dialogType) {
        entry.dialogType = dialogType;
        changed = true;
    }
    if (entry.unreadCount != unreadCount) {
        entry.unreadCount = unreadCount < 0 ? 0 : unreadCount;
        changed = true;
    }
    if (entry.pinnedRank != pinnedRank) {
        entry.pinnedRank = pinnedRank;
        changed = true;
    }
    if (entry.draftText != draftText) {
        entry.draftText = draftText;
        changed = true;
    }
    if (lastMsgTs > 0 && entry.lastMsgTs != lastMsgTs) {
        entry.lastMsgTs = lastMsgTs;
        changed = true;
    }
    if (entry.muteState != muteState) {
        entry.muteState = muteState;
        changed = true;
    }
    if (!changed) {
        return;
    }
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex,
                     {DialogTypeRole, UnreadCountRole, PinnedRankRole, DraftTextRole, LastMsgTsRole, MuteStateRole});
}

QVariantMap FriendListModel::get(int indexValue) const
{
    if (indexValue < 0 || indexValue >= rowCount()) {
        return {};
    }

    const FriendEntry &entry = _items[static_cast<size_t>(indexValue)];
    return {
        {"uid", entry.uid},
        {"userId", entry.userId},
        {"name", entry.name},
        {"nick", entry.nick},
        {"icon", entry.icon},
        {"desc", entry.desc},
        {"lastMsg", entry.lastMsg},
        {"sex", entry.sex},
        {"back", entry.back},
        {"dialogType", entry.dialogType},
        {"unreadCount", entry.unreadCount},
        {"pinnedRank", entry.pinnedRank},
        {"draftText", entry.draftText},
        {"lastMsgTs", entry.lastMsgTs},
        {"muteState", entry.muteState},
        {"mentionCount", entry.mentionCount}
    };
}

int FriendListModel::indexOfUid(int uid) const
{
    for (int i = 0; i < rowCount(); ++i) {
        if (_items[static_cast<size_t>(i)].uid == uid) {
            return i;
        }
    }

    return -1;
}

QString FriendListModel::normalizeIcon(QString icon)
{
    return normalizeIconForQml(icon);
}

void FriendListModel::upsert(const FriendEntry &entry)
{
    const int idx = indexOfUid(entry.uid);
    if (idx >= 0) {
        FriendEntry &stored = _items[static_cast<size_t>(idx)];
        stored = entry;
        const QModelIndex modelIndex = index(idx, 0);
        emit dataChanged(modelIndex, modelIndex);
        return;
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _items.push_back(entry);
    endInsertRows();
    emit countChanged();
}
