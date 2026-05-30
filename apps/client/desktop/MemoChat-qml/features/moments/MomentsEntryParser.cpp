#include "MomentsEntryParser.h"

#include "IconPathUtils.h"

#include <QtGlobal>

MomentItem parseMomentItem(const QJsonObject& obj)
{
    MomentItem item;
    item.seq = obj["seq"].toInt();
    item.mediaType = obj["media_type"].toString();
    item.mediaKey = obj["media_key"].toString();
    item.thumbKey = obj["thumb_key"].toString();
    item.content = obj["content"].toString();
    item.width = obj["width"].toInt();
    item.height = obj["height"].toInt();
    item.durationMs = obj["duration_ms"].toInt();
    return item;
}

MomentLike parseMomentLike(const QJsonObject& obj)
{
    MomentLike like;
    like.uid = obj["uid"].toInt();
    like.userNick = obj["user_nick"].toString();
    if (like.userNick.trimmed().isEmpty())
    {
        like.userNick = QStringLiteral("用户");
    }
    like.userIcon = normalizeIconForQml(obj["user_icon"].toString());
    like.createdAt = obj["created_at"].toVariant().toLongLong();
    return like;
}

MomentComment parseMomentComment(const QJsonObject& obj)
{
    MomentComment comment;
    comment.id = obj["id"].toVariant().toLongLong();
    comment.uid = obj["uid"].toInt();
    comment.userNick = obj["user_nick"].toString();
    if (comment.userNick.trimmed().isEmpty())
    {
        comment.userNick = QStringLiteral("用户");
    }
    comment.userIcon = normalizeIconForQml(obj["user_icon"].toString());
    comment.content = obj["content"].toString();
    comment.replyUid = obj["reply_uid"].toInt();
    comment.replyNick = obj["reply_nick"].toString();
    comment.createdAt = obj["created_at"].toVariant().toLongLong();
    comment.likeCount = qMax(0, obj["like_count"].toInt());
    comment.hasLiked = obj["has_liked"].toBool();
    const QJsonArray likes = obj["likes"].toArray();
    comment.likes.reserve(likes.size());
    for (const auto& likeVar : likes)
    {
        comment.likes.append(parseMomentLike(likeVar.toObject()));
    }
    return comment;
}

QVector<MomentComment> parseMomentComments(const QJsonArray& arr)
{
    QVector<MomentComment> comments;
    comments.reserve(arr.size());
    for (const auto& commentVar : arr)
    {
        comments.append(parseMomentComment(commentVar.toObject()));
    }
    return comments;
}

MomentEntry parseMomentEntryFromJson(const QJsonObject& obj)
{
    MomentEntry entry;
    entry.momentId = obj["moment_id"].toVariant().toLongLong();
    entry.uid = obj["uid"].toInt();
    entry.userId = obj["user_id"].toString();
    entry.userName = obj["user_name"].toString();
    entry.userNick = obj["user_nick"].toString();
    if (entry.userNick.trimmed().isEmpty())
    {
        entry.userNick = entry.userName;
    }
    entry.userIcon = normalizeIconForQml(obj["user_icon"].toString());
    entry.visibility = obj["visibility"].toInt();
    entry.location = obj["location"].toString();
    entry.createdAt = obj["created_at"].toVariant().toLongLong();
    entry.likeCount = obj["like_count"].toInt();
    entry.commentCount = obj["comment_count"].toInt();
    entry.hasLiked = obj["has_liked"].toBool();

    const QJsonArray items = obj["items"].toArray();
    entry.items.reserve(items.size());
    for (const auto& itemVar : items)
    {
        entry.items.append(parseMomentItem(itemVar.toObject()));
    }

    const QJsonArray likes = obj["likes"].toArray();
    entry.likes.reserve(likes.size());
    for (const auto& likeVar : likes)
    {
        entry.likes.append(parseMomentLike(likeVar.toObject()));
    }

    entry.comments = parseMomentComments(obj["comments"].toArray());
    return entry;
}
