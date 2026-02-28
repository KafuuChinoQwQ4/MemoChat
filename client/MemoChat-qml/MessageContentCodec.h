#ifndef MESSAGECONTENTCODEC_H
#define MESSAGECONTENTCODEC_H

#include <QString>

struct DecodedMessageContent {
    QString type;
    QString content;
    QString fileName;
};

class MessageContentCodec
{
public:
    static QString encodeImage(const QString &fileUrl);
    static QString encodeFile(const QString &fileUrl, const QString &fileName = QString());
    static QString encodeCallInvite(const QString &callType, const QString &joinUrl);
    static DecodedMessageContent decode(const QString &rawContent);
    static QString toPreviewText(const QString &rawContent);
};

#endif // MESSAGECONTENTCODEC_H
