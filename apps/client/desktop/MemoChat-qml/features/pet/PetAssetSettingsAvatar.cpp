#include "PetAssetSettings.h"
#include "PetAssetSettingsPrivate.h"

#include "Live2DAvatarOpenGLRenderer.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QRect>
#include <QStandardPaths>
#include <QUrl>
#include <QVector>
#include <QtGlobal>

#include <algorithm>

using namespace memochat::pet_asset_settings;

namespace {
bool isQrcPath(const QString &path)
{
    return path.startsWith(QStringLiteral(":/")) || path.startsWith(QStringLiteral("qrc:/"));
}

QString qrcFilePath(const QString &path)
{
    if (path.startsWith(QStringLiteral("qrc:/"))) {
        return QStringLiteral(":") + QUrl(path).path();
    }
    return path;
}

QString resolveAvatarInputPath(const QString &value, const QString &baseDirectory = QString())
{
    const QString cleaned = value.trimmed();
    if (cleaned.isEmpty()) {
        return {};
    }

    const QUrl url(cleaned);
    if (url.isLocalFile()) {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (isQrcPath(cleaned)) {
        return cleaned;
    }
    if (QDir::isAbsolutePath(cleaned)) {
        return QDir::cleanPath(cleaned);
    }

    QStringList candidates;
    if (!baseDirectory.isEmpty()) {
        candidates.append(QDir(baseDirectory).absoluteFilePath(cleaned));
    }
    candidates.append(clientSourcePath(cleaned));
    candidates.append(QDir::current().absoluteFilePath(cleaned));

    for (const QString &candidate : candidates) {
        const QString path = QDir::cleanPath(candidate);
        if (isQrcPath(path) || QFileInfo::exists(path)) {
            return path;
        }
    }
    return QDir::cleanPath(candidates.value(0, cleaned));
}

QString resolveAvatarModelReference(const QString &modelDirectory, const QString &reference)
{
    const QString cleaned = reference.trimmed();
    if (cleaned.isEmpty()) {
        return {};
    }

    const QUrl url(cleaned);
    if (url.isLocalFile()) {
        return QDir::cleanPath(url.toLocalFile());
    }
    if (isQrcPath(cleaned) || QDir::isAbsolutePath(cleaned)) {
        return QDir::cleanPath(cleaned);
    }
    if (isQrcPath(modelDirectory)) {
        return QDir::cleanPath(modelDirectory + QLatin1Char('/') + cleaned);
    }
    return QDir(modelDirectory).absoluteFilePath(cleaned);
}

QString avatarCacheDirectory()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (root.isEmpty()) {
        root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    if (root.isEmpty()) {
        root = QDir::home().absoluteFilePath(QStringLiteral(".memochat/cache"));
    }
    return QDir(root).absoluteFilePath(QStringLiteral("pet/live2d-avatars"));
}

void addPathFingerprint(QCryptographicHash &hash, const QString &path)
{
    const QByteArray separator(1, '\0');
    hash.addData(path.toUtf8());
    hash.addData(separator);
    if (path.isEmpty() || isQrcPath(path)) {
        return;
    }
    const QFileInfo info(path);
    if (!info.exists()) {
        return;
    }
    hash.addData(QByteArray::number(info.size()));
    hash.addData(separator);
    hash.addData(QByteArray::number(info.lastModified().toMSecsSinceEpoch()));
    hash.addData(separator);
}

QStringList modelTexturePaths(const QString &modelJsonPath, QString *modelDirectory)
{
    QFile modelFile(qrcFilePath(modelJsonPath));
    if (!modelFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QJsonDocument document = QJsonDocument::fromJson(modelFile.readAll());
    if (!document.isObject()) {
        return {};
    }

    const QFileInfo modelInfo(modelJsonPath);
    const QString directory = isQrcPath(modelJsonPath)
                                  ? QString(modelJsonPath).left(modelJsonPath.lastIndexOf(QLatin1Char('/')))
                                  : modelInfo.absolutePath();
    if (modelDirectory) {
        *modelDirectory = directory;
    }

    QStringList paths;
    const QJsonArray textures = document.object()
                                    .value(QStringLiteral("FileReferences"))
                                    .toObject()
                                    .value(QStringLiteral("Textures"))
                                    .toArray();
    for (const QJsonValue &texture : textures) {
        const QString texturePath = texture.toString().trimmed();
        if (!texturePath.isEmpty()) {
            paths.append(resolveAvatarModelReference(directory, texturePath));
        }
    }
    return paths;
}

int avatarCandidateScore(const QFileInfo &info)
{
    const QString name = info.completeBaseName().toLower();
    int score = 0;
    const QStringList strongTokens = {
        QStringLiteral("avatar"),
        QStringLiteral("icon"),
        QStringLiteral("preview"),
        QStringLiteral("profile"),
        QStringLiteral("head"),
        QStringLiteral("face"),
        QStringLiteral("头像"),
        QStringLiteral("头"),
        QStringLiteral("脸"),
        QStringLiteral("预览"),
        QStringLiteral("立绘"),
    };
    for (const QString &token : strongTokens) {
        if (name.contains(token)) {
            score += 80;
        }
    }
    if (name.contains(QStringLiteral("texture"))) {
        score -= 30;
    }
    if (name.contains(QStringLiteral("normal")) || name.contains(QStringLiteral("mask"))) {
        score -= 20;
    }
    return score;
}

QStringList packageImageCandidates(const QString &modelDirectory, const QStringList &texturePaths)
{
    QStringList candidates;
    if (!modelDirectory.isEmpty() && !isQrcPath(modelDirectory)) {
        struct ScoredPath {
            int score = 0;
            QString path;
        };
        QVector<ScoredPath> scored;
        QDirIterator it(modelDirectory,
                        {QStringLiteral("*.png"),
                         QStringLiteral("*.jpg"),
                         QStringLiteral("*.jpeg"),
                         QStringLiteral("*.webp")},
                        QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext() && scored.size() < 256) {
            it.next();
            const QFileInfo info = it.fileInfo();
            scored.push_back({avatarCandidateScore(info), info.absoluteFilePath()});
        }
        std::stable_sort(scored.begin(), scored.end(), [](const ScoredPath &left, const ScoredPath &right) {
            return left.score > right.score;
        });
        for (const ScoredPath &item : scored) {
            if (item.score > 0 && !candidates.contains(item.path)) {
                candidates.append(item.path);
            }
        }
    }

    for (const QString &texture : texturePaths) {
        if (!candidates.contains(texture)) {
            candidates.append(texture);
        }
    }
    return candidates;
}

QImage readAvatarImage(const QString &path)
{
    QImageReader reader(qrcFilePath(path));
    reader.setAutoTransform(true);
    return reader.read();
}

QRect alphaBounds(const QImage &image, int alphaThreshold = 12)
{
    if (image.isNull()) {
        return {};
    }
    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    int left = argb.width();
    int top = argb.height();
    int right = -1;
    int bottom = -1;
    for (int y = 0; y < argb.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(argb.constScanLine(y));
        for (int x = 0; x < argb.width(); ++x) {
            if (qAlpha(line[x]) <= alphaThreshold) {
                continue;
            }
            left = qMin(left, x);
            top = qMin(top, y);
            right = qMax(right, x);
            bottom = qMax(bottom, y);
        }
    }
    if (right < left || bottom < top) {
        return {};
    }
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

QRect clampedSquare(const QRect &source, const QRect &imageRect)
{
    if (source.isEmpty() || imageRect.isEmpty()) {
        return {};
    }

    const int side = qMin(qMax(source.width(), source.height()), qMin(imageRect.width(), imageRect.height()));
    int x = source.center().x() - side / 2;
    int y = source.center().y() - side / 2;
    x = qBound(imageRect.left(), x, imageRect.right() - side + 1);
    y = qBound(imageRect.top(), y, imageRect.bottom() - side + 1);
    return QRect(x, y, side, side).intersected(imageRect);
}

QImage squareAvatarImage(const QImage &source, const QRect &crop)
{
    if (source.isNull() || crop.isEmpty()) {
        return {};
    }

    QImage avatar(256, 256, QImage::Format_ARGB32_Premultiplied);
    avatar.fill(Qt::transparent);
    QPainter painter(&avatar);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(QRectF(0, 0, avatar.width(), avatar.height()), source, crop);
    return avatar;
}

QImage cropHeadAvatar(const QImage &source)
{
    if (source.isNull()) {
        return {};
    }

    const QRect bounds = alphaBounds(source);
    if (bounds.isEmpty()) {
        return {};
    }

    const int maxSide = qMax(bounds.width(), bounds.height());
    const int side = qMin(maxSide,
                          qMax(24, qRound(qMax(bounds.width() * 0.62, bounds.height() * 0.30))));
    const QPoint center(bounds.center().x(), bounds.top() + qRound(bounds.height() * 0.20));
    QRect crop(center.x() - side / 2, center.y() - side / 2, side, side);
    const int pad = qMax(4, qRound(side * 0.08));
    crop = crop.adjusted(-pad, -pad, pad, pad);
    return squareAvatarImage(source, clampedSquare(crop, source.rect()));
}

QString saveAvatarToCache(const QImage &avatar, const QString &cachePath)
{
    if (avatar.isNull()) {
        return {};
    }
    QDir dir(QFileInfo(cachePath).absolutePath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return {};
    }
    if (!avatar.save(cachePath, "PNG")) {
        return {};
    }
    return QUrl::fromLocalFile(cachePath).toString();
}

QImage renderedLive2DAvatar(const QString &modelJsonPath)
{
    QString error;
    const QImage avatar = Live2DAvatarOpenGLRenderer().renderAvatar(modelJsonPath, &error);
    if (avatar.isNull() && !error.isEmpty()) {
        qWarning().noquote() << "Live2D avatar OpenGL render unavailable:" << error;
    }
    return avatar;
}
} // namespace

QString PetAssetSettings::live2dAvatarUrl() const
{
    return resolveLive2DAvatarUrl(_model_json, _model_root);
}

QString PetAssetSettings::resolveLive2DAvatarUrl(const QString &modelJson, const QString &modelRoot) const
{
    const QString resolvedRoot = resolveAvatarInputPath(modelRoot);
    const QString resolvedModelJson = resolveAvatarInputPath(modelJson, resolvedRoot);
    if (resolvedModelJson.isEmpty()) {
        return {};
    }

    QString modelDirectory;
    const QStringList textures = modelTexturePaths(resolvedModelJson, &modelDirectory);
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QByteArrayLiteral("memochat-live2d-avatar-opengl-v3"));
    addPathFingerprint(hash, resolvedModelJson);
    for (const QString &texture : textures) {
        addPathFingerprint(hash, texture);
    }

    const QString cacheKey = QString::fromLatin1(hash.result().toHex());
    const QDir cacheDir(avatarCacheDirectory());
    const QString renderCachePath = cacheDir.absoluteFilePath(QStringLiteral("avatar_gl_%1.png").arg(cacheKey));
    if (QFileInfo::exists(renderCachePath)) {
        return QUrl::fromLocalFile(renderCachePath).toString();
    }

    QImage avatar = renderedLive2DAvatar(resolvedModelJson);
    if (!avatar.isNull()) {
        return saveAvatarToCache(avatar, renderCachePath);
    }

    const QString fallbackCachePath = cacheDir.absoluteFilePath(QStringLiteral("avatar_%1.png").arg(cacheKey));
    if (QFileInfo::exists(fallbackCachePath)) {
        return QUrl::fromLocalFile(fallbackCachePath).toString();
    }

    if (avatar.isNull()) {
        const QStringList candidates = packageImageCandidates(modelDirectory, textures);
        for (const QString &candidate : candidates) {
            avatar = cropHeadAvatar(readAvatarImage(candidate));
            if (!avatar.isNull()) {
                break;
            }
        }
    }

    return saveAvatarToCache(avatar, fallbackCachePath);
}
