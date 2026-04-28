#ifndef LOCALFILEPICKERSERVICE_H
#define LOCALFILEPICKERSERVICE_H

#include <QString>
#include <QVariantList>

class LocalFilePickerService
{
public:
    static bool pickImageUrl(QString *fileUrl, QString *errorText);
    static bool pickFileUrl(QString *fileUrl, QString *displayName, QString *errorText);
    static bool pickImageUrls(QVariantList *attachments, QString *errorText);
    static bool pickMomentMediaUrls(QVariantList *attachments, QString *errorText);
    static bool pickFileUrls(QVariantList *attachments, QString *errorText);
    static bool pickAvatarUrl(QString *avatarUrl, QString *errorText);
    static bool pickAvatarFromScreen(QString *avatarUrl, QString *errorText);
    static bool pickAvatarFromWebcam(QString *avatarUrl, QString *errorText);
    static bool openUrl(const QString &urlText, QString *errorText);
};

#endif // LOCALFILEPICKERSERVICE_H
