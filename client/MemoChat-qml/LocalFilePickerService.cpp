#include "LocalFilePickerService.h"
#include "imagecropperdialog.h"
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPixmap>
#include <QStandardPaths>
#include <QUrl>

namespace {
QString storeAvatarImage(const QImage &image)
{
    QString storageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (storageDir.isEmpty()) {
        return {};
    }

    QDir dir(storageDir);
    if (!dir.exists("avatars") && !dir.mkpath("avatars")) {
        return {};
    }

    const QString avatarPath = dir.filePath("avatars/head.png");
    if (!image.save(avatarPath, "PNG")) {
        return {};
    }

    return QUrl::fromLocalFile(avatarPath).toString();
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
