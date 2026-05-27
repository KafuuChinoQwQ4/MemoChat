#include "MomentsController.h"
#include "IconPathUtils.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QtGlobal>

MomentEntry MomentsController::parseMomentEntry(const QJsonObject& obj) const {
    MomentEntry entry;
    entry.momentId = obj["moment_id"].toVariant().toLongLong();
    entry.uid = obj["uid"].toInt();
    entry.userId = obj["user_id"].toString();
    entry.userName = obj["user_name"].toString();
    entry.userNick = obj["user_nick"].toString();
    if (entry.userNick.trimmed().isEmpty()) {
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
    for (const auto& itemVar : items) {
        const QJsonObject itemObj = itemVar.toObject();
        MomentItem item;
        item.seq = itemObj["seq"].toInt();
        item.mediaType = itemObj["media_type"].toString();
        item.mediaKey = itemObj["media_key"].toString();
        item.thumbKey = itemObj["thumb_key"].toString();
        item.content = itemObj["content"].toString();
        item.width = itemObj["width"].toInt();
        item.height = itemObj["height"].toInt();
        item.durationMs = itemObj["duration_ms"].toInt();
        entry.items.append(item);
    }

    const QJsonArray likes = obj["likes"].toArray();
    for (const auto& likeVar : likes) {
        const QJsonObject likeObj = likeVar.toObject();
        MomentLike lk;
        lk.uid = likeObj["uid"].toInt();
        lk.userNick = likeObj["user_nick"].toString();
        if (lk.userNick.trimmed().isEmpty()) {
            lk.userNick = QStringLiteral("用户");
        }
        lk.userIcon = normalizeIconForQml(likeObj["user_icon"].toString());
        lk.createdAt = likeObj["created_at"].toVariant().toLongLong();
        entry.likes.append(lk);
    }

    const QJsonArray comments = obj["comments"].toArray();
    for (const auto& cmVar : comments) {
        const QJsonObject cmObj = cmVar.toObject();
        MomentComment cm;
        cm.id = cmObj["id"].toVariant().toLongLong();
        cm.uid = cmObj["uid"].toInt();
        cm.userNick = cmObj["user_nick"].toString();
        if (cm.userNick.trimmed().isEmpty()) {
            cm.userNick = QStringLiteral("用户");
        }
        cm.userIcon = normalizeIconForQml(cmObj["user_icon"].toString());
        cm.content = cmObj["content"].toString();
        cm.replyUid = cmObj["reply_uid"].toInt();
        cm.replyNick = cmObj["reply_nick"].toString();
        cm.createdAt = cmObj["created_at"].toVariant().toLongLong();
        cm.likeCount = qMax(0, cmObj["like_count"].toInt());
        cm.hasLiked = cmObj["has_liked"].toBool();
        const QJsonArray commentLikes = cmObj["likes"].toArray();
        for (const auto& likeVar : commentLikes) {
            const QJsonObject likeObj = likeVar.toObject();
            MomentLike lk;
            lk.uid = likeObj["uid"].toInt();
            lk.userNick = likeObj["user_nick"].toString();
            if (lk.userNick.trimmed().isEmpty()) {
                lk.userNick = QStringLiteral("用户");
            }
            lk.userIcon = normalizeIconForQml(likeObj["user_icon"].toString());
            lk.createdAt = likeObj["created_at"].toVariant().toLongLong();
            cm.likes.append(lk);
        }
        entry.comments.append(cm);
    }

    applyAuthoritativeState(entry);
    return entry;
}

void MomentsController::applyAuthoritativeState(MomentEntry& entry) const {
    if (entry.momentId <= 0) {
        return;
    }
    const auto pendingIt = _pending_likes.constFind(entry.momentId);
    if (pendingIt != _pending_likes.constEnd()) {
        entry.hasLiked = pendingIt.value().desiredLiked;
        entry.likeCount = qMax(0, pendingIt.value().desiredCount);
        return;
    }
    if (_authoritative_liked.contains(entry.momentId)) {
        entry.hasLiked = _authoritative_liked.value(entry.momentId);
    }
    if (_authoritative_like_counts.contains(entry.momentId)) {
        entry.likeCount = qMax(0, _authoritative_like_counts.value(entry.momentId));
    }
    if (_authoritative_comment_counts.contains(entry.momentId)) {
        entry.commentCount = qMax(0, _authoritative_comment_counts.value(entry.momentId));
    }
}
