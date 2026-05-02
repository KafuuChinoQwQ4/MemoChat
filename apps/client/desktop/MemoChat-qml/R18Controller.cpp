#include "R18Controller.h"

#include "ClientGateway.h"
#include "global.h"
#include "usermgr.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
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

void R18Controller::refreshHistory()
{
    auto payload = authPayload();
    QUrl url(gate_url_prefix + QStringLiteral("/api/r18/history"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uid"), QString::number(payload.value(QStringLiteral("uid")).toInt()));
    query.addQueryItem(QStringLiteral("token"), payload.value(QStringLiteral("token")).toString());
    url.setQuery(query);
    getJson(url, QStringLiteral("history"));
}

void R18Controller::refreshOfficialSources(const QString& catalogUrl)
{
    if (!catalogUrl.trimmed().isEmpty()) {
        setOfficialSourceCatalogUrl(catalogUrl.trimmed());
    }
    QUrl url(_official_source_catalog_url);
    if (!url.isValid() || url.scheme().isEmpty()) {
        setError(QStringLiteral("官方源目录 URL 无效"));
        return;
    }
    getJson(url, QStringLiteral("official_sources"));
}

void R18Controller::importOfficialSource(int row)
{
    const auto item = _official_sources.get(row);
    if (item.isEmpty()) {
        setError(QStringLiteral("官方源条目无效"));
        return;
    }
    const QUrl scriptUrl = resolveOfficialSourceUrl(item);
    if (!scriptUrl.isValid() || scriptUrl.scheme().isEmpty()) {
        setError(QStringLiteral("官方源脚本 URL 无效"));
        return;
    }

    downloadAndImportSource(scriptUrl, item);
}

void R18Controller::importSourceUrl(const QString& sourceUrl)
{
    QUrl scriptUrl(sourceUrl.trimmed());
    if (!scriptUrl.isValid() || scriptUrl.scheme().isEmpty()) {
        setError(QStringLiteral("源脚本 URL 无效"));
        return;
    }
    downloadAndImportSource(scriptUrl, {});
}

void R18Controller::selectSource(const QString& sourceId)
{
    const QString nextSource = sourceId.isEmpty() ? QStringLiteral("mock") : sourceId;
    if (_current_source_id == nextSource) {
        return;
    }
    _current_source_id = nextSource;
    emit currentSourceChanged();
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
    setCurrentFavorite(false);

    auto payload = authPayload();
    payload[QStringLiteral("source_id")] = _current_source_id;
    payload[QStringLiteral("comic_id")] = comicId;
    postJson(QStringLiteral("/api/r18/comic/detail"), payload, QStringLiteral("detail"));
}

void R18Controller::openChapter(const QString& sourceId, const QString& chapterId)
{
    _current_chapter_id = chapterId;
    setCurrentPageIndex(1);
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

void R18Controller::toggleFavorite(const QString& sourceId, const QString& comicId, bool favorited)
{
    auto payload = authPayload();
    payload[QStringLiteral("source_id")] = sourceId.isEmpty() ? _current_source_id : sourceId;
    payload[QStringLiteral("comic_id")] = comicId.isEmpty() ? _current_comic.value(QStringLiteral("comic_id")).toString() : comicId;
    payload[QStringLiteral("favorited")] = favorited;
    postJson(QStringLiteral("/api/r18/favorite/toggle"), payload, QStringLiteral("favorite"));
}

void R18Controller::updateHistory(const QString& sourceId, const QString& comicId, const QString& chapterId, int pageIndex)
{
    setCurrentPageIndex(pageIndex < 1 ? 1 : pageIndex);
    auto payload = authPayload();
    payload[QStringLiteral("source_id")] = sourceId.isEmpty() ? _current_source_id : sourceId;
    payload[QStringLiteral("comic_id")] = comicId.isEmpty() ? _current_comic.value(QStringLiteral("comic_id")).toString() : comicId;
    payload[QStringLiteral("chapter_id")] = chapterId.isEmpty() ? _current_chapter_id : chapterId;
    payload[QStringLiteral("page_index")] = _current_page_index;
    postJson(QStringLiteral("/api/r18/history/update"), payload, QStringLiteral("history_update"));
}

void R18Controller::postJson(const QString& path, const QJsonObject& payload, const QString& op)
{
    const bool quiet = op == QStringLiteral("history_update");
    if (!quiet) {
        setLoading(true);
        setError({});
    }
    QNetworkRequest request(QUrl(gate_url_prefix + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    auto* reply = _network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    reply->setProperty("r18_op", op);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QString op = reply->property("r18_op").toString();
        const bool quiet = op == QStringLiteral("history_update");
        const auto bytes = reply->readAll();
        reply->deleteLater();
        if (!quiet) {
            setLoading(false);
        }
        const auto doc = QJsonDocument::fromJson(bytes);
        if (!doc.isObject()) {
            if (!quiet) {
                setError(QStringLiteral("R18 响应格式错误"));
            }
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
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        const QString op = reply->property("r18_op").toString();
        const auto bytes = reply->readAll();
        reply->deleteLater();
        setLoading(false);
        const auto doc = QJsonDocument::fromJson(bytes);
        if (op == QStringLiteral("official_sources") && doc.isArray()) {
            QUrl catalogUrl = url;
            auto items = doc.array().toVariantList();
            for (auto& entry : items) {
                auto map = entry.toMap();
                map[QStringLiteral("catalog_url")] = catalogUrl.toString();
                const QUrl scriptUrl = resolveOfficialSourceUrl(map);
                map[QStringLiteral("source_url")] = scriptUrl.toString();
                entry = map;
            }
            _official_sources.setItems(items);
            return;
        }
        if (!doc.isObject()) {
            setError(QStringLiteral("R18 响应格式错误"));
            return;
        }
        handleResponse(op, doc.object());
    });
}

void R18Controller::setOfficialSourceCatalogUrl(const QString& url)
{
    if (_official_source_catalog_url == url) return;
    _official_source_catalog_url = url;
    emit officialSourceCatalogUrlChanged();
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

QJsonObject R18Controller::officialSourceManifest(const QVariantMap& item, const QUrl& scriptUrl) const
{
    QJsonObject manifest;
    manifest[QStringLiteral("id")] = item.value(QStringLiteral("key"), item.value(QStringLiteral("id"))).toString();
    manifest[QStringLiteral("name")] = item.value(QStringLiteral("name"), manifest.value(QStringLiteral("id")).toString()).toString();
    manifest[QStringLiteral("version")] = item.value(QStringLiteral("version"), QStringLiteral("0.0.0")).toString();
    manifest[QStringLiteral("description")] = item.value(QStringLiteral("description")).toString();
    manifest[QStringLiteral("format")] = QStringLiteral("venera-js");
    manifest[QStringLiteral("source_url")] = scriptUrl.toString();
    manifest[QStringLiteral("catalog_url")] = _official_source_catalog_url;
    return manifest;
}

QJsonObject R18Controller::sourceUrlManifest(const QUrl& scriptUrl) const
{
    const QString fileName = QFileInfo(scriptUrl.path()).fileName();
    QString id = fileName;
    if (id.endsWith(QStringLiteral(".js"), Qt::CaseInsensitive)) {
        id.chop(3);
    }
    if (id.isEmpty()) {
        id = QStringLiteral("venera-source");
    }

    QJsonObject manifest;
    manifest[QStringLiteral("id")] = id;
    manifest[QStringLiteral("name")] = id;
    manifest[QStringLiteral("version")] = QStringLiteral("0.0.0");
    manifest[QStringLiteral("format")] = QStringLiteral("venera-js");
    manifest[QStringLiteral("source_url")] = scriptUrl.toString();
    manifest[QStringLiteral("catalog_url")] = QString();
    return manifest;
}

QUrl R18Controller::resolveOfficialSourceUrl(const QVariantMap& item) const
{
    const QString explicitUrl = item.value(QStringLiteral("url")).toString();
    if (!explicitUrl.isEmpty()) {
        return QUrl(explicitUrl);
    }

    const QString fileName = item.value(QStringLiteral("fileName"), item.value(QStringLiteral("filename"))).toString();
    if (fileName.isEmpty()) {
        return {};
    }

    QUrl base(item.value(QStringLiteral("catalog_url"), _official_source_catalog_url).toString());
    if (!base.isValid()) {
        return {};
    }
    return base.resolved(QUrl(fileName));
}

void R18Controller::downloadAndImportSource(const QUrl& scriptUrl, const QVariantMap& item)
{
    setLoading(true);
    setError({});
    QNetworkRequest request(scriptUrl);
    auto* reply = _network.get(request);
    reply->setProperty("r18_source_url", scriptUrl);
    connect(reply, &QNetworkReply::finished, this, [this, reply, item]() {
        const auto bytes = reply->readAll();
        const QUrl scriptUrl = reply->property("r18_source_url").toUrl();
        reply->deleteLater();
        setLoading(false);
        if (bytes.isEmpty()) {
            setError(QStringLiteral("源脚本下载失败"));
            return;
        }

        const QJsonObject manifest = item.isEmpty()
            ? sourceUrlManifest(scriptUrl)
            : officialSourceManifest(item, scriptUrl);
        auto payload = authPayload();
        QString fileName = item.value(QStringLiteral("fileName"), item.value(QStringLiteral("filename"), item.value(QStringLiteral("key")))).toString();
        if (fileName.isEmpty()) {
            fileName = QFileInfo(scriptUrl.path()).fileName();
        }
        if (fileName.isEmpty()) {
            fileName = QStringLiteral("venera-source.js");
        }
        payload[QStringLiteral("file_name")] = fileName.endsWith(QStringLiteral(".js"), Qt::CaseInsensitive) ? fileName : fileName + QStringLiteral(".js");
        payload[QStringLiteral("data_base64")] = QString::fromLatin1(bytes.toBase64());
        payload[QStringLiteral("manifest_json")] = QString::fromUtf8(QJsonDocument(manifest).toJson(QJsonDocument::Compact));
        postJson(QStringLiteral("/api/r18/source/import"), payload, QStringLiteral("import"));
    });
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

void R18Controller::setCurrentFavorite(bool favorite)
{
    if (_current_favorite == favorite) return;
    _current_favorite = favorite;
    emit currentFavoriteChanged();
}

void R18Controller::setCurrentPageIndex(int pageIndex)
{
    const int normalized = pageIndex < 1 ? 1 : pageIndex;
    if (_current_page_index == normalized) return;
    _current_page_index = normalized;
    emit currentPageChanged();
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
    } else if (op == QStringLiteral("history")) {
        _history.setItems(data.value(QStringLiteral("items")).toArray().toVariantList());
    } else if (op == QStringLiteral("detail")) {
        _current_comic = data.toVariantMap();
        emit currentComicChanged();
        _chapters.setItems(data.value(QStringLiteral("chapters")).toArray().toVariantList());
    } else if (op == QStringLiteral("pages")) {
        _pages.setItems(data.value(QStringLiteral("pages")).toArray().toVariantList());
        updateHistory(_current_source_id,
                      _current_comic.value(QStringLiteral("comic_id")).toString(),
                      _current_chapter_id,
                      _current_page_index);
    } else if (op == QStringLiteral("favorite")) {
        setCurrentFavorite(data.value(QStringLiteral("favorited")).toBool());
    } else if (op == QStringLiteral("source_state") || op == QStringLiteral("import")) {
        refreshSources();
    }
}
