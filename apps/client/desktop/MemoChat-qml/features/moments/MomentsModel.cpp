#include "MomentsModel.h"
#include <QJsonArray>
#include <QStringList>
#include <QtGlobal>

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
    case TextContentRole: {
        QStringList parts;
        for (const auto& it : item.items) {
            const QString type = it.mediaType.trimmed().toLower();
            const bool mediaWithKey = (type == QStringLiteral("image") || type == QStringLiteral("video"))
                && !it.mediaKey.trimmed().isEmpty();
            if (mediaWithKey) {
                continue;
            }
            if (!it.content.trimmed().isEmpty()) {
                parts.append(it.content);
            }
        }
        return parts.join(QLatin1Char('\n'));
    }
    case LikesRole: {
        QVariantList list;
        for (const auto& lk : item.likes) {
            QVariantMap m;
            m["uid"] = lk.uid;
            m["user_nick"] = lk.userNick;
            m["user_icon"] = lk.userIcon;
            m["created_at"] = lk.createdAt;
            list.append(m);
        }
        return list;
    }
    case CommentsRole: {
        QVariantList list;
        for (const auto& cm : item.comments) {
            QVariantMap m;
            m["id"] = cm.id;
            m["uid"] = cm.uid;
            m["user_nick"] = cm.userNick;
            m["user_icon"] = cm.userIcon;
            m["content"] = cm.content;
            m["reply_uid"] = cm.replyUid;
            m["reply_nick"] = cm.replyNick;
            m["created_at"] = cm.createdAt;
            m["like_count"] = cm.likeCount;
            m["has_liked"] = cm.hasLiked;
            QVariantList likes;
            for (const auto& lk : cm.likes) {
                QVariantMap likeMap;
                likeMap["uid"] = lk.uid;
                likeMap["user_nick"] = lk.userNick;
                likeMap["user_icon"] = lk.userIcon;
                likeMap["created_at"] = lk.createdAt;
                likes.append(likeMap);
            }
            m["likes"] = likes;
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
    roles[TextContentRole]   = "textContent";
    roles[LikesRole]         = "likes";
    roles[CommentsRole]      = "comments";
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

void MomentsModel::prependOrUpdateMoment(const MomentEntry& moment) {
    for (int i = 0; i < _items.size(); ++i) {
        if (_items[i].momentId == moment.momentId) {
            _items[i] = moment;
            if (i == 0) {
                const QModelIndex idx = index(0);
                emit dataChanged(idx, idx);
                return;
            }

            beginMoveRows(QModelIndex(), i, i, QModelIndex(), 0);
            _items.move(i, 0);
            endMoveRows();
            const QModelIndex idx = index(0);
            emit dataChanged(idx, idx);
            return;
        }
    }

    beginInsertRows(QModelIndex(), 0, 0);
    _items.prepend(moment);
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

void MomentsModel::setCommentCount(qint64 momentId, int count) {
    for (int i = 0; i < _items.size(); ++i) {
        if (_items[i].momentId == momentId) {
            _items[i].commentCount = qMax(0, count);
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {CommentCountRole});
            return;
        }
    }
}

void MomentsModel::updateDetail(qint64 momentId, const QVector<MomentLike>& likes, const QVector<MomentComment>& comments) {
    for (int i = 0; i < _items.size(); ++i) {
        if (_items[i].momentId == momentId) {
            _items[i].likes = likes;
            _items[i].comments = comments;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {LikesRole, CommentsRole});
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

MomentEntry MomentsModel::getMomentById(qint64 momentId) const {
    for (const auto& item : _items) {
        if (item.momentId == momentId) {
            return item;
        }
    }
    return MomentEntry();
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

int MomentsModel::getMomentLikeCount(qint64 momentId) const {
    for (const auto& item : _items) {
        if (item.momentId == momentId) {
            return item.likeCount;
        }
    }
    return 0;
}

bool MomentsModel::hasMoment(qint64 momentId) const {
    for (const auto& item : _items) {
        if (item.momentId == momentId) {
            return true;
        }
    }
    return false;
}

QVariantMap MomentsModel::snapshotMoment(qint64 momentId) const {
    QVariantMap map;
    for (const auto& item : _items) {
        if (item.momentId != momentId) {
            continue;
        }
        map[QStringLiteral("momentId")] = item.momentId;
        map[QStringLiteral("uid")] = item.uid;
        map[QStringLiteral("userId")] = item.userId;
        map[QStringLiteral("userName")] = item.userName;
        map[QStringLiteral("userNick")] = item.userNick;
        map[QStringLiteral("userIcon")] = item.userIcon;
        map[QStringLiteral("visibility")] = item.visibility;
        map[QStringLiteral("location")] = item.location;
        map[QStringLiteral("createdAt")] = item.createdAt;
        map[QStringLiteral("likeCount")] = item.likeCount;
        map[QStringLiteral("commentCount")] = item.commentCount;
        map[QStringLiteral("hasLiked")] = item.hasLiked;
        QVariantList itemsList;
        for (const auto& it : item.items) {
            QVariantMap m;
            m[QStringLiteral("seq")] = it.seq;
            m[QStringLiteral("media_type")] = it.mediaType;
            m[QStringLiteral("media_key")] = it.mediaKey;
            m[QStringLiteral("thumb_key")] = it.thumbKey;
            m[QStringLiteral("content")] = it.content;
            m[QStringLiteral("width")] = it.width;
            m[QStringLiteral("height")] = it.height;
            m[QStringLiteral("duration_ms")] = it.durationMs;
            itemsList.append(m);
        }
        map[QStringLiteral("items")] = itemsList;
        QVariantList likesList;
        for (const auto& lk : item.likes) {
            QVariantMap m;
            m[QStringLiteral("uid")] = lk.uid;
            m[QStringLiteral("user_nick")] = lk.userNick;
            m[QStringLiteral("user_icon")] = lk.userIcon;
            m[QStringLiteral("created_at")] = lk.createdAt;
            likesList.append(m);
        }
        map[QStringLiteral("likes")] = likesList;
        QVariantList commentsList;
        for (const auto& cm : item.comments) {
            QVariantMap m;
            m[QStringLiteral("id")] = cm.id;
            m[QStringLiteral("uid")] = cm.uid;
            m[QStringLiteral("user_nick")] = cm.userNick;
            m[QStringLiteral("user_icon")] = cm.userIcon;
            m[QStringLiteral("content")] = cm.content;
            m[QStringLiteral("reply_uid")] = cm.replyUid;
            m[QStringLiteral("reply_nick")] = cm.replyNick;
            m[QStringLiteral("created_at")] = cm.createdAt;
            m[QStringLiteral("like_count")] = cm.likeCount;
            m[QStringLiteral("has_liked")] = cm.hasLiked;
            QVariantList commentLikes;
            for (const auto& lk : cm.likes) {
                QVariantMap likeMap;
                likeMap[QStringLiteral("uid")] = lk.uid;
                likeMap[QStringLiteral("user_nick")] = lk.userNick;
                likeMap[QStringLiteral("user_icon")] = lk.userIcon;
                likeMap[QStringLiteral("created_at")] = lk.createdAt;
                commentLikes.append(likeMap);
            }
            m[QStringLiteral("likes")] = commentLikes;
            commentsList.append(m);
        }
        map[QStringLiteral("comments")] = commentsList;
        return map;
    }
    return map;
}
