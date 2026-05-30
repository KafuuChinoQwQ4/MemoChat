#include "PetAvatarPathUtils.h"

#include "PetAssetSettingsPrivate.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUrl>

namespace memochat::pet_asset_settings::avatar_path_utils
{

bool isQrcPath(const QString& path)
{
    return path.startsWith(QStringLiteral(":/")) || path.startsWith(QStringLiteral("qrc:/"));
}

QString qrcFilePath(const QString& path)
{
    if (path.startsWith(QStringLiteral("qrc:/")))
    {
        return QStringLiteral(":") + QUrl(path).path();
    }
    return path;
}

QString resolveAvatarInputPath(const QString& value, const QString& baseDirectory)
{
    const QString cleaned = value.trimmed();
    if (cleaned.isEmpty())
    {
        return {};
    }

    const QUrl url(cleaned);
    if (url.isLocalFile())
    {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (isQrcPath(cleaned))
    {
        return cleaned;
    }
    if (QDir::isAbsolutePath(cleaned))
    {
        return QDir::cleanPath(cleaned);
    }

    QStringList candidates;
    if (!baseDirectory.isEmpty())
    {
        candidates.append(QDir(baseDirectory).absoluteFilePath(cleaned));
    }
    candidates.append(clientSourcePath(cleaned));
    candidates.append(QDir::current().absoluteFilePath(cleaned));

    for (const QString& candidate : candidates)
    {
        const QString path = QDir::cleanPath(candidate);
        if (isQrcPath(path) || QFileInfo::exists(path))
        {
            return path;
        }
    }
    return QDir::cleanPath(candidates.value(0, cleaned));
}

QString resolveAvatarModelReference(const QString& modelDirectory, const QString& reference)
{
    const QString cleaned = reference.trimmed();
    if (cleaned.isEmpty())
    {
        return {};
    }

    const QUrl url(cleaned);
    if (url.isLocalFile())
    {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (isQrcPath(cleaned) || QDir::isAbsolutePath(cleaned))
    {
        return QDir::cleanPath(cleaned);
    }
    if (isQrcPath(modelDirectory))
    {
        return QDir::cleanPath(modelDirectory + QLatin1Char('/') + cleaned);
    }
    return QDir(modelDirectory).absoluteFilePath(cleaned);
}

QString avatarCacheDirectory()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (root.isEmpty())
    {
        root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    if (root.isEmpty())
    {
        root = QDir::home().absoluteFilePath(QStringLiteral(".memochat/cache"));
    }
    return QDir(root).absoluteFilePath(QStringLiteral("pet/live2d-avatars"));
}

void addPathFingerprint(QCryptographicHash& hash, const QString& path)
{
    const QByteArray separator(1, '\0');
    hash.addData(path.toUtf8());
    hash.addData(separator);
    if (path.isEmpty() || isQrcPath(path))
    {
        return;
    }
    const QFileInfo info(path);
    if (!info.exists())
    {
        return;
    }
    hash.addData(QByteArray::number(info.size()));
    hash.addData(separator);
    hash.addData(QByteArray::number(info.lastModified().toMSecsSinceEpoch()));
    hash.addData(separator);
}

QStringList modelTexturePaths(const QString& modelJsonPath, QString* modelDirectory)
{
    QFile modelFile(qrcFilePath(modelJsonPath));
    if (!modelFile.open(QIODevice::ReadOnly))
    {
        return {};
    }

    const QJsonDocument document = QJsonDocument::fromJson(modelFile.readAll());
    if (!document.isObject())
    {
        return {};
    }

    const QFileInfo modelInfo(modelJsonPath);
    const QString directory = isQrcPath(modelJsonPath)
                                  ? QString(modelJsonPath).left(modelJsonPath.lastIndexOf(QLatin1Char('/')))
                                  : modelInfo.absolutePath();
    if (modelDirectory)
    {
        *modelDirectory = directory;
    }

    QStringList paths;
    const QJsonArray textures =
        document.object()
            .value(QStringLiteral("FileReferences")).toObject().value(QStringLiteral("Textures")).toArray();
    for (const QJsonValue& texture : textures)
    {
        const QString texturePath = texture.toString().trimmed();
        if (!texturePath.isEmpty())
        {
            paths.append(resolveAvatarModelReference(directory, texturePath));
        }
    }
    return paths;
}

} // namespace memochat::pet_asset_settings::avatar_path_utils
