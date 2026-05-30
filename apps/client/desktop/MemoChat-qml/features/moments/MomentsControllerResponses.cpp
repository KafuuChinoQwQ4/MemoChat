#include "MomentsController.h"
#include "IconPathUtils.h"
#include "MomentsEntryParser.h"
#include "MomentsResponseParser.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QtGlobal>

void MomentsController::onLoadFeedRsp(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_MOMENTS_LIST)
        return;

    QJsonObject root;
    if (!memochat::moments_response::parseRootObject(res, &root))
    {
        setErrorText(QStringLiteral("数据解析失败"));
        setProgressText(QString());
        setLoading(false);
        return;
    }

    const int errorCode = memochat::moments_response::errorCode(root);
    if (errorCode != 0)
    {
        setErrorText(QStringLiteral("加载失败，错误码: %1").arg(errorCode));
        setProgressText(QString());
        setLoading(false);
        return;
    }

    const QJsonArray momentsArr = root["moments"].toArray();
    const bool hasMore = root["has_more"].toBool();
    setHasMore(hasMore);

    const bool freshLoad = (_last_moment_id == 0);
    QVector<MomentEntry> moments;
    qint64 cursorMomentId = 0;
    for (const auto& momentVar : momentsArr)
    {
        const QJsonObject momentObj = momentVar.toObject();
        moments.append(parseMomentEntry(momentObj));
        const qint64 mid = momentObj["moment_id"].toVariant().toLongLong();
        if (mid > 0 && (cursorMomentId == 0 || mid < cursorMomentId))
        {
            cursorMomentId = mid;
        }
    }

    if (freshLoad)
    {
        _model->setMoments(moments);
    }
    else
    {
        _model->appendMoments(moments);
    }
    if (cursorMomentId > 0)
    {
        _last_moment_id = cursorMomentId;
    }
    else if (freshLoad)
    {
        _last_moment_id = 0;
    }

    if (freshLoad && _pending_published_moment_id > 0)
    {
        if (_model->hasMoment(_pending_published_moment_id))
        {
            _pending_published_moment_id = 0;
        }
        else
        {
            refreshMoment(_pending_published_moment_id);
        }
    }

    setProgressText(QString());
    setLoading(false);
}

void MomentsController::onPublishRsp(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_MOMENTS_PUBLISH)
        return;

    QJsonObject root;
    if (!memochat::moments_response::parseRootObject(res, &root))
    {
        setProgressText(QString());
        setLoading(false);
        emit publishError(QStringLiteral("发布失败"));
        return;
    }

    const int errorCode = memochat::moments_response::errorCode(root);
    if (errorCode != 0)
    {
        setProgressText(QString());
        setLoading(false);
        emit publishError(QStringLiteral("发布失败，错误码: %1").arg(errorCode));
        return;
    }

    const qint64 momentId = root["moment_id"].toVariant().toLongLong();
    setProgressText(QString());
    _pending_published_moment_id = momentId;
    _last_moment_id = 0;
    setLoading(false);
    loadFeed();
    emit publishSuccess(momentId);
}

void MomentsController::onDeleteRsp(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_MOMENTS_DELETE)
        return;

    QJsonObject root;
    if (!memochat::moments_response::parseRootObject(res, &root))
    {
        return;
    }

    const int errorCode = memochat::moments_response::errorCode(root);
    if (errorCode == 0)
    {
        const qint64 momentId = root["moment_id"].toVariant().toLongLong();
        if (momentId > 0)
        {
            _model->removeMoment(momentId);
        }
    }
}

