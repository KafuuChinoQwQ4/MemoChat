#include "Live2DModelAssetParser.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrl>
#include <algorithm>

namespace
{

QString normalizedChecksumPath(const QString& modelDirectory, const QString& path)
{
    if (path.isEmpty())
    {
        return {};
    }
    const QString cleanedPath = QDir::cleanPath(path);
    if (!Live2DModelAssetParser::isQrcPath(cleanedPath) && !modelDirectory.isEmpty() &&
        !Live2DModelAssetParser::isQrcPath(modelDirectory))
    {
        const QDir dir(modelDirectory);
        const QString relative = dir.relativeFilePath(cleanedPath);
        if (!relative.startsWith(QStringLiteral("..")))
        {
            return QDir::fromNativeSeparators(relative);
        }
    }
    return QDir::fromNativeSeparators(cleanedPath);
}

} // namespace

namespace Live2DModelAssetParser
{

bool isQrcPath(const QString& path)
{
    return path.startsWith(QStringLiteral(":/")) || path.startsWith(QStringLiteral("qrc:/"));
}

bool fileExists(const QString& path)
{
    if (path.isEmpty())
    {
        return false;
    }
    if (isQrcPath(path))
    {
        QFile file(path.startsWith(QStringLiteral("qrc:/")) ? QStringLiteral(":") + QUrl(path).path() : path);
        return file.exists();
    }
    return QFileInfo::exists(path);
}

QString qrcFilePath(const QString& path)
{
    if (path.startsWith(QStringLiteral("qrc:/")))
    {
        return QStringLiteral(":") + QUrl(path).path();
    }
    return path;
}

void appendExistingPackageFile(QStringList& files, const QString& path)
{
    if (path.isEmpty() || !fileExists(path) || QFileInfo(path).isDir())
    {
        return;
    }
    const QString cleaned = QDir::cleanPath(path);
    if (!files.contains(cleaned))
    {
        files.append(cleaned);
    }
}

void appendJsonReferenceWarning(QStringList& warnings,
                                const QString& label,
                                const QString& reference,
                                const QString& resolvedPath)
{
    if (reference.trimmed().isEmpty())
    {
        return;
    }
    if (!fileExists(resolvedPath) || QFileInfo(resolvedPath).isDir())
    {
        warnings.append(QStringLiteral("%1不存在：%2").arg(label, reference));
    }
}

bool appendJsonParseWarning(QStringList& warnings, const QString& label, const QString& path, QStringList* packageFiles)
{
    if (path.trimmed().isEmpty())
    {
        return false;
    }
    if (!fileExists(path) || QFileInfo(path).isDir())
    {
        warnings.append(QStringLiteral("%1不存在：%2").arg(label, path));
        return false;
    }
    QFile file(qrcFilePath(path));
    if (!file.open(QIODevice::ReadOnly))
    {
        warnings.append(QStringLiteral("%1无法读取：%2").arg(label, file.errorString()));
        return false;
    }
    QJsonParseError error;
    QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        warnings.append(QStringLiteral("%1格式无效：%2").arg(label, error.errorString()));
        return false;
    }
    if (packageFiles)
    {
        appendExistingPackageFile(*packageFiles, path);
    }
    return true;
}

QString findSiblingVtubeMapping(const QString& modelDirectory)
{
    if (modelDirectory.isEmpty() || isQrcPath(modelDirectory))
    {
        return {};
    }
    const QDir dir(modelDirectory);
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.vtube.json")}, QDir::Files | QDir::NoDotAndDotDot);
    if (files.isEmpty())
    {
        return {};
    }
    return files.first().absoluteFilePath();
}

QString computePackageChecksum(const QString& modelDirectory, const QStringList& packageFiles)
{
    QStringList files = packageFiles;
    files.removeDuplicates();
    std::sort(files.begin(),
              files.end(),
              [&modelDirectory](const QString& left, const QString& right)
              {
                  return normalizedChecksumPath(modelDirectory, left) < normalizedChecksumPath(modelDirectory, right);
              });

    QCryptographicHash hash(QCryptographicHash::Sha256);
    for (const QString& path : files)
    {
        QFile file(qrcFilePath(path));
        if (!file.open(QIODevice::ReadOnly))
        {
            continue;
        }
        const QByteArray relative = normalizedChecksumPath(modelDirectory, path).toUtf8();
        hash.addData(relative);
        hash.addData(QByteArrayView("\0", 1));
        hash.addData(file.readAll());
        hash.addData(QByteArrayView("\0", 1));
    }
    return QString::fromLatin1(hash.result().toHex());
}

void appendMissingDirectoryWarning(QStringList& warnings,
                                   const QString& label,
                                   const QString& input,
                                   const QString& resolvedPath)
{
    if (input.trimmed().isEmpty() || isQrcPath(resolvedPath))
    {
        return;
    }
    const QFileInfo info(resolvedPath);
    if (!info.exists() || !info.isDir())
    {
        warnings.append(QStringLiteral("%1不存在：%2").arg(label, input));
    }
}

} // namespace Live2DModelAssetParser
