#include "MomentsController.h"
#include "global.h"
#include "httpmgr.h"

#include <QDebug>
#include <QJsonObject>
#include <QUrl>
#include <QtGlobal>

void MomentsController::loadFeed() {
    loadFeedForAuthor(_author_filter_uid);
}

void MomentsController::loadFeedForAuthor(int authorUid) {
    if (_loading) return;
    setAuthorFilterUid(authorUid);
    _last_moment_id = 0;
    setLoading(true);
    setErrorText(QString());
    setProgressText(QString());

    QJsonObject payload = buildAuthJson();
    payload["last_moment_id"] = 0;
    payload["limit"] = kPageSize;
    if (_author_filter_uid > 0) {
        payload["author_uid"] = _author_filter_uid;
    }

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/list"),
        payload,
        ReqId::ID_MOMENTS_LIST,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::loadMore() {
    if (_loading || !_has_more) return;
    setLoading(true);
    setProgressText(QString());

    QJsonObject payload = buildAuthJson();
    payload["last_moment_id"] = _last_moment_id;
    payload["limit"] = kPageSize;
    if (_author_filter_uid > 0) {
        payload["author_uid"] = _author_filter_uid;
    }

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/list"),
        payload,
        ReqId::ID_MOMENTS_LIST,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::deleteMoment(qint64 momentId) {
    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/delete"),
        payload,
        ReqId::ID_MOMENTS_DELETE,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::submitLikeRequest(qint64 momentId, bool liked) {
    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;
    payload["like"] = liked;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/like"),
        payload,
        ReqId::ID_MOMENTS_LIKE,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::toggleLike(qint64 momentId) {
    qDebug() << "[MomentsController] toggleLike requested momentId=" << momentId
             << "inFlight=" << _like_in_flight.contains(momentId);
    if (momentId <= 0) {
        return;
    }

    const bool wasLiked = _model->getMomentLiked(momentId);
    const int prevCount = _model->getMomentLikeCount(momentId);
    const int nextCount = wasLiked ? qMax(0, prevCount - 1) : prevCount + 1;

    auto it = _pending_likes.find(momentId);
    if (it == _pending_likes.end()) {
        PendingLike pending;
        pending.momentId = momentId;
        pending.rollbackLiked = wasLiked;
        pending.rollbackCount = prevCount;
        pending.desiredLiked = !wasLiked;
        pending.desiredCount = nextCount;
        pending.requestInFlight = true;
        pending.requestLiked = pending.desiredLiked;
        _pending_likes.insert(momentId, pending);
        _like_in_flight.insert(momentId);
        _model->updateLiked(momentId, pending.desiredLiked, pending.desiredCount);
        emit likeToggled(momentId, pending.desiredLiked, pending.desiredCount);
        submitLikeRequest(momentId, pending.requestLiked);
        return;
    }

    PendingLike& pending = it.value();
    pending.desiredLiked = !wasLiked;
    pending.desiredCount = nextCount;
    _model->updateLiked(momentId, pending.desiredLiked, pending.desiredCount);
    emit likeToggled(momentId, pending.desiredLiked, pending.desiredCount);

    if (!pending.requestInFlight) {
        pending.requestInFlight = true;
        pending.requestLiked = pending.desiredLiked;
        _like_in_flight.insert(momentId);
        submitLikeRequest(momentId, pending.requestLiked);
    }
}

void MomentsController::addComment(qint64 momentId, const QString& content, int replyUid) {
    if (momentId <= 0 || content.trimmed().isEmpty()) return;

    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;
    payload["content"] = content.trimmed();
    payload["reply_uid"] = replyUid;

    _pending_comment_moments[ReqId::ID_MOMENTS_COMMENT] = momentId;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/comment"),
        payload,
        ReqId::ID_MOMENTS_COMMENT,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::deleteComment(qint64 momentId, qint64 commentId) {
    if (momentId <= 0 || commentId <= 0) return;

    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;
    payload["comment_id"] = commentId;
    payload["delete"] = true;

    _pending_comment_moments[ReqId::ID_MOMENTS_COMMENT] = momentId;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/comment"),
        payload,
        ReqId::ID_MOMENTS_COMMENT,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::refreshMoment(qint64 momentId) {
    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/detail"),
        payload,
        ReqId::ID_MOMENTS_DETAIL,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::toggleCommentLike(qint64 momentId, qint64 commentId, bool liked) {
    if (momentId <= 0 || commentId <= 0) return;

    QJsonObject payload = buildAuthJson();
    payload["comment_id"] = commentId;
    payload["like"] = liked;

    _pending_comment_moments[ReqId::ID_MOMENTS_COMMENT_LIKE] = momentId;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/comment/like"),
        payload,
        ReqId::ID_MOMENTS_COMMENT_LIKE,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::refreshComments(qint64 momentId) {
    if (momentId <= 0) return;

    if (!_comments_loading.contains(momentId)) {
        _comments_loading.insert(momentId);
        emit commentsLoadingChanged(momentId, true);
    }
    _pending_comment_moments[ReqId::ID_MOMENTS_COMMENT_LIST] = momentId;

    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;
    payload["last_comment_id"] = 0;
    payload["limit"] = 50;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/comment/list"),
        payload,
        ReqId::ID_MOMENTS_COMMENT_LIST,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}
