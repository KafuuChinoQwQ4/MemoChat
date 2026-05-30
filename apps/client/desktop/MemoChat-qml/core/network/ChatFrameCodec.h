#ifndef CHATFRAMECODEC_H
#define CHATFRAMECODEC_H

#include <QByteArray>
#include <QVector>

#include "global.h"

struct ChatFrame
{
    ReqId reqId = static_cast<ReqId>(0);
    int length = 0;
    QByteArray payload;
};

class ChatFrameParser
{
public:
    QVector<ChatFrame> append(const QByteArray& bytes);
    void reset();

private:
    QByteArray _buffer;
    bool _pending = false;
    quint16 _messageId = 0;
    quint16 _messageLen = 0;
};

QByteArray encodeChatFrame(ReqId reqId, const QByteArray& payload);

#endif // CHATFRAMECODEC_H
