#include "MomentsController.h"
#include "MomentsResponseParser.h"

#include <QJsonObject>
#include <QtGlobal>

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
