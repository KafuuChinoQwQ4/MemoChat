#include "LocalFilePickerService.h"

#include "LocalFilePickerServicePrivate.h"

#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QSet>
#include <QUrl>
#include <QUuid>

QString inferMomentAttachmentType(const QString& filename)
{
    const QMimeDatabase mimeDb;
    const QMimeType mimeType = mimeDb.mimeTypeForFile(filename, QMimeDatabase::MatchContent);
    if (mimeType.name().startsWith(QStringLiteral("image/")))
    {
        return QStringLiteral("image");
    }
    if (mimeType.name().startsWith(QStringLiteral("video/")))
    {
        return QStringLiteral("video");
    }

    const QString suffix = QFileInfo(filename).suffix().toLower();
    static const QSet<QString> imageSuffixes = {QStringLiteral("png"),
        QStringLiteral("jpg"),
                       QStringLiteral("jpeg"), QStringLiteral("bmp"), QStringLiteral("webp"), QStringLiteral("gif") };
    static const QSet<QString> videoSuffixes = {QStringLiteral("mp4"),
        QStringLiteral("mov"),
                       QStringLiteral("avi"), QStringLiteral("mkv"), QStringLiteral("webm"), QStringLiteral("m4v") };
    if (imageSuffixes.contains(suffix))
    {
        return QStringLiteral("image");
    }
    if (videoSuffixes.contains(suffix))
    {
        return QStringLiteral("video");
    }
    return {};
}

QVariantMap buildAttachmentMap(const QString& filename, const QString& type)
{
    QVariantMap attachment;
    const QFileInfo fileInfo(filename);
    attachment.insert(QStringLiteral("attachmentId"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    attachment.insert(QStringLiteral("type"), type);
    attachment.insert(QStringLiteral("fileUrl"), QUrl::fromLocalFile(filename).toString());
    attachment.insert(QStringLiteral("fileName"), fileInfo.fileName());
    attachment.insert(QStringLiteral("sizeBytes"), fileInfo.size());
    attachment.insert(QStringLiteral("selectedAt"), QDateTime::currentMSecsSinceEpoch());

    if (type == QStringLiteral("image"))
    {
        QImageReader reader(filename);
        const QSize imageSize = reader.size();
        attachment.insert(QStringLiteral("previewUrl"), QUrl::fromLocalFile(filename).toString());
        attachment.insert(QStringLiteral("width"), imageSize.width());
        attachment.insert(QStringLiteral("height"), imageSize.height());
        attachment.insert(QStringLiteral("mimeType"),
            reader.format().isEmpty() ? QString()
                                      : QStringLiteral("image/%1").arg(QString::fromLatin1(reader.format()).toLower()));
    }
    else if (type == QStringLiteral("video"))
    {
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

bool LocalFilePickerService::pickImageUrl(QString* fileUrl, QString* errorText)
{
    const QString filename =
        QFileDialog::getOpenFileName(nullptr, "选择图片", QString(), "图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (filename.isEmpty())
    {
        return false;
    }

    QImageReader reader(filename);
    if (!reader.canRead())
    {
        if (errorText)
        {
            *errorText = "图片读取失败";
        }
        return false;
    }

    if (fileUrl)
    {
        *fileUrl = QUrl::fromLocalFile(filename).toString();
    }
    return true;
}

bool LocalFilePickerService::pickImageUrls(QVariantList* attachments, QString* errorText)
{
    const QStringList filenames =
        QFileDialog::getOpenFileNames(nullptr, "选择图片", QString(), "图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (filenames.isEmpty())
    {
        return false;
    }

    QVariantList picked;
    picked.reserve(filenames.size());
    for (const QString& filename : filenames)
    {
        QImageReader reader(filename);
        if (!reader.canRead())
        {
            if (errorText)
            {
                *errorText = QStringLiteral("图片读取失败: %1").arg(QFileInfo(filename).fileName());
            }
            return false;
        }
        picked.push_back(buildAttachmentMap(filename, QStringLiteral("image")));
    }

    if (attachments)
    {
        *attachments = picked;
    }
    return true;
}

bool LocalFilePickerService::pickMomentMediaUrls(QVariantList* attachments, QString* errorText)
{
    const QStringList filenames = QFileDialog::getOpenFileNames(
        nullptr,
        QStringLiteral("选择图片或视频"),
            QString(),
            QStringLiteral("媒体文件 (*.png *.jpg *.jpeg *.bmp *.webp *.gif *.mp4 *.mov *.avi *.mkv *.webm *.m4v)"));
    if (filenames.isEmpty())
    {
        return false;
    }

    QVariantList picked;
    picked.reserve(filenames.size());
    for (const QString& filename : filenames)
    {
        const QString type = inferMomentAttachmentType(filename);
        if (type.isEmpty())
        {
            if (errorText)
            {
                *errorText = QStringLiteral("暂不支持的素材类型: %1").arg(QFileInfo(filename).fileName());
            }
            return false;
        }
        if (type == QStringLiteral("image"))
        {
            QImageReader reader(filename);
            if (!reader.canRead())
            {
                if (errorText)
                {
                    *errorText = QStringLiteral("图片读取失败: %1").arg(QFileInfo(filename).fileName());
                }
                return false;
            }
        }
        picked.push_back(buildAttachmentMap(filename, type));
    }

    if (attachments)
    {
        *attachments = picked;
    }
    return true;
}

bool LocalFilePickerService::pickFileUrl(QString* fileUrl, QString* displayName, QString* errorText)
{
    Q_UNUSED(errorText);
    const QString filename = QFileDialog::getOpenFileName(nullptr, "选择文件", QString(), "所有文件 (*.*)");
    if (filename.isEmpty())
    {
        return false;
    }

    if (fileUrl)
    {
        *fileUrl = QUrl::fromLocalFile(filename).toString();
    }
    if (displayName)
    {
        *displayName = QFileInfo(filename).fileName();
    }
    return true;
}

bool LocalFilePickerService::pickFileUrls(QVariantList* attachments, QString* errorText)
{
    Q_UNUSED(errorText);
    const QStringList filenames = QFileDialog::getOpenFileNames(nullptr, "选择文件", QString(), "所有文件 (*.*)");
    if (filenames.isEmpty())
    {
        return false;
    }

    QVariantList picked;
    picked.reserve(filenames.size());
    for (const QString& filename : filenames)
    {
        picked.push_back(buildAttachmentMap(filename, QStringLiteral("file")));
    }

    if (attachments)
    {
        *attachments = picked;
    }
    return true;
}
