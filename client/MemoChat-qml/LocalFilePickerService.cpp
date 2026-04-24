#include "LocalFilePickerService.h"
#include "imagecropperdialog.h"
#include "ScreenCaptureService.h"
#include "WebcamCaptureDialog.h"
#include <QDesktopServices>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QMimeDatabase>
#include <QImageWriter>
#include <QImageReader>
#include <QPixmap>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <QMutex>
#include <QMutexLocker>

namespace {
QString inferMomentAttachmentType(const QString &filename)
{
    const QMimeDatabase mimeDb;
    const QMimeType mimeType = mimeDb.mimeTypeForFile(filename, QMimeDatabase::MatchContent);
    if (mimeType.name().startsWith(QStringLiteral("image/"))) {
        return QStringLiteral("image");
    }
    if (mimeType.name().startsWith(QStringLiteral("video/"))) {
        return QStringLiteral("video");
    }

    const QString suffix = QFileInfo(filename).suffix().toLower();
    static const QSet<QString> imageSuffixes = {
        QStringLiteral("png"),
        QStringLiteral("jpg"),
        QStringLiteral("jpeg"),
        QStringLiteral("bmp"),
        QStringLiteral("webp"),
        QStringLiteral("gif")
    };
    static const QSet<QString> videoSuffixes = {
        QStringLiteral("mp4"),
        QStringLiteral("mov"),
        QStringLiteral("avi"),
        QStringLiteral("mkv"),
        QStringLiteral("webm"),
        QStringLiteral("m4v")
    };
    if (imageSuffixes.contains(suffix)) {
        return QStringLiteral("image");
    }
    if (videoSuffixes.contains(suffix)) {
        return QStringLiteral("video");
    }
    return {};
}

QVariantMap buildAttachmentMap(const QString &filename, const QString &type)
{
    QVariantMap attachment;
    const QFileInfo fileInfo(filename);
    attachment.insert(QStringLiteral("attachmentId"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    attachment.insert(QStringLiteral("type"), type);
    attachment.insert(QStringLiteral("fileUrl"), QUrl::fromLocalFile(filename).toString());
    attachment.insert(QStringLiteral("fileName"), fileInfo.fileName());
    attachment.insert(QStringLiteral("sizeBytes"), fileInfo.size());
    attachment.insert(QStringLiteral("selectedAt"), QDateTime::currentMSecsSinceEpoch());

    if (type == QStringLiteral("image")) {
        QImageReader reader(filename);
        const QSize imageSize = reader.size();
        attachment.insert(QStringLiteral("previewUrl"), QUrl::fromLocalFile(filename).toString());
        attachment.insert(QStringLiteral("width"), imageSize.width());
        attachment.insert(QStringLiteral("height"), imageSize.height());
        attachment.insert(QStringLiteral("mimeType"), reader.format().isEmpty()
                                                ? QString()
                                                : QStringLiteral("image/%1").arg(QString::fromLatin1(reader.format()).toLower()));
    } else if (type == QStringLiteral("video")) {
        const QMimeDatabase mimeDb;
        const QMimeType mimeType = mimeDb.mimeTypeForFile(fileInfo);
        attachment.insert(QStringLiteral("previewUrl"), QUrl::fromLocalFile(filename).toString());
        attachment.insert(QStringLiteral("width"), 0);
        attachment.insert(QStringLiteral("height"), 0);
        attachment.insert(QStringLiteral("durationMs"), 0);
        attachment.insert(QStringLiteral("mimeType"), mimeType.name());
    }

    return attachment;
}

QMutex g_avatar_mutex;

QString getAvatarDir()
{
    const QString storageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (storageDir.isEmpty()) {
        return {};
    }
    QDir dir(storageDir);
    if (!dir.exists("avatars") && !dir.mkpath("avatars")) {
        return {};
    }
    return dir.filePath("avatars");
}

void cleanupOldAvatars(const QString &keepUrl)
{
    QMutexLocker locker(&g_avatar_mutex);
    const QString avatarsDir = getAvatarDir();
    if (avatarsDir.isEmpty()) {
        return;
    }
    const QString keepName = QFileInfo(keepUrl).fileName();
    QDir dir(avatarsDir);
    for (const QString &file : dir.entryList({"head_*.jpg", "head_*.png"})) {
        if (file != keepName) {
            dir.remove(file);
        }
    }
}

QString storeAvatarImage(const QImage &image)
{
    const QString avatarsDir = getAvatarDir();
    if (avatarsDir.isEmpty()) {
        return {};
    }

    const QString uniqueName = QStringLiteral("head_%1.jpg")
                                   .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString avatarPath = avatarsDir + "/" + uniqueName;

    QImageWriter writer(avatarPath, "JPEG");
    writer.setQuality(90);
    if (!writer.write(image)) {
        return {};
    }

    const QString url = QUrl::fromLocalFile(avatarPath).toString();
    cleanupOldAvatars(url);
    return url;
}

}

bool LocalFilePickerService::pickImageUrl(QString *fileUrl, QString *errorText)
{
    const QString filename = QFileDialog::getOpenFileName(
        nullptr,
        "选择图片",
        QString(),
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (filename.isEmpty()) {
        return false;
    }

    QImageReader reader(filename);
    if (!reader.canRead()) {
        if (errorText) {
            *errorText = "图片读取失败";
        }
        return false;
    }

    if (fileUrl) {
        *fileUrl = QUrl::fromLocalFile(filename).toString();
    }
    return true;
}

bool LocalFilePickerService::pickImageUrls(QVariantList *attachments, QString *errorText)
{
    const QStringList filenames = QFileDialog::getOpenFileNames(
        nullptr,
        "选择图片",
        QString(),
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (filenames.isEmpty()) {
        return false;
    }

    QVariantList picked;
    picked.reserve(filenames.size());
    for (const QString &filename : filenames) {
        QImageReader reader(filename);
        if (!reader.canRead()) {
            if (errorText) {
                *errorText = QStringLiteral("图片读取失败: %1").arg(QFileInfo(filename).fileName());
            }
            return false;
        }
        picked.push_back(buildAttachmentMap(filename, QStringLiteral("image")));
    }

    if (attachments) {
        *attachments = picked;
    }
    return true;
}

bool LocalFilePickerService::pickMomentMediaUrls(QVariantList *attachments, QString *errorText)
{
    const QStringList filenames = QFileDialog::getOpenFileNames(
        nullptr,
        QStringLiteral("选择图片或视频"),
        QString(),
        QStringLiteral("媒体文件 (*.png *.jpg *.jpeg *.bmp *.webp *.gif *.mp4 *.mov *.avi *.mkv *.webm *.m4v)"));
    if (filenames.isEmpty()) {
        return false;
    }

    QVariantList picked;
    picked.reserve(filenames.size());
    for (const QString &filename : filenames) {
        const QString type = inferMomentAttachmentType(filename);
        if (type.isEmpty()) {
            if (errorText) {
                *errorText = QStringLiteral("暂不支持的素材类型: %1").arg(QFileInfo(filename).fileName());
            }
            return false;
        }
        if (type == QStringLiteral("image")) {
            QImageReader reader(filename);
            if (!reader.canRead()) {
                if (errorText) {
                    *errorText = QStringLiteral("图片读取失败: %1").arg(QFileInfo(filename).fileName());
                }
                return false;
            }
        }
        picked.push_back(buildAttachmentMap(filename, type));
    }

    if (attachments) {
        *attachments = picked;
    }
    return true;
}

bool LocalFilePickerService::pickFileUrl(QString *fileUrl, QString *displayName, QString *errorText)
{
    Q_UNUSED(errorText);
    const QString filename = QFileDialog::getOpenFileName(
        nullptr,
        "选择文件",
        QString(),
        "所有文件 (*.*)");
    if (filename.isEmpty()) {
        return false;
    }

    if (fileUrl) {
        *fileUrl = QUrl::fromLocalFile(filename).toString();
    }
    if (displayName) {
        *displayName = QFileInfo(filename).fileName();
    }
    return true;
}

bool LocalFilePickerService::pickFileUrls(QVariantList *attachments, QString *errorText)
{
    Q_UNUSED(errorText);
    const QStringList filenames = QFileDialog::getOpenFileNames(
        nullptr,
        "选择文件",
        QString(),
        "所有文件 (*.*)");
    if (filenames.isEmpty()) {
        return false;
    }

    QVariantList picked;
    picked.reserve(filenames.size());
    for (const QString &filename : filenames) {
        picked.push_back(buildAttachmentMap(filename, QStringLiteral("file")));
    }

    if (attachments) {
        *attachments = picked;
    }
    return true;
}

static bool doCropAndSaveAvatar(const QPixmap &src)
{
    Q_UNUSED(src);
    return false;
}

bool LocalFilePickerService::pickAvatarUrl(QString *avatarUrl, QString *errorText)
{
    const QString filename = QFileDialog::getOpenFileName(
        nullptr,
        "选择头像",
        QString(),
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (filename.isEmpty()) {
        return false;
    }

    QImageReader probeReader(filename);
    if (!probeReader.canRead()) {
        if (errorText) {
            *errorText = "头像读取失败";
        }
        return false;
    }

    const QPixmap cropped = ImageCropperDialog::getCroppedImage(filename, 600, 400, CropperShape::CIRCLE);
    if (cropped.isNull()) {
        return false;
    }

    const QImage image = cropped.toImage();
    if (image.isNull()) {
        if (errorText) {
            *errorText = "头像读取失败";
        }
        return false;
    }

    const QImage squared = image.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QString savedUrl = storeAvatarImage(squared);
    if (savedUrl.isEmpty()) {
        if (errorText) {
            *errorText = "头像保存失败";
        }
        return false;
    }

    if (avatarUrl) {
        *avatarUrl = savedUrl;
    }
    return true;
}

bool LocalFilePickerService::pickAvatarFromScreen(QString *avatarUrl, QString *errorText)
{
    QPixmap captured;
    if (!ScreenCaptureService::captureScreen(&captured) || captured.isNull()) {
        if (errorText) {
            *errorText = "截图已取消";
        }
        return false;
    }

    // Convert to image and pass to cropper
    const QImage img = captured.toImage();
    if (img.isNull()) {
        if (errorText) {
            *errorText = "截图读取失败";
        }
        return false;
    }

    const QPixmap cropped = ImageCropperDialog::getCroppedImage(
        QPixmap::fromImage(img), 600, 400, CropperShape::CIRCLE);
    if (cropped.isNull()) {
        if (errorText) {
            *errorText = "裁剪已取消";
        }
        return false;
    }

    const QImage finalImg = cropped.toImage();
    if (finalImg.isNull()) {
        if (errorText) {
            *errorText = "头像读取失败";
        }
        return false;
    }

    const QImage scaled = finalImg.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QString savedUrl = storeAvatarImage(scaled);
    if (savedUrl.isEmpty()) {
        if (errorText) {
            *errorText = "头像保存失败";
        }
        return false;
    }

    if (avatarUrl) {
        *avatarUrl = savedUrl;
    }
    return true;
}

bool LocalFilePickerService::pickAvatarFromWebcam(QString *avatarUrl, QString *errorText)
{
    const QPixmap captured = WebcamCaptureDialog::capture(nullptr);
    if (captured.isNull()) {
        if (errorText) {
            *errorText = "拍照已取消";
        }
        return false;
    }

    const QPixmap cropped = ImageCropperDialog::getCroppedImage(
        captured, 600, 400, CropperShape::CIRCLE);
    if (cropped.isNull()) {
        if (errorText) {
            *errorText = "裁剪已取消";
        }
        return false;
    }

    const QImage finalImg = cropped.toImage();
    if (finalImg.isNull()) {
        if (errorText) {
            *errorText = "头像读取失败";
        }
        return false;
    }

    const QImage scaled = finalImg.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QString savedUrl = storeAvatarImage(scaled);
    if (savedUrl.isEmpty()) {
        if (errorText) {
            *errorText = "头像保存失败";
        }
        return false;
    }

    if (avatarUrl) {
        *avatarUrl = savedUrl;
    }
    return true;
}

bool LocalFilePickerService::openUrl(const QString &urlText, QString *errorText)
{
    const QUrl directUrl(urlText);
    QUrl targetUrl = directUrl;
    if (!directUrl.isValid() || directUrl.scheme().isEmpty()) {
        targetUrl = QUrl::fromLocalFile(urlText);
    }

    if (!targetUrl.isValid()) {
        if (errorText) {
            *errorText = "无效的资源地址";
        }
        return false;
    }

    if (!QDesktopServices::openUrl(targetUrl)) {
        if (errorText) {
            *errorText = "打开资源失败";
        }
        return false;
    }
    return true;
}
