#include "R18Controller.h"

#include "LocalFilePickerService.h"
#include "R18ControllerPrivate.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

using namespace memochat::r18;

void R18Controller::refreshOfficialSources(const QString& catalogUrl)
{
    const QString trimmedCatalogUrl = catalogUrl.trimmed();
    if (!trimmedCatalogUrl.isEmpty())
    {
        setOfficialSourceCatalogUrl(trimmedCatalogUrl);
    }
    if (_official_source_catalog_url.trimmed().isEmpty())
    {
        _official_sources.clear();
        setStatusText(QStringLiteral("未配置自定义源目录"));
        return;
    }
    const QFileInfo catalogInfo(_official_source_catalog_url);
    QUrl url = QUrl::fromUserInput(_official_source_catalog_url);
    if (catalogInfo.exists() || looksLikeWindowsDrivePath(_official_source_catalog_url))
    {
        const QString resolvedPath = catalogInfo.isDir() ? QDir(catalogInfo.absoluteFilePath())
                                                               .filePath(QStringLiteral("index.json"))
                                                         : catalogInfo.absoluteFilePath();
        url = QUrl::fromLocalFile(resolvedPath);
    }
    if (!url.isValid() || url.scheme().isEmpty())
    {
        setError(QStringLiteral("自定义源目录路径无效"));
        return;
    }
    setStatusText(QStringLiteral("正在读取自定义源目录"));
    getJson(url, QStringLiteral("official_sources"));
}

void R18Controller::importOfficialSource(int row)
{
    const auto item = _official_sources.get(row);
    if (item.isEmpty())
    {
        setError(QStringLiteral("官方源条目无效"));
        return;
    }
    const QUrl scriptUrl = resolveOfficialSourceUrl(item);
    if (!scriptUrl.isValid() || scriptUrl.scheme().isEmpty())
    {
        setError(QStringLiteral("官方源脚本 URL 无效"));
        return;
    }

    downloadAndImportSource(scriptUrl, item);
}

void R18Controller::importSourceUrl(const QString& sourceUrl)
{
    QUrl scriptUrl(sourceUrl.trimmed());
    if (!scriptUrl.isValid() || scriptUrl.scheme().isEmpty())
    {
        setError(QStringLiteral("源脚本 URL 无效"));
        return;
    }
    setStatusText(QStringLiteral("正在下载漫画源"));
    downloadAndImportSource(scriptUrl, {});
}

QString R18Controller::pickSourcePackage()
{
    QString fileUrl;
    QString displayName;
    QString errorText;
    if (!LocalFilePickerService::pickFileUrl(&fileUrl, &displayName, &errorText))
    {
        if (!errorText.isEmpty())
        {
            setError(errorText);
        }
        return {};
    }
    const QUrl url(fileUrl);
    if (url.isLocalFile())
    {
        return url.toLocalFile();
    }
    return fileUrl;
}

QString R18Controller::pickSourceCatalogPath()
{
    const QString directory = QFileDialog::getExistingDirectory(nullptr, QStringLiteral("选择漫画源目录"), QString());
    if (!directory.isEmpty())
    {
        return directory;
    }

    const QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        QStringLiteral("选择漫画源 index.json"), QString(), QStringLiteral("漫画源目录 (*.json);;所有文件 (*.*)"));
    return fileName;
}

void R18Controller::importSourcePackage(const QString& filePath, const QString& manifestJson)
{
    const QUrl fileUrl(filePath);
    const QString localPath = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : filePath;
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        setError(QStringLiteral("无法打开源包"));
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("file_name")] = QFileInfo(localPath).fileName();
    payload[QStringLiteral("data_base64")] = QString::fromLatin1(file.readAll().toBase64());
    payload[QStringLiteral("manifest_json")] = manifestJson;
    setStatusText(QStringLiteral("正在导入源包"));
    postJson(QStringLiteral("/api/r18/source/import"), payload, QStringLiteral("import"));
}

