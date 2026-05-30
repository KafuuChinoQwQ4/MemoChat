#include "LocalFilePickerService.h"

#include "imagecropperdialog.h"
#include "ScreenCaptureService.h"
#include "WebcamCaptureDialog.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>

namespace
{
QMutex g_avatar_mutex;

QString getAvatarDir()
{
    const QString storageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (storageDir.isEmpty())
    {
        return {};
    }
    QDir dir(storageDir);
    if (!dir.exists("avatars") && !dir.mkpath("avatars"))
    {
        return {};
    }
    return dir.filePath("avatars");
}

void cleanupOldAvatars(const QString& keepUrl)
{
    QMutexLocker locker(&g_avatar_mutex);
    const QString avatarsDir = getAvatarDir();
    if (avatarsDir.isEmpty())
    {
        return;
    }
    const QString keepName = QFileInfo(keepUrl).fileName();
    QDir dir(avatarsDir);
    for (const QString& file : dir.entryList({"head_*.jpg", "head_*.png"}))
    {
        if (file != keepName)
        {
            dir.remove(file);
        }
    }
}

QString storeAvatarImage(const QImage& image)
{
    const QString avatarsDir = getAvatarDir();
    if (avatarsDir.isEmpty())
    {
        return {};
    }

    const QString uniqueName = QStringLiteral("head_%1.jpg") .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString avatarPath = avatarsDir + "/" + uniqueName;

    QImageWriter writer(avatarPath, "JPEG");
    writer.setQuality(90);
    if (!writer.write(image))
    {
        return {};
    }

    const QString url = QUrl::fromLocalFile(avatarPath).toString();
    cleanupOldAvatars(url);
    return url;
}
} // namespace

bool LocalFilePickerService::pickAvatarUrl(QString* avatarUrl, QString* errorText)
{
    const QString filename =
        QFileDialog::getOpenFileName(nullptr, "选择头像", QString(), "图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (filename.isEmpty())
    {
        return false;
    }

    QImageReader probeReader(filename);
    if (!probeReader.canRead())
    {
        if (errorText)
        {
            *errorText = "头像读取失败";
        }
        return false;
    }

    const QPixmap cropped = ImageCropperDialog::getCroppedImage(filename, 600, 400, CropperShape::CIRCLE);
    if (cropped.isNull())
    {
        return false;
    }

    const QImage image = cropped.toImage();
    if (image.isNull())
    {
        if (errorText)
        {
            *errorText = "头像读取失败";
        }
        return false;
    }

    const QImage squared = image.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QString savedUrl = storeAvatarImage(squared);
    if (savedUrl.isEmpty())
    {
        if (errorText)
        {
            *errorText = "头像保存失败";
        }
        return false;
    }

    if (avatarUrl)
    {
        *avatarUrl = savedUrl;
    }
    return true;
}

bool LocalFilePickerService::pickAvatarFromScreen(QString* avatarUrl, QString* errorText)
{
    QPixmap captured;
    if (!ScreenCaptureService::captureScreen(&captured) || captured.isNull())
    {
        if (errorText)
        {
            *errorText = "截图已取消";
        }
        return false;
    }

    const QImage img = captured.toImage();
    if (img.isNull())
    {
        if (errorText)
        {
            *errorText = "截图读取失败";
        }
        return false;
    }

    const QPixmap cropped =
        ImageCropperDialog::getCroppedImage(QPixmap::fromImage(img), 600, 400, CropperShape::CIRCLE);
    if (cropped.isNull())
    {
        if (errorText)
        {
            *errorText = "裁剪已取消";
        }
        return false;
    }

    const QImage finalImg = cropped.toImage();
    if (finalImg.isNull())
    {
        if (errorText)
        {
            *errorText = "头像读取失败";
        }
        return false;
    }

    const QImage scaled = finalImg.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QString savedUrl = storeAvatarImage(scaled);
    if (savedUrl.isEmpty())
    {
        if (errorText)
        {
            *errorText = "头像保存失败";
        }
        return false;
    }

    if (avatarUrl)
    {
        *avatarUrl = savedUrl;
    }
    return true;
}

bool LocalFilePickerService::pickAvatarFromWebcam(QString* avatarUrl, QString* errorText)
{
    const QPixmap captured = WebcamCaptureDialog::capture(nullptr);
    if (captured.isNull())
    {
        if (errorText)
        {
            *errorText = "拍照已取消";
        }
        return false;
    }

    const QPixmap cropped = ImageCropperDialog::getCroppedImage(captured, 600, 400, CropperShape::CIRCLE);
    if (cropped.isNull())
    {
        if (errorText)
        {
            *errorText = "裁剪已取消";
        }
        return false;
    }

    const QImage finalImg = cropped.toImage();
    if (finalImg.isNull())
    {
        if (errorText)
        {
            *errorText = "头像读取失败";
        }
        return false;
    }

    const QImage scaled = finalImg.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QString savedUrl = storeAvatarImage(scaled);
    if (savedUrl.isEmpty())
    {
        if (errorText)
        {
            *errorText = "头像保存失败";
        }
        return false;
    }

    if (avatarUrl)
    {
        *avatarUrl = savedUrl;
    }
    return true;
}
