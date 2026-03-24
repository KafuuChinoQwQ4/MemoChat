#include "MomentsModel.h"
#include <QJsonArray>

MomentsModel::MomentsModel(QObject* parent)
    : QAbstractListModel(parent) {
}

int MomentsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return _items.size();
}

QVariant MomentsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= _items.size()) {
        return QVariant();
    }

    const auto& item = _items.at(index.row());
    switch (role) {
    case MomentIdRole:      return item.momentId;
    case UidRole:           return item.uid;
    case UserIdRole:        return item.userId;
    case UserNameRole:      return item.userName;
    case UserNickRole:      return item.userNick;
    case UserIconRole:      return item.userIcon;
    case VisibilityRole:    return item.visibility;
    case LocationRole:      return item.location;
    case CreatedAtRole:     return item.createdAt;
    case LikeCountRole:     return item.likeCount;
    case CommentCountRole:  return item.commentCount;
    case HasLikedRole:      return item.hasLiked;
    case ItemsRole: {
        QVariantList list;
        for (const auto& it : item.items) {
            QVariantMap m;
            m["seq"] = it.seq;
            m["media_type"] = it.mediaType;
            m["media_key"] = it.mediaKey;
            m["thumb_key"] = it.thumbKey;
            m["content"] = it.content;
            m["width"] = it.width;
            m["height"] = it.height;
            m["duration_ms"] = it.durationMs;
            list.append(m);
        }
        return list;
    }
    default: return QVariant();
    }
}

QHash<int, QByteArray> MomentsModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[MomentIdRole]      = "momentId";
    roles[UidRole]           = "uid";
    roles[UserIdRole]        = "userId";
    roles[UserNameRole]      = "userName";
    roles[UserNickRole]     = "userNick";
    roles[UserIconRole]      = "userIcon";
    roles[VisibilityRole]    = "visibility";
    roles[LocationRole]      = "location";
    roles[CreatedAtRole]     = "createdAt";
    roles[LikeCountRole]     = "likeCount";
    roles[CommentCountRole]  = "commentCount";
    roles[HasLikedRole]      = "hasLiked";
    roles[ItemsRole]         = "items";
    return roles;
}

int MomentsModel::count() const {
    return _items.size();
}

void MomentsModel::clear() {
    if (_items.isEmpty()) {
        return;
    }
    beginResetModel();
    _items.clear();
    endResetModel();
    emit countChanged();
}

void MomentsModel::setMoments(const QVector<MomentEntry>& moments) {
    beginResetModel();
    _items = moments;
    endResetModel();
    emit countChanged();
}

void MomentsModel::appendMoments(const QVector<MomentEntry>& moments) {
    if (moments.isEmpty()) {
        return;
    }
    int startRow = _items.size();
    beginInsertRows(QModelIndex(), startRow, startRow + moments.size() - 1);
    _items.append(moments);
    endInsertRows();
    emit countChanged();
}

void MomentsModel::upsertMoment(const MomentEntry& moment) {
    for (int i = 0; i < _items.size(); ++i) {
        if (_items[i].momentId == moment.momentId) {
            _items[i] = moment;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx);
            return;
        }
    }
    beginInsertRows(QModelIndex(), _items.size(), _items.size());
    _items.append(moment);
    endInsertRows();
    emit countChanged();
}

void MomentsModel::updateLiked(qint64 momentId, bool liked, int likeCount) {
    for (int i = 0; i < _items.size(); ++i) {
        if (_items[i].momentId == momentId) {
            _items[i].hasLiked = liked;
            _items[i].likeCount = likeCount;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {HasLikedRole, LikeCountRole});
            return;
        }
    }
}

void MomentsModel::updateCommentCount(qint64 momentId, int delta) {
    for (int i = 0; i < _items.size(); ++i) {
        if (_items[i].momentId == momentId) {
            _items[i].commentCount += delta;
            if (_items[i].commentCount < 0) _items[i].commentCount = 0;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {CommentCountRole});
            return;
        }
    }
}

void MomentsModel::removeMoment(qint64 momentId) {
    for (int i = 0; i < _items.size(); ++i) {
        if (_items[i].momentId == momentId) {
            beginRemoveRows(QModelIndex(), i, i);
            _items.remove(i);
            endRemoveRows();
            emit countChanged();
            return;
        }
    }
}

QVariantMap MomentsModel::get(int index) const {
    QVariantMap map;
    if (index < 0 || index >= _items.size()) {
        return map;
    }
    const auto& item = _items.at(index);
    map["momentId"]    = item.momentId;
    map["uid"]        = item.uid;
    map["userName"]   = item.userName;
    map["userNick"]   = item.userNick;
    map["userIcon"]   = item.userIcon;
    map["visibility"] = item.visibility;
    map["location"]   = item.location;
    map["createdAt"]  = item.createdAt;
    map["likeCount"]  = item.likeCount;
    map["commentCount"] = item.commentCount;
    map["hasLiked"]   = item.hasLiked;
    return map;
}

MomentEntry MomentsModel::getMoment(int index) const {
    if (index < 0 || index >= _items.size()) {
        return MomentEntry();
    }
    return _items.at(index);
}

qint64 MomentsModel::getMomentId(int index) const {
    if (index < 0 || index >= _items.size()) {
        return 0;
    }
    return _items.at(index).momentId;
}

bool MomentsModel::getMomentLiked(qint64 momentId) const {
    for (const auto& item : _items) {
        if (item.momentId == momentId) {
            return item.hasLiked;
        }
    }
    return false;
}
