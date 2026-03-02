#include "FriendListModel.h"
#include "IconPathUtils.h"

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
    default:
        return {};
    }
}

QHash<int, QByteArray> FriendListModel::roleNames() const
{
    return {
        {UidRole, "uid"},
        {NameRole, "name"},
        {NickRole, "nick"},
        {IconRole, "icon"},
        {DescRole, "desc"},
        {LastMsgRole, "lastMsg"},
        {SexRole, "sex"},
        {BackRole, "back"}
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
        entry.name = friendInfo->_name;
        entry.nick = friendInfo->_nick;
        entry.icon = normalizeIcon(friendInfo->_icon);
        entry.desc = friendInfo->_desc;
        entry.lastMsg = friendInfo->_last_msg;
        entry.sex = friendInfo->_sex;
        entry.back = friendInfo->_back;
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
    entry.name = friendInfo->_name;
    entry.nick = friendInfo->_nick;
    entry.icon = normalizeIcon(friendInfo->_icon);
    entry.desc = friendInfo->_desc;
    entry.lastMsg = friendInfo->_last_msg;
    entry.sex = friendInfo->_sex;
    entry.back = friendInfo->_back;
    upsert(entry);
}

void FriendListModel::upsertFriend(const std::shared_ptr<AuthInfo> &authInfo)
{
    if (!authInfo) {
        return;
    }

    FriendEntry entry;
    entry.uid = authInfo->_uid;
    entry.name = authInfo->_name;
    entry.nick = authInfo->_nick;
    entry.icon = normalizeIcon(authInfo->_icon);
    entry.sex = authInfo->_sex;
    entry.back = authInfo->_nick;
    upsert(entry);
}

void FriendListModel::upsertFriend(const std::shared_ptr<AuthRsp> &authRsp)
{
    if (!authRsp) {
        return;
    }

    FriendEntry entry;
    entry.uid = authRsp->_uid;
    entry.name = authRsp->_name;
    entry.nick = authRsp->_nick;
    entry.icon = normalizeIcon(authRsp->_icon);
    entry.sex = authRsp->_sex;
    entry.back = authRsp->_nick;
    upsert(entry);
}

void FriendListModel::updateLastMessage(int uid, const QString &lastMsg)
{
    const int idx = indexOfUid(uid);
    if (idx < 0) {
        return;
    }

    FriendEntry &entry = _items[static_cast<size_t>(idx)];
    if (entry.lastMsg == lastMsg) {
        return;
    }

    entry.lastMsg = lastMsg;
    const QModelIndex modelIndex = index(idx, 0);
    emit dataChanged(modelIndex, modelIndex, {LastMsgRole});
}

QVariantMap FriendListModel::get(int indexValue) const
{
    if (indexValue < 0 || indexValue >= rowCount()) {
        return {};
    }

    const FriendEntry &entry = _items[static_cast<size_t>(indexValue)];
    return {
        {"uid", entry.uid},
        {"name", entry.name},
        {"nick", entry.nick},
        {"icon", entry.icon},
        {"desc", entry.desc},
        {"lastMsg", entry.lastMsg},
        {"sex", entry.sex},
        {"back", entry.back}
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
