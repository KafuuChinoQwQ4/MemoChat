#include "MomentsController.h"
#include "ClientGateway.h"
#include "LocalFilePickerService.h"
#include "MediaUploadService.h"
#include "httpmgr.h"
#include "usermgr.h"
#include "global.h"
#include "IconPathUtils.h"
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QtGlobal>
#include <QtConcurrent>

namespace {
struct MomentPublishTaskResult {
    bool ok = false;
    QString errorText;
    QVariantList items;
};

QVariantMap normalizeMomentAttachment(const QVariantMap& source)
{
    QVariantMap attachment = source;
    const QString type = attachment.value(QStringLiteral("type")).toString().trimmed();
    const QString fileUrl = attachment.value(QStringLiteral("fileUrl")).toString().trimmed();
    if ((type != QStringLiteral("image") && type != QStringLiteral("video")) || fileUrl.isEmpty()) {
        return {};
    }

    if (attachment.value(QStringLiteral("attachmentId")).toString().trimmed().isEmpty()) {
        attachment.insert(QStringLiteral("attachmentId"),
                          QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    if (attachment.value(QStringLiteral("fileName")).toString().trimmed().isEmpty()) {
        attachment.insert(QStringLiteral("fileName"), QFileInfo(QUrl(fileUrl).toLocalFile()).fileName());
    }
    return attachment;
}
}

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
                        this, &MomentsController::onCommentLikeRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish,
                        this, &MomentsController::onDetailRsp);
        QObject::connect(httpMgr.get(), &HttpMgr::sig_moments_mod_finish,
                        this, &MomentsController::onCommentListRsp);
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

void MomentsController::setProgressText(const QString& text) {
    if (_progress_text != text) {
        _progress_text = text;
        emit progressTextChanged();
    }
}

void MomentsController::setAuthorFilterUid(int uid) {
    const int normalizedUid = qMax(0, uid);
    if (_author_filter_uid != normalizedUid) {
        _author_filter_uid = normalizedUid;
        emit authorFilterChanged();
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

void MomentsController::publishMoment(const QString& location, int visibility, const QVariantList& items) {
    if (_loading) return;
    setLoading(true);
    setErrorText(QString());
    setProgressText(QStringLiteral("正在发布朋友圈..."));

    submitPublishRequest(location, visibility, items, false);
}

QVariantList MomentsController::pickMomentMedia()
{
    setErrorText(QString());

    QVariantList attachments;
    QString errorText;
    if (!LocalFilePickerService::pickMomentMediaUrls(&attachments, &errorText)) {
        if (!errorText.isEmpty()) {
            setErrorText(errorText);
        }
        return {};
    }

    QVariantList normalized;
    normalized.reserve(attachments.size());
    for (const QVariant& attachmentVar : attachments) {
        const QVariantMap normalizedAttachment = normalizeMomentAttachment(attachmentVar.toMap());
        if (!normalizedAttachment.isEmpty()) {
            normalized.push_back(normalizedAttachment);
        }
    }
    return normalized;
}

void MomentsController::publishDraftMoment(const QString& text, int visibility,
                                           const QVariantList& attachments)
{
    if (_loading) {
        return;
    }

    const QString trimmedText = text.trimmed();
    QVariantList normalizedAttachments;
    normalizedAttachments.reserve(attachments.size());
    for (const QVariant& attachmentVar : attachments) {
        const QVariantMap normalizedAttachment = normalizeMomentAttachment(attachmentVar.toMap());
        if (!normalizedAttachment.isEmpty()) {
            normalizedAttachments.push_back(normalizedAttachment);
        }
    }

    if (trimmedText.isEmpty() && normalizedAttachments.isEmpty()) {
        setErrorText(QStringLiteral("请输入内容或添加图片/视频"));
        emit publishError(_error_text);
        return;
    }

    auto um = UserMgr::GetInstance();
    if (!um || um->GetUid() <= 0 || um->GetToken().trimmed().isEmpty()) {
        setErrorText(QStringLiteral("登录态失效，请重新登录"));
        emit publishError(_error_text);
        return;
    }

    setLoading(true);
    setErrorText(QString());
    setProgressText(normalizedAttachments.isEmpty()
                        ? QStringLiteral("正在发布朋友圈...")
                        : QStringLiteral("正在准备素材 0/%1").arg(normalizedAttachments.size()));

    auto* watcher = new QFutureWatcher<MomentPublishTaskResult>(this);
    const int uid = um->GetUid();
    const QString token = um->GetToken();
    const int attachmentCount = normalizedAttachments.size();

    const auto future = QtConcurrent::run([this, trimmedText, normalizedAttachments, uid, token, attachmentCount]() {
        MomentPublishTaskResult result;
        QVariantList items;

        if (!trimmedText.isEmpty()) {
            items += MomentsController::buildTextItem(trimmedText);
        }

        for (int index = 0; index < normalizedAttachments.size(); ++index) {
            const QVariantMap attachment = normalizedAttachments.at(index).toMap();
            const QString uploadType = attachment.value(QStringLiteral("type")).toString();
            const QString fileUrl = attachment.value(QStringLiteral("fileUrl")).toString();

            UploadedMediaInfo uploaded;
            QString uploadError;
            const bool ok = MediaUploadService::uploadLocalFile(
                fileUrl,
                uploadType,
                uid,
                token,
                &uploaded,
                &uploadError,
                [this, index, attachmentCount](int percent, const QString& stage) {
                    QMetaObject::invokeMethod(this, [this, index, attachmentCount, percent, stage]() {
                        setProgressText(QStringLiteral("正在上传素材 %1/%2 %3 %4%")
                                            .arg(index + 1)
                                            .arg(qMax(1, attachmentCount))
                                            .arg(stage)
                                            .arg(qBound(0, percent, 100)));
                    }, Qt::QueuedConnection);
                });
            if (!ok) {
                result.errorText = uploadError.isEmpty()
                                       ? QStringLiteral("素材上传失败")
                                       : uploadError;
                return result;
            }

            QString mediaKey = uploaded.mediaKey;
            if (mediaKey.isEmpty()) {
                const QUrl uploadedUrl(uploaded.remoteUrl);
                const QUrlQuery query(uploadedUrl);
                mediaKey = query.queryItemValue(QStringLiteral("asset"));
            }
            if (mediaKey.isEmpty()) {
                result.errorText = QStringLiteral("上传完成，但未返回有效媒体标识");
                return result;
            }

            const int width = attachment.value(QStringLiteral("width")).toInt();
            const int height = attachment.value(QStringLiteral("height")).toInt();
            if (uploadType == QStringLiteral("video")) {
                items += MomentsController::buildVideoItem(
                    mediaKey,
                    QString(),
                    width,
                    height,
                    attachment.value(QStringLiteral("durationMs")).toInt());
            } else {
                items += MomentsController::buildImageItem(mediaKey, QString(), width, height);
            }
        }

        result.ok = true;
        result.items = items;
        return result;
    });

    connect(watcher, &QFutureWatcher<MomentPublishTaskResult>::finished, this,
            [this, watcher, visibility]() {
        const MomentPublishTaskResult result = watcher->result();
        watcher->deleteLater();

        if (!result.ok) {
            setLoading(false);
            setProgressText(QString());
            setErrorText(result.errorText.isEmpty()
                             ? QStringLiteral("发布失败")
                             : result.errorText);
            emit publishError(_error_text);
            return;
        }

        setProgressText(QStringLiteral("正在提交朋友圈..."));
        submitPublishRequest(QString(), visibility, result.items, false);
    });
    watcher->setFuture(future);
}

void MomentsController::submitPublishRequest(const QString& location, int visibility,
                                             const QVariantList& items, bool manageLoading) {
    if (manageLoading) {
        if (_loading) return;
        setLoading(true);
        setErrorText(QString());
        setProgressText(QStringLiteral("正在发布朋友圈..."));
    }

    QJsonObject payload = buildAuthJson();
    payload["location"] = location;
    payload["visibility"] = visibility;

    QJsonArray itemsArr;
    QString firstTextContent;
    for (const auto& itemVar : items) {
        const QVariantMap itemMap = itemVar.toMap();
        QJsonObject itemObj;
        for (auto it = itemMap.constBegin(); it != itemMap.constEnd(); ++it) {
            itemObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        if (firstTextContent.isEmpty()
            && itemMap.value(QStringLiteral("media_type")).toString() == QStringLiteral("text")) {
            firstTextContent = itemMap.value(QStringLiteral("content")).toString();
        }
        itemsArr.append(itemObj);
    }
    payload["items"] = itemsArr;
    if (!firstTextContent.trimmed().isEmpty()) {
        payload["content"] = firstTextContent;
    }

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

void MomentsController::onLoadFeedRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_LIST) return;

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        setErrorText(QStringLiteral("数据解析失败"));
        setProgressText(QString());
        setLoading(false);
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    if (errorCode != 0) {
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
    for (const auto& momentVar : momentsArr) {
        const QJsonObject momentObj = momentVar.toObject();
        moments.append(parseMomentEntry(momentObj));
        const qint64 mid = momentObj["moment_id"].toVariant().toLongLong();
        if (mid > 0 && (cursorMomentId == 0 || mid < cursorMomentId)) {
            cursorMomentId = mid;
        }
    }

    if (freshLoad) {
        _model->setMoments(moments);
    } else {
        _model->appendMoments(moments);
    }
    if (cursorMomentId > 0) {
        _last_moment_id = cursorMomentId;
    } else if (freshLoad) {
        _last_moment_id = 0;
    }

    if (freshLoad && _pending_published_moment_id > 0) {
        if (_model->hasMoment(_pending_published_moment_id)) {
            _pending_published_moment_id = 0;
        } else {
            refreshMoment(_pending_published_moment_id);
        }
    }

    setProgressText(QString());
    setLoading(false);
}

void MomentsController::onPublishRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_PUBLISH) return;

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        setProgressText(QString());
        setLoading(false);
        emit publishError(QStringLiteral("发布失败"));
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    if (errorCode != 0) {
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
    qDebug() << "[MomentsController] onLikeRsp err=" << err << "res=" << res;

    auto firstInFlightMoment = [this]() -> qint64 {
        for (auto it = _pending_likes.constBegin(); it != _pending_likes.constEnd(); ++it) {
            if (it.value().requestInFlight) {
                return it.key();
            }
        }
        return 0;
    };

    auto failPending = [this](qint64 momentId) {
        auto it = _pending_likes.find(momentId);
        if (it == _pending_likes.end()) {
            return;
        }

        PendingLike& pending = it.value();
        pending.requestInFlight = false;
        _like_in_flight.remove(momentId);

        if (pending.desiredLiked != pending.rollbackLiked) {
            const int desiredCount = pending.desiredLiked
                ? pending.rollbackCount + 1
                : qMax(0, pending.rollbackCount - 1);
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

    if (err != ErrorCodes::SUCCESS) {
        failPending(firstInFlightMoment());
        return;
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        failPending(firstInFlightMoment());
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    const qint64 responseMomentId = root.value(QStringLiteral("moment_id")).toVariant().toLongLong();
    const qint64 momentId = responseMomentId > 0 ? responseMomentId : firstInFlightMoment();
    if (errorCode != 0) {
        failPending(momentId);
        return;
    }

    if (root.contains(QStringLiteral("has_liked")) && root.contains(QStringLiteral("like_count"))) {
        const bool liked = root.value(QStringLiteral("has_liked")).toBool();
        const int likeCount = qMax(0, root.value(QStringLiteral("like_count")).toInt());
        auto it = _pending_likes.find(momentId);
        if (it == _pending_likes.end()) {
            _authoritative_liked[momentId] = liked;
            _authoritative_like_counts[momentId] = likeCount;
            _model->updateLiked(momentId, liked, likeCount);
            emit likeToggled(momentId, liked, likeCount);
            return;
        }

        PendingLike& pending = it.value();
        pending.requestInFlight = false;
        _like_in_flight.remove(momentId);

        if (pending.desiredLiked == liked) {
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

void MomentsController::onCommentRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_COMMENT) return;

    const qint64 pendingMomentId = _pending_comment_moments.take(ReqId::ID_MOMENTS_COMMENT);
    auto finishComment = [this](qint64 momentId) {
        emit commentAdded(momentId);
        if (momentId > 0) {
            refreshComments(momentId);
        }
    };

    if (err != ErrorCodes::SUCCESS) {
        finishComment(pendingMomentId);
        return;
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        finishComment(pendingMomentId);
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    const qint64 responseMomentId = root["moment_id"].toVariant().toLongLong();
    const qint64 momentId = responseMomentId > 0 ? responseMomentId : pendingMomentId;
    if (errorCode != 0) {
        finishComment(momentId);
        return;
    }

    const bool deleted = root["delete"].toBool();
    if (momentId > 0) {
        if (root.contains(QStringLiteral("comment_count"))) {
            const int commentCount = qMax(0, root.value(QStringLiteral("comment_count")).toInt());
            _authoritative_comment_counts[momentId] = commentCount;
            _model->setCommentCount(momentId, commentCount);
        } else {
            _model->updateCommentCount(momentId, deleted ? -1 : 1);
        }
    }
    finishComment(momentId);
}

void MomentsController::onCommentLikeRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_COMMENT_LIKE) return;

    const qint64 pendingMomentId = _pending_comment_moments.take(ReqId::ID_MOMENTS_COMMENT_LIKE);
    if (pendingMomentId > 0) {
        refreshComments(pendingMomentId);
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
    const MomentEntry previous = _model->getMomentById(entry.momentId);
    if (_pending_published_moment_id > 0 && entry.momentId == _pending_published_moment_id) {
        _model->prependOrUpdateMoment(entry);
        _pending_published_moment_id = 0;
    } else {
        _model->upsertMoment(entry);
    }
    const int authoritativeCommentCount = _authoritative_comment_counts.value(entry.momentId, -1);
    if (authoritativeCommentCount >= 0
        && previous.comments.size() >= authoritativeCommentCount
        && previous.comments.size() > entry.comments.size()) {
        _model->updateDetail(entry.momentId, entry.likes, previous.comments);
    } else if (!entry.comments.isEmpty()) {
        _model->updateDetail(entry.momentId, entry.likes, entry.comments);
    } else {
        _model->updateDetail(entry.momentId, entry.likes, previous.comments);
    }
    emit momentRefreshed(entry.momentId);
}

void MomentsController::onCommentListRsp(ReqId id, const QString& res, ErrorCodes err) {
    if (id != ReqId::ID_MOMENTS_COMMENT_LIST) return;

    const qint64 pendingMomentId = _pending_comment_moments.take(ReqId::ID_MOMENTS_COMMENT_LIST);
    auto finishLoading = [this](qint64 momentId) {
        if (momentId > 0 && _comments_loading.remove(momentId)) {
            emit commentsLoadingChanged(momentId, false);
        }
    };

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseErr);
    if (err != ErrorCodes::SUCCESS || parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        finishLoading(pendingMomentId);
        emit commentAdded(pendingMomentId);
        return;
    }

    const QJsonObject root = doc.object();
    const int errorCode = root["error"].toInt();
    qint64 momentId = root["moment_id"].toVariant().toLongLong();
    if (momentId <= 0) momentId = pendingMomentId;
    if (errorCode != 0 || momentId <= 0) {
        finishLoading(momentId > 0 ? momentId : pendingMomentId);
        emit commentAdded(momentId);
        return;
    }

    QVector<MomentComment> comments;
    const QJsonArray commentsArr = root["comments"].toArray();
    comments.reserve(commentsArr.size());
    for (const auto& cmVar : commentsArr) {
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
        comments.append(cm);
    }

    const MomentEntry current = _model->getMomentById(momentId);
    _model->updateDetail(momentId, current.likes, comments);
    if (root.contains(QStringLiteral("comment_count"))) {
        const int commentCount = qMax(0, root.value(QStringLiteral("comment_count")).toInt());
        _authoritative_comment_counts[momentId] = commentCount;
        _model->setCommentCount(momentId, commentCount);
    }
    finishLoading(momentId);
    emit momentRefreshed(momentId);
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
