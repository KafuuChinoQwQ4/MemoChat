#include "MomentsController.h"
#include "MomentsEntryParser.h"

#include <QJsonObject>

MomentEntry MomentsController::parseMomentEntry(const QJsonObject& obj) const
{
    MomentEntry entry = parseMomentEntryFromJson(obj);
    applyAuthoritativeState(entry);
    return entry;
}

void MomentsController::applyAuthoritativeState(MomentEntry& entry) const
{
    if (entry.momentId <= 0)
    {
        return;
    }
    const auto pendingIt = _pending_likes.constFind(entry.momentId);
    if (pendingIt != _pending_likes.constEnd())
    {
        entry.hasLiked = pendingIt.value().desiredLiked;
        entry.likeCount = qMax(0, pendingIt.value().desiredCount);
        return;
    }
    if (_authoritative_liked.contains(entry.momentId))
    {
        entry.hasLiked = _authoritative_liked.value(entry.momentId);
    }
    if (_authoritative_like_counts.contains(entry.momentId))
    {
        entry.likeCount = qMax(0, _authoritative_like_counts.value(entry.momentId));
    }
    if (_authoritative_comment_counts.contains(entry.momentId))
    {
        entry.commentCount = qMax(0, _authoritative_comment_counts.value(entry.momentId));
    }
}
