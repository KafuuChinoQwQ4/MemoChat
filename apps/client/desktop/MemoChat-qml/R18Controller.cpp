#include "R18Controller.h"

#include "ClientGateway.h"
#include "global.h"
#include "usermgr.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

R18Controller::R18Controller(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
{
}

void R18Controller::refreshSources()
{
    auto payload = authPayload();
    QUrl url(gate_url_prefix + QStringLiteral("/api/r18/sources"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(payload.value(QStringLiteral("uid")).toInt()));
    query.addQueryItem(QStringLiteral("token"), payload.value(QStringLiteral("token")).toString());
    url.setQuery(query);
    getJson(url, QStringLiteral("sources"));
}

void R18Controller::search(const QString& keyword, int page)
{
    auto payload = authPayload();
    payload[QStringLiteral("source_id")] = _current_source_id;
    payload[QStringLiteral("keyword")] = keyword;
    payload[QStringLiteral("page")] = page < 1 ? 1 : page;
    postJson(QStringLiteral("/api/r18/search"), payload, QStringLiteral("search"));
}

void R18Controller::openComic(const QString& sourceId, const QString& comicId)
{
    _current_source_id = sourceId.isEmpty() ? QStringLiteral("mock") : sourceId;
    emit currentSourceChanged();

    auto payload = authPayload();
    payload[QStringLiteral("source_id")] = _current_source_id;
    payload[QStringLiteral("comic_id")] = comicId;
    postJson(QStringLiteral("/api/r18/comic/detail"), payload, QStringLiteral("detail"));
}

void R18Controller::openChapter(const QString& sourceId, const QString& chapterId)
{
    auto payload = authPayload();
    payload[QStringLiteral("source_id")] = sourceId.isEmpty() ? _current_source_id : sourceId;
    payload[QStringLiteral("chapter_id")] = chapterId;
    postJson(QStringLiteral("/api/r18/chapter/pages"), payload, QStringLiteral("pages"));
}

void R18Controller::enableSource(const QString& sourceId, bool enabled)
{
    auto payload = authPayload();
    payload[QStringLiteral("source_id")] = sourceId;
    postJson(enabled ? QStringLiteral("/api/r18/source/enable") : QStringLiteral("/api/r18/source/disable"),
             payload,
             QStringLiteral("source_state"));
}

void R18Controller::importSourcePackage(const QString& filePath, const QString& manifestJson)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("无法打开源包"));
        return;
    }

    auto payload = authPayload();
    payload[QStringLiteral("file_name")] = QFileInfo(filePath).fileName();
    payload[QStringLiteral("data_base64")] = QString::fromLatin1(file.readAll().toBase64());
    payload[QStringLiteral("manifest_json")] = manifestJson;
    postJson(QStringLiteral("/api/r18/source/import"), payload, QStringLiteral("import"));
}

void R18Controller::postJson(const QString& path, const QJsonObject& payload, const QString& op)
{
    setLoading(true);
    setError({});
    QNetworkRequest request(QUrl(gate_url_prefix + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto* reply = _network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    reply->setProperty("r18_op", op);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QString op = reply->property("r18_op").toString();
        const auto bytes = reply->readAll();
        reply->deleteLater();
        setLoading(false);
        const auto doc = QJsonDocument::fromJson(bytes);
        if (!doc.isObject()) {
            setError(QStringLiteral("R18 响应格式错误"));
            return;
        }
        handleResponse(op, doc.object());
    });
}

void R18Controller::getJson(const QUrl& url, const QString& op)
{
    setLoading(true);
    setError({});
    QNetworkRequest request(url);
    auto* reply = _network.get(request);
    reply->setProperty("r18_op", op);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QString op = reply->property("r18_op").toString();
        const auto bytes = reply->readAll();
        reply->deleteLater();
        setLoading(false);
        const auto doc = QJsonDocument::fromJson(bytes);
        if (!doc.isObject()) {
            setError(QStringLiteral("R18 响应格式错误"));
            return;
        }
        handleResponse(op, doc.object());
    });
}

QJsonObject R18Controller::authPayload() const
{
    QJsonObject payload;
    if (_gateway && _gateway->userMgr()) {
        payload[QStringLiteral("uid")] = _gateway->userMgr()->GetUid();
        payload[QStringLiteral("token")] = _gateway->userMgr()->GetToken();
    }
    return payload;
}

void R18Controller::setLoading(bool loading)
{
    if (_loading == loading) return;
    _loading = loading;
    emit loadingChanged();
}

void R18Controller::setError(const QString& error)
{
    if (_error == error) return;
    _error = error;
    emit errorChanged();
}

void R18Controller::handleResponse(const QString& op, const QJsonObject& root)
{
    if (root.value(QStringLiteral("error")).toInt() != 0) {
        setError(root.value(QStringLiteral("message")).toString(QStringLiteral("R18 请求失败")));
        return;
    }

    const auto data = root.value(QStringLiteral("data")).toObject();
    if (op == QStringLiteral("sources")) {
        _sources.setItems(data.value(QStringLiteral("sources")).toArray().toVariantList());
    } else if (op == QStringLiteral("search")) {
        _comics.setItems(data.value(QStringLiteral("items")).toArray().toVariantList());
    } else if (op == QStringLiteral("detail")) {
        _current_comic = data.toVariantMap();
        emit currentComicChanged();
        _chapters.setItems(data.value(QStringLiteral("chapters")).toArray().toVariantList());
    } else if (op == QStringLiteral("pages")) {
        _pages.setItems(data.value(QStringLiteral("pages")).toArray().toVariantList());
    } else if (op == QStringLiteral("source_state") || op == QStringLiteral("import")) {
        refreshSources();
    }
}
