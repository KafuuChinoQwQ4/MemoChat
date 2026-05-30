#include "MomentsModel.h"
#include <QJsonArray>
#include <QStringList>
#include <QtGlobal>

MomentsModel::MomentsModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int MomentsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return _items.size();
}

QVariant MomentsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= _items.size())
    {
        return QVariant();
    }

    const auto& item = _items.at(index.row());
    switch (role)
    {
        case MomentIdRole:
            return item.momentId;
        case UidRole:
            return item.uid;
        case UserIdRole:
            return item.userId;
        case UserNameRole:
            return item.userName;
        case UserNickRole:
            return item.userNick;
        case UserIconRole:
            return item.userIcon;
        case VisibilityRole:
            return item.visibility;
        case LocationRole:
            return item.location;
        case CreatedAtRole:
            return item.createdAt;
        case LikeCountRole:
            return item.likeCount;
        case CommentCountRole:
            return item.commentCount;
        case HasLikedRole:
            return item.hasLiked;
        case ItemsRole:
        {
            QVariantList list;
            for (const auto& it : item.items)
            {
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
        case TextContentRole:
        {
            QStringList parts;
            for (const auto& it : item.items)
            {
                const QString type = it.mediaType.trimmed().toLower();
                const bool mediaWithKey = (type == QStringLiteral("image") || type == QStringLiteral("video")) &&
                                                                  !it.mediaKey.trimmed().isEmpty();
                if (mediaWithKey)
                {
                    continue;
                }
                if (!it.content.trimmed().isEmpty())
                {
                    parts.append(it.content);
                }
            }
            return parts.join(QLatin1Char('\n'));
        }
        case LikesRole:
        {
            QVariantList list;
            for (const auto& lk : item.likes)
            {
                QVariantMap m;
                m["uid"] = lk.uid;
                m["user_nick"] = lk.userNick;
                m["user_icon"] = lk.userIcon;
                m["created_at"] = lk.createdAt;
                list.append(m);
            }
            return list;
        }
        case CommentsRole:
        {
            QVariantList list;
            for (const auto& cm : item.comments)
            {
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
                for (const auto& lk : cm.likes)
                {
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
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> MomentsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[MomentIdRole] = "momentId";
    roles[UidRole] = "uid";
    roles[UserIdRole] = "userId";
    roles[UserNameRole] = "userName";
    roles[UserNickRole] = "userNick";
    roles[UserIconRole] = "userIcon";
    roles[VisibilityRole] = "visibility";
    roles[LocationRole] = "location";
    roles[CreatedAtRole] = "createdAt";
    roles[LikeCountRole] = "likeCount";
    roles[CommentCountRole] = "commentCount";
    roles[HasLikedRole] = "hasLiked";
    roles[ItemsRole] = "items";
    roles[TextContentRole] = "textContent";
    roles[LikesRole] = "likes";
    roles[CommentsRole] = "comments";
    return roles;
}

int MomentsModel::count() const
{
    return _items.size();
}