void MomentsController::onLikeRsp(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_MOMENTS_LIKE)
    {
        return;
    }
    qDebug() << "[MomentsController] onLikeRsp err=" << err << "res=" << res;

    auto firstInFlightMoment = [this]() -> qint64
    {
        for (auto it = _pending_likes.constBegin(); it != _pending_likes.constEnd(); ++it)
        {
            if (it.value().requestInFlight)
            {
                return it.key();
            }
        }
        return 0;
    };

    auto failPending = [this](qint64 momentId)
    {
        auto it = _pending_likes.find(momentId);
        if (it == _pending_likes.end())
        {
            return;
        }

        PendingLike& pending = it.value();
        pending.requestInFlight = false;
        _like_in_flight.remove(momentId);

        if (pending.desiredLiked != pending.rollbackLiked)
        {
            const int desiredCount =
                pending.desiredLiked ? pending.rollbackCount + 1 : qMax(0, pending.rollbackCount - 1);
            pending.desiredCount = desiredCount;
            _model->updateLiked(momentId, pending.desiredLiked, desiredCount);
            emit likeToggled(momentId, pending.desiredLiked, desiredCount);
            pending.requestInFlight = true;
            pending.requestLiked = pending.desiredLiked;
            _like_in_flight.insert(momentId);
            submitLikeRequest(momentId, pending.requestLiked);
            return;
        }

        _model->updateLiked(momentId, pending.rollbackLiked, pending.rollbackCount);
        emit likeToggled(momentId, pending.rollbackLiked, pending.rollbackCount);
        _pending_likes.erase(it);
    };

    if (err != ErrorCodes::SUCCESS)
    {
        failPending(firstInFlightMoment());
        return;
    }

    QJsonObject root;
    if (!memochat::moments_response::parseRootObject(res, &root))
    {
        failPending(firstInFlightMoment());
        return;
    }

    const int errorCode = memochat::moments_response::errorCode(root);
    const qint64 responseMomentId = root.value(QStringLiteral("moment_id")).toVariant().toLongLong();
    const qint64 momentId = responseMomentId > 0 ? responseMomentId : firstInFlightMoment();
    if (errorCode != 0)
    {
        failPending(momentId);
        return;
    }

    if (root.contains(QStringLiteral("has_liked")) && root.contains(QStringLiteral("like_count")))
    {
        const bool liked = root.value(QStringLiteral("has_liked")).toBool();
        const int likeCount = qMax(0, root.value(QStringLiteral("like_count")).toInt());
        auto it = _pending_likes.find(momentId);
        if (it == _pending_likes.end())
        {
            _authoritative_liked[momentId] = liked;
            _authoritative_like_counts[momentId] = likeCount;
            _model->updateLiked(momentId, liked, likeCount);
            emit likeToggled(momentId, liked, likeCount);
            return;
        }

        PendingLike& pending = it.value();
        pending.requestInFlight = false;
        _like_in_flight.remove(momentId);

        if (pending.desiredLiked == liked)
        {
            _authoritative_liked[momentId] = liked;
            _authoritative_like_counts[momentId] = likeCount;
            _model->updateLiked(momentId, liked, likeCount);
            emit likeToggled(momentId, liked, likeCount);
            _pending_likes.erase(it);
            return;
        }

        pending.rollbackLiked = liked;
        pending.rollbackCount = likeCount;
        pending.desiredCount = pending.desiredLiked ? likeCount + 1 : qMax(0, likeCount - 1);
        _model->updateLiked(momentId, pending.desiredLiked, pending.desiredCount);
        emit likeToggled(momentId, pending.desiredLiked, pending.desiredCount);
        pending.requestInFlight = true;
        pending.requestLiked = pending.desiredLiked;
        _like_in_flight.insert(momentId);
        submitLikeRequest(momentId, pending.requestLiked);
        return;
    }

    refreshMoment(momentId);
}

void MomentsController::onCommentRsp(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_MOMENTS_COMMENT)
        return;

    const qint64 pendingMomentId = _pending_comment_moments.take(ReqId::ID_MOMENTS_COMMENT);
    auto finishComment = [this](qint64 momentId)
    {
        emit commentAdded(momentId);
        if (momentId > 0)
        {
            refreshComments(momentId);
        }
    };

    if (err != ErrorCodes::SUCCESS)
    {
        finishComment(pendingMomentId);
        return;
    }

    QJsonObject root;
    if (!memochat::moments_response::parseRootObject(res, &root))
    {
        finishComment(pendingMomentId);
        return;
    }

    const int errorCode = memochat::moments_response::errorCode(root);
    const qint64 responseMomentId = root["moment_id"].toVariant().toLongLong();
    const qint64 momentId = responseMomentId > 0 ? responseMomentId : pendingMomentId;
    if (errorCode != 0)
    {
        finishComment(momentId);
        return;
    }

    const bool deleted = root["delete"].toBool();
    if (momentId > 0)
    {
        if (root.contains(QStringLiteral("comment_count")))
        {
            const int commentCount = qMax(0, root.value(QStringLiteral("comment_count")).toInt());
            _authoritative_comment_counts[momentId] = commentCount;
            _model->setCommentCount(momentId, commentCount);
        }
        else
        {
            _model->updateCommentCount(momentId, deleted ? -1 : 1);
        }
    }
    finishComment(momentId);
}

