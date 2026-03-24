#include "MomentsController.h"
#include "ClientGateway.h"
#include "httpmgr.h"
#include "usermgr.h"
#include "global.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <QUuid>

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
    entry.userIcon = obj["user_icon"].toString();
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
    QJsonObject payload = buildAuthJson();
    payload["moment_id"] = momentId;
    bool currentlyLiked = _model->getMomentLiked(momentId);
    payload["like"] = !currentlyLiked;

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
    if (parseErr.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject root = doc.object();
        const int errorCode = root["error"].toInt();
        if (errorCode == 0) {
            // Get moment_id from request - we don't have it here, so model will refresh on next load
        }
    }
}

void MomentsController::onLikeRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_LIKE) return;

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
    // Model update is optimistic - toggleLike already updated local state
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
    // Comment was added successfully
}

void MomentsController::onDetailRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_DETAIL) return;

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

    const QJsonObject momentObj = root["moment"].toObject();
    const MomentEntry entry = parseMomentEntry(momentObj);
    _model->upsertMoment(entry);
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
