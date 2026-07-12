#include "R18Controller.h"

#include "R18ControllerPrivate.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
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