void MomentsController::onCommentLikeRsp(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_MOMENTS_COMMENT_LIKE)
        return;

    const qint64 pendingMomentId = _pending_comment_moments.take(ReqId::ID_MOMENTS_COMMENT_LIKE);
    if (pendingMomentId > 0)
    {
        refreshComments(pendingMomentId);
    }
}

void MomentsController::onDetailRsp(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_MOMENTS_DETAIL)
        return;
    qDebug() << "[MomentsController] onDetailRsp called, res length=" << res.length() << " err=" << err;

    QJsonObject root;
    QString parseError;
    if (!memochat::moments_response::parseRootObject(res, &root, &parseError))
    {
        qDebug() << "[MomentsController] onDetailRsp: JSON parse error" << parseError;
        return;
    }

    const int errorCode = memochat::moments_response::errorCode(root);
    qDebug() << "[MomentsController] onDetailRsp: errorCode=" << errorCode;
    if (errorCode != 0)
    {
        return;
    }

    const QJsonObject momentObj = root["moment"].toObject();
    const MomentEntry entry = parseMomentEntry(momentObj);
    qDebug() << "[MomentsController] onDetailRsp: parsed momentId=" << entry.momentId
             << " comments=" << entry.comments.size();
    const MomentEntry previous = _model->getMomentById(entry.momentId);
    if (_pending_published_moment_id > 0 && entry.momentId == _pending_published_moment_id)
    {
        _model->prependOrUpdateMoment(entry);
        _pending_published_moment_id = 0;
    }
    else
    {
        _model->upsertMoment(entry);
    }
    const int authoritativeCommentCount = _authoritative_comment_counts.value(entry.momentId, -1);
    if (authoritativeCommentCount >= 0 && previous.comments.size() >= authoritativeCommentCount &&
        previous.comments.size() > entry.comments.size())
    {
        _model->updateDetail(entry.momentId, entry.likes, previous.comments);
    }
    else if (!entry.comments.isEmpty())
    {
        _model->updateDetail(entry.momentId, entry.likes, entry.comments);
    }
    else
    {
        _model->updateDetail(entry.momentId, entry.likes, previous.comments);
    }
    emit momentRefreshed(entry.momentId);
}

void MomentsController::onCommentListRsp(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_MOMENTS_COMMENT_LIST)
        return;

    const qint64 pendingMomentId = _pending_comment_moments.take(ReqId::ID_MOMENTS_COMMENT_LIST);
    auto finishLoading = [this](qint64 momentId)
    {
        if (momentId > 0 && _comments_loading.remove(momentId))
        {
            emit commentsLoadingChanged(momentId, false);
        }
    };

    QJsonObject root;
    if (err != ErrorCodes::SUCCESS || !memochat::moments_response::parseRootObject(res, &root))
    {
        finishLoading(pendingMomentId);
        emit commentAdded(pendingMomentId);
        return;
    }

    const int errorCode = memochat::moments_response::errorCode(root);
    qint64 momentId = root["moment_id"].toVariant().toLongLong();
    if (momentId <= 0)
        momentId = pendingMomentId;
    if (errorCode != 0 || momentId <= 0)
    {
        finishLoading(momentId > 0 ? momentId : pendingMomentId);
        emit commentAdded(momentId);
        return;
    }

    const QVector<MomentComment> comments = parseMomentComments(root["comments"].toArray());

    const MomentEntry current = _model->getMomentById(momentId);
    _model->updateDetail(momentId, current.likes, comments);
    if (root.contains(QStringLiteral("comment_count")))
    {
        const int commentCount = qMax(0, root.value(QStringLiteral("comment_count")).toInt());
        _authoritative_comment_counts[momentId] = commentCount;
        _model->setCommentCount(momentId, commentCount);
    }
    finishLoading(momentId);
    emit momentRefreshed(momentId);
}
