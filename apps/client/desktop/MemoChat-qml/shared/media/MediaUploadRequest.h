#ifndef MEDIAUPLOADREQUEST_H
#define MEDIAUPLOADREQUEST_H

#include <QString>

struct MediaUploadRequest
{
    QString localFileUrl;
    QString mediaType;
    int uid = 0;
    QString token;
    QString fallbackName;
};

#endif // MEDIAUPLOADREQUEST_H
