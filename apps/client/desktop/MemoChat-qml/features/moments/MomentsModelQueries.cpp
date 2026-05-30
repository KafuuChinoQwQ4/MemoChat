#include "MomentsModel.h"

QVariantMap MomentsModel::get(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= _items.size())
    {
        return map;
    }
    const auto& item = _items.at(index);
    map["momentId"] = item.momentId;
    map["uid"] = item.uid;
    map["userName"] = item.userName;
    map["userNick"] = item.userNick;
    map["userIcon"] = item.userIcon;
    map["visibility"] = item.visibility;
    map["location"] = item.location;
    map["createdAt"] = item.createdAt;
    map["likeCount"] = item.likeCount;
    map["commentCount"] = item.commentCount;
    map["hasLiked"] = item.hasLiked;
    return map;
}

MomentEntry MomentsModel::getMoment(int index) const
{
    if (index < 0 || index >= _items.size())
    {
        return MomentEntry();
    }
    return _items.at(index);
}

MomentEntry MomentsModel::getMomentById(qint64 momentId) const
{
    for (const auto& item : _items)
    {
        if (item.momentId == momentId)
        {
            return item;
        }
    }
    return MomentEntry();
}

qint64 MomentsModel::getMomentId(int index) const
{
    if (index < 0 || index >= _items.size())
    {
        return 0;
    }
    return _items.at(index).momentId;
}

bool MomentsModel::getMomentLiked(qint64 momentId) const
{
    for (const auto& item : _items)
    {
        if (item.momentId == momentId)
        {
            return item.hasLiked;
        }
    }
    return false;
}

int MomentsModel::getMomentLikeCount(qint64 momentId) const
{
    for (const auto& item : _items)
    {
        if (item.momentId == momentId)
        {
            return item.likeCount;
        }
    }
    return 0;
}

bool MomentsModel::hasMoment(qint64 momentId) const
{
    for (const auto& item : _items)
    {
        if (item.momentId == momentId)
        {
            return true;
        }
    }
    return false;
}

QVariantMap MomentsModel::snapshotMoment(qint64 momentId) const
{
    QVariantMap map;
    for (const auto& item : _items)
    {
        if (item.momentId != momentId)
        {
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
        for (const auto& it : item.items)
        {
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
        for (const auto& lk : item.likes)
        {
            QVariantMap m;
            m[QStringLiteral("uid")] = lk.uid;
            m[QStringLiteral("user_nick")] = lk.userNick;
            m[QStringLiteral("user_icon")] = lk.userIcon;
            m[QStringLiteral("created_at")] = lk.createdAt;
            likesList.append(m);
        }
        map[QStringLiteral("likes")] = likesList;
        QVariantList commentsList;
        for (const auto& cm : item.comments)
        {
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
            for (const auto& lk : cm.likes)
            {
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
