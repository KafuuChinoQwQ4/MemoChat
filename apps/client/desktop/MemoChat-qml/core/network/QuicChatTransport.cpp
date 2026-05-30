#include "QuicChatTransport.h"

#include <QDebug>
#include <QMetaObject>

QuicChatTransport::QuicChatTransport()
{
    _connect_timer.setSingleShot(true);
    QObject::connect(&_connect_timer,
                     &QTimer::timeout,
                     [this]()
                     {
                         if (!_connecting)
                         {
                             return;
                         }
                         qWarning() << "quic transport connection timeout. host:" << _host << "port:" << _port;
                         CloseConnection();
                         emit sig_con_success(false);
                     });
}

QuicChatTransport::~QuicChatTransport()
{
    CloseConnection();
}

void QuicChatTransport::resetParser()
{
    _frameParser.reset();
}

void QuicChatTransport::handleReceivedBytes(const QByteArray& bytes)
{
    const QVector<ChatFrame> frames = _frameParser.append(bytes);
    for (const ChatFrame& frame : frames)
    {
        emitMessageReceivedOnObjectThread(frame.reqId, frame.length, frame.payload);
    }
}

void QuicChatTransport::emitMessageReceivedOnObjectThread(ReqId reqId, int len, const QByteArray& data)
{
    QMetaObject::invokeMethod(
        this,
        [this, reqId, len, data]()
        {
            emit sig_message_received(reqId, len, data);
        },
        Qt::QueuedConnection);
}

void QuicChatTransport::CloseConnection()
{
    _connect_timer.stop();
    _connecting = false;
    const bool wasConnected = _connected;
    _connected = false;
    _streamReady = false;
    resetParser();
#if MEMOCHAT_HAS_MSQUIC
    closeHandles();
#endif
    if (wasConnected)
    {
        emit sig_connection_closed();
    }
}

bool QuicChatTransport::isConnected() const
{
    return _connected && _streamReady;
}
