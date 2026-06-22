#include "MomentsController.h"
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

    qDebug() << "[MomentsController] onCommentListRsp: err=" << err << "res=" << res.left(300);
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
