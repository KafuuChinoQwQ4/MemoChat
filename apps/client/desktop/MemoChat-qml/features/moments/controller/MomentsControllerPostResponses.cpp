#include "MomentsController.h"
#include "MomentsResponseParser.h"

#include <QDebug>
#include <QJsonObject>
#include <QtGlobal>

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
            if (pending.retryCount >= kMaxLikeRetries)
            {
                // Retry limit reached: give up and roll back to the last known server state
                // so the UI doesn't stay permanently out-of-sync.
                qWarning() << "[MomentsController] like retry limit reached for momentId=" << momentId
                           << ", rolling back UI";
                _model->updateLiked(momentId, pending.rollbackLiked, pending.rollbackCount);
                emit likeToggled(momentId, pending.rollbackLiked, pending.rollbackCount);
                _pending_likes.erase(it);
                return;
            }
            pending.retryCount++;
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
