#include "ChatFrameCodec.h"

#include <QDataStream>
#include <QIODevice>
#include <QtEndian>

namespace
{
constexpr int kChatFrameHeaderLen = sizeof(quint16) * 2;
}

QVector<ChatFrame> ChatFrameParser::append(const QByteArray& bytes)
{
    _buffer.append(bytes);

    QVector<ChatFrame> frames;
    while (true)
    {
        if (!_pending)
        {
            if (_buffer.size() < kChatFrameHeaderLen)
            {
                break;
            }

            const auto* raw = reinterpret_cast<const uchar*>(_buffer.constData());
            _messageId = qFromBigEndian<quint16>(raw);
            _messageLen = qFromBigEndian<quint16>(raw + sizeof(quint16));
            _buffer.remove(0, kChatFrameHeaderLen);
            _pending = true;
        }

        if (_buffer.size() < _messageLen)
        {
            break;
        }

        ChatFrame frame;
        frame.reqId = static_cast<ReqId>(_messageId);
        frame.length = static_cast<int>(_messageLen);
        frame.payload = QByteArray(_buffer.constData(), _messageLen);
        _buffer.remove(0, _messageLen);
        _pending = false;
        _messageId = 0;
        _messageLen = 0;
        frames.push_back(frame);
    }

    return frames;
}

void ChatFrameParser::reset()
{
    _buffer.clear();
    _pending = false;
    _messageId = 0;
    _messageLen = 0;
}

QByteArray encodeChatFrame(ReqId reqId, const QByteArray& payload)
{
    QByteArray frame;
    QDataStream out(&frame, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << static_cast<quint16>(reqId) << static_cast<quint16>(payload.size());
    frame.append(payload);
    return frame;
}
