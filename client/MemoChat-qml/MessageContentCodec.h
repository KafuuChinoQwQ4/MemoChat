#ifndef MESSAGECONTENTCODEC_H
#define MESSAGECONTENTCODEC_H

#include <QString>

struct DecodedMessageContent {
    QString type;
    QString content;
    QString fileName;
    bool isReply = false;
    QString replyToMsgId;
    QString replySender;
    QString replyPreview;
};

class MessageContentCodec
{
public:
    static QString encodeImage(const QString &fileUrl);
    static QString encodeFile(const QString &fileUrl, const QString &fileName = QString());
    static QString encodeCallInvite(const QString &callType, const QString &joinUrl);
    static QString encodeReplyText(const QString &text, const QString &replyToMsgId,
                                   const QString &replySender, const QString &replyPreview);
    static DecodedMessageContent decode(const QString &rawContent);
    static QString toPreviewText(const QString &rawContent);
};

#endif // MESSAGECONTENTCODEC_H
