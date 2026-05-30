#include "FriendListModel.h"

FriendListModel::FriendListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int FriendListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return static_cast<int>(_items.size());
}

QVariant FriendListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
    {
        return {};
    }

    const FriendEntry& entry = _items[static_cast<size_t>(index.row())];
    switch (role)
    {
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
    return {{UidRole, "uid"},
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
            {MentionCountRole, "mentionCount"}};
}

int FriendListModel::count() const
{
    return rowCount();
}

void FriendListModel::upsert(const FriendEntry& entry)
{
    const int idx = indexOfUid(entry.uid);
    if (idx >= 0)
    {
        FriendEntry& stored = _items[static_cast<size_t>(idx)];
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
