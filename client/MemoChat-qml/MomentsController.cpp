#include "MomentsController.h"
#include "ClientGateway.h"
#include "httpmgr.h"
#include "usermgr.h"
#include "global.h"
#include "IconPathUtils.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <QUuid>
#include <QtGlobal>

MomentsController::MomentsController(QObject* parent)
    : QObject(parent)
    , _model(new MomentsModel(this)) {
    auto httpMgr = HttpMgr::GetInstance();
    if (httpMgr) {
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish,
                        this, &MomentsController::onLoadFeedRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish,
                        this, &MomentsController::onPublishRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish,
                        this, &MomentsController::onDeleteRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish,
                        this, &MomentsController::onLikeRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish,
                        this, &MomentsController::onCommentRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish,
                        this, &MomentsController::onDetailRsp);
    }
}

MomentsController::~MomentsController() {
}

void MomentsController::setLoading(bool v) {
    if (_loading != v) {
        _loading = v;
        emit loadingChanged();
    }
}

void MomentsController::setHasMore(bool v) {
    if (_has_more != v) {
        _has_more = v;
        emit hasMoreChanged();
    }
}

void MomentsController::setErrorText(const QString& text) {
    if (_error_text != text) {
        _error_text = text;
        emit errorTextChanged();
    }
}

QJsonObject MomentsController::buildAuthJson() const {
    auto um = UserMgr::GetInstance();
    QJsonObject obj;
    if (um) {
        obj["uid"] = um->GetUid();
        obj["login_ticket"] = um->GetToken();
    }
    return obj;
}

MomentEntry MomentsController::parseMomentEntry(const QJsonObject& obj) const {
    MomentEntry entry;
    entry.momentId = obj["moment_id"].toVariant().toLongLong();
    entry.uid = obj["uid"].toInt();
    entry.userId = obj["user_id"].toString();
    entry.userName = obj["user_name"].toString();
    entry.userNick = obj["user_nick"].toString();
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
        cm.userIcon = normalizeIconForQml(cmObj["user_icon"].toString());
        cm.content = cmObj["content"].toString();
        cm.replyUid = cmObj["reply_uid"].toInt();
        cm.replyNick = cmObj["reply_nick"].toString();
        cm.createdAt = cmObj["created_at"].toVariant().toLongLong();
        entry.comments.append(cm);
    }

    return entry;
}

void MomentsController::loadFeed() {
    if (_loading) return;
    setLoading(true);
    setErrorText(QString());

    QJsonObject payload = buildAuthJson();
    payload["last_moment_id"] = 0;
    payload["limit"] = kPageSize;

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

    QJsonObject payload = buildAuthJson();
    payload["last_moment_id"] = _last_moment_id;
    payload["limit"] = kPageSize;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/list"),
        payload,
        ReqId::ID_MOMENTS_LIST,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::publishMoment(const QString& location, int visibility, const QVariantList& items) {
    if (_loading) return;
    setLoading(true);
    setErrorText(QString());

    QJsonObject payload = buildAuthJson();
    payload["location"] = location;
    payload["visibility"] = visibility;

    QJsonArray itemsArr;
    for (const auto& itemVar : items) {
        const QVariantMap itemMap = itemVar.toMap();
        QJsonObject itemObj;
        for (auto it = itemMap.constBegin(); it != itemMap.constEnd(); ++it) {
            itemObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        itemsArr.append(itemObj);
    }
    payload["items"] = itemsArr;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/publish"),
        payload,
        ReqId::ID_MOMENTS_PUBLISH,
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

void MomentsController::toggleLike(qint64 momentId) {
    if (momentId <= 0 || _like_in_flight.contains(momentId)) {
        return;
    }

    const bool wasLiked = _model->getMomentLiked(momentId);
    const int prevCount = _model->getMomentLikeCount(momentId);
    const int nextCount = wasLiked ? qMax(0, prevCount - 1) : prevCount + 1;

    PendingLike pl;
    pl.momentId = momentId;
    pl.wasLiked = wasLiked;
    pl.prevCount = prevCount;
    _like_pending_queue.enqueue(pl);
    _like_in_flight.insert(momentId);

    _model->updateLiked(momentId, !wasLiked, nextCount);

    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;
    payload["like"] = !wasLiked;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/like"),
        payload,
        ReqId::ID_MOMENTS_LIKE,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::addComment(qint64 momentId, const QString& content, int replyUid) {
    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;
    payload["content"] = content;
    payload["reply_uid"] = replyUid;

    HttpMgr::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/api/moments/comment"),
        payload,
        ReqId::ID_MOMENTS_COMMENT,
        Modules::MOMENTSMOD,
        QStringLiteral("moments"));
}

void MomentsController::deleteComment(qint64 momentId, qint64 commentId) {
    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;
    payload["comment_id"] = commentId;
    payload["delete"] = true;

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

void MomentsController::onLoadFeedRsp(ReqId id, const QString& res, ErrorCodes err) {
    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        setErrorText(QStringLiteral("数据解析失败"));
        setLoading(false);
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    if (errorCode != 0) {
        setErrorText(QStringLiteral("加载失败，错误码: %1").arg(errorCode));
        setLoading(false);
        return;
    }

    const QJsonArray momentsArr = root["moments"].toArray();
    const bool hasMore = root["has_more"].toBool();
    setHasMore(hasMore);

    QVector<MomentEntry> moments;
    qint64 lastId = 0;
    for (const auto& momentVar : momentsArr) {
        const QJsonObject momentObj = momentVar.toObject();
        moments.append(parseMomentEntry(momentObj));
        const qint64 mid = momentObj["moment_id"].toVariant().toLongLong();
        if (mid > lastId) lastId = mid;
    }

    if (id == ReqId::ID_MOMENTS_LIST) {
        // If last_moment_id was 0, this is a fresh load
        if (_last_moment_id == 0) {
            _model->setMoments(moments);
        } else {
            _model->appendMoments(moments);
        }
        _last_moment_id = lastId;
    }

    setLoading(false);
}

void MomentsController::onPublishRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_PUBLISH) return;

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit publishError(QStringLiteral("发布失败"));
        setLoading(false);
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    if (errorCode != 0) {
        emit publishError(QStringLiteral("发布失败，错误码: %1").arg(errorCode));
        setLoading(false);
        return;
    }

    const qint64 momentId = root["moment_id"].toVariant().toLongLong();
    emit publishSuccess(momentId);

    // Refresh the feed
    _last_moment_id = 0;
    loadFeed();
}

void MomentsController::onDeleteRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_DELETE) return;

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    if (errorCode == 0) {
        const qint64 momentId = root["moment_id"].toVariant().toLongLong();
        if (momentId > 0) {
            _model->removeMoment(momentId);
        }
    }
}