QJsonObject R18Controller::officialSourceManifest(const QVariantMap& item, const QUrl& scriptUrl) const
{
    QJsonObject manifest;
    manifest[QStringLiteral("id")] = item.value(QStringLiteral("key"), item.value(QStringLiteral("id"))).toString();
    manifest[QStringLiteral("name")] =
                 item.value(QStringLiteral("name"), manifest.value(QStringLiteral("id")).toString()).toString();
    manifest[QStringLiteral("version")] = item.value(QStringLiteral("version"), QStringLiteral("0.0.0")).toString();
    manifest[QStringLiteral("description")] = item.value(QStringLiteral("description")).toString();
    manifest[QStringLiteral("format")] = QStringLiteral("source-js");
    manifest[QStringLiteral("source_url")] = scriptUrl.toString();
    manifest[QStringLiteral("catalog_url")] = _official_source_catalog_url;
    return manifest;
}

QJsonObject R18Controller::sourceUrlManifest(const QUrl& scriptUrl) const
{
    const QString fileName = QFileInfo(scriptUrl.path()).fileName();
    QString id = fileName;
    if (id.endsWith(QStringLiteral(".js"), Qt::CaseInsensitive))
    {
        id.chop(3);
    }
    if (id.isEmpty())
    {
        id = QStringLiteral("custom-source");
    }

    QJsonObject manifest;
    manifest[QStringLiteral("id")] = id;
    manifest[QStringLiteral("name")] = id;
    manifest[QStringLiteral("version")] = QStringLiteral("0.0.0");
    manifest[QStringLiteral("format")] = QStringLiteral("source-js");
    manifest[QStringLiteral("source_url")] = scriptUrl.toString();
    manifest[QStringLiteral("catalog_url")] = QString();
    return manifest;
}

QUrl R18Controller::resolveOfficialSourceUrl(const QVariantMap& item) const
{
    const QString explicitUrl = item.value(QStringLiteral("url")).toString();
    if (!explicitUrl.isEmpty())
    {
        return QUrl(explicitUrl);
    }

    const QString fileName = item.value(QStringLiteral("fileName"), item.value(QStringLiteral("filename"))).toString();
    if (fileName.isEmpty())
    {
        return {};
    }

    QUrl base(item.value(QStringLiteral("catalog_url"), _official_source_catalog_url).toString());
    if (!base.isValid())
    {
        return {};
    }
    return base.resolved(QUrl(fileName));
}

void R18Controller::downloadAndImportSource(const QUrl& scriptUrl, const QVariantMap& item)
{
    setLoading(true);
    setError({});
    QNetworkRequest request(scriptUrl);
    applyRequestOptions(request);
    auto* reply = _network.get(request);
    armTimeout(reply);
    reply->setProperty("r18_source_url", scriptUrl);
    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply, item]()
        {
            const auto networkError = reply->error();
            const QString networkErrorText = reply->errorString();
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const auto bytes = reply->readAll();
            const QUrl scriptUrl = reply->property("r18_source_url").toUrl();
            reply->deleteLater();
            setLoading(false);
            if (networkError != QNetworkReply::NoError)
            {
                setError(QStringLiteral("源脚本下载失败: %1").arg(networkErrorText));
                return;
            }
            if (httpStatus >= 400)
            {
                setError(QStringLiteral("源脚本下载失败: HTTP %1").arg(httpStatus));
                return;
            }
            if (bytes.isEmpty())
            {
                setError(QStringLiteral("源脚本下载失败"));
                return;
            }

            const QJsonObject manifest =
                item.isEmpty() ? sourceUrlManifest(scriptUrl) : officialSourceManifest(item, scriptUrl);
            QJsonObject payload;
            QString fileName = item.value(
                QStringLiteral("fileName"),
                               item.value(QStringLiteral("filename"), item.value(QStringLiteral("key")))).toString();
            if (fileName.isEmpty())
            {
                fileName = QFileInfo(scriptUrl.path()).fileName();
            }
            if (fileName.isEmpty())
            {
                fileName = QStringLiteral("custom-source.js");
            }
            payload[QStringLiteral("file_name")] = fileName.endsWith(QStringLiteral(".js"), Qt::CaseInsensitive)
                                                                     ? fileName
                                                                     : fileName + QStringLiteral(".js");
            payload[QStringLiteral("data_base64")] = QString::fromLatin1(bytes.toBase64());
            payload[QStringLiteral("manifest_json")] =
                        QString::fromUtf8(QJsonDocument(manifest).toJson(QJsonDocument::Compact));
            setStatusText(
                QStringLiteral("正在导入漫画源: %1").arg(manifest.value(QStringLiteral("name")).toString(fileName)));
            postJson(QStringLiteral("/api/r18/source/import"), payload, QStringLiteral("import"));
        });
}
