#ifndef MEDIAUPLOADREQUEST_H
#define MEDIAUPLOADREQUEST_H

#include <QList>
#include <QString>
#include <QtGlobal>

struct MediaUploadRequest
{
    QString localFileUrl;
    QString mediaType;
    int uid = 0;
    QString token;
    QString fallbackName;
    QList<int> grantUids;
    qint64 grantGroupId = 0;
    bool grantPublic = false;
    bool grantFriends = false;
};

#endif // MEDIAUPLOADREQUEST_H