void MomentsController::onLikeRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_LIKE) {
        return;
    }

    PendingLike p;
    if (!_like_pending_queue.isEmpty()) {
        p = _like_pending_queue.dequeue();
    }
    if (p.momentId > 0) {
        _like_in_flight.remove(p.momentId);
    }

    auto revert = [this, p]() {
        if (p.momentId > 0) {
            _model->updateLiked(p.momentId, p.wasLiked, p.prevCount);
        }
    };

    if (err != ErrorCodes::SUCCESS) {
        revert();
        return;
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        revert();
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    if (errorCode != 0) {
        revert();
        return;
    }
    // Success: optimistic UI already applied in toggleLike
}

void MomentsController::onCommentRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_COMMENT) return;

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    if (errorCode != 0) {
        return;
    }

    const qint64 momentId = root["moment_id"].toVariant().toLongLong();
    const bool deleted = root["delete"].toBool();
    if (momentId > 0) {
        _model->updateCommentCount(momentId, deleted ? -1 : 1);
    }
}

void MomentsController::onDetailRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_DETAIL) return;
    qDebug() << "[MomentsController] onDetailRsp called, res length=" << res.length() << " err=" << err;

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "[MomentsController] onDetailRsp: JSON parse error" << parseErr.errorString();
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    qDebug() << "[MomentsController] onDetailRsp: errorCode=" << errorCode;
    if (errorCode != 0) {
        return;
    }

    const QJsonObject momentObj = root["moment"].toObject();
    const MomentEntry entry = parseMomentEntry(momentObj);
    qDebug() << "[MomentsController] onDetailRsp: parsed momentId=" << entry.momentId << " comments=" << entry.comments.size();
    _model->upsertMoment(entry);
    _model->updateDetail(entry.momentId, entry.likes, entry.comments);
    emit momentRefreshed(entry.momentId);
}

// Static helpers
QVariantList MomentsController::buildTextItem(const QString& content) {
    QVariantMap item;
    item["media_type"] = QStringLiteral("text");
    item["content"] = content;
    QVariantList list;
    list.append(item);
    return list;
}

QVariantList MomentsController::buildImageItem(const QString& mediaKey, const QString& thumbKey,
                                               int width, int height) {
    QVariantMap item;
    item["media_type"] = QStringLiteral("image");
    item["media_key"] = mediaKey;
    item["thumb_key"] = thumbKey.isEmpty() ? mediaKey : thumbKey;
    item["width"] = width;
    item["height"] = height;
    QVariantList list;
    list.append(item);
    return list;
}

QVariantList MomentsController::buildVideoItem(const QString& mediaKey, const QString& thumbKey,
                                               int width, int height, int durationMs) {
    QVariantMap item;
    item["media_type"] = QStringLiteral("video");
    item["media_key"] = mediaKey;
    item["thumb_key"] = thumbKey.isEmpty() ? mediaKey : thumbKey;
    item["width"] = width;
    item["height"] = height;
    item["duration_ms"] = durationMs;
    QVariantList list;
    list.append(item);
    return list;
}
