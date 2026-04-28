#include "tcpmgr.h"

#include <limits>

#include <QAbstractSocket>
#include <QDateTime>
#include <QDataStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QtEndian>

#include "TelemetryUtils.h"

namespace {
const char *socketErrorName(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::ConnectionRefusedError: return "ConnectionRefusedError";
    case QAbstractSocket::RemoteHostClosedError: return "RemoteHostClosedError";
    case QAbstractSocket::HostNotFoundError: return "HostNotFoundError";
    case QAbstractSocket::SocketTimeoutError: return "SocketTimeoutError";
    case QAbstractSocket::NetworkError: return "NetworkError";
    default: return "OtherSocketError";
    }
}
}

TcpMgr::TcpMgr()
    : _host(""),
      _port(0),
      _b_recv_pending(false),
      _connecting(false),
      _message_id(0),
      _message_len(0)
{
    _connect_timeout_timer.setSingleShot(true);
    _connect_timeout_timer.setInterval(15000);
    QObject::connect(&_connect_timeout_timer, &QTimer::timeout, [this]() {
        if (!_connecting) {
            return;
        }
        qWarning() << "TCP connect timeout. host:" << _host << "port:" << _port;
        _connecting = false;
        _socket.abort();
        emit sig_con_success(false);
    });

    QObject::connect(&_socket, &QTcpSocket::connected, [this]() {
        qInfo() << "tcp connect success. host:" << _host << "port:" << _port;
        _connect_timeout_timer.stop();
        _connecting = false;
        _buffer.clear();
        _b_recv_pending = false;
        _message_id = 0;
        _message_len = 0;
        emit sig_con_success(true);
    });

    QObject::connect(&_socket, &QTcpSocket::readyRead, [this]() {
        static constexpr int kHeaderLen = sizeof(quint16) * 2;
        static constexpr quint16 kMaxBodyLen = std::numeric_limits<quint16>::max();

        _buffer.append(_socket.readAll());
        while (true) {
            if (!_b_recv_pending) {
                if (_buffer.size() < kHeaderLen) {
                    break;
                }

                const auto *raw = reinterpret_cast<const uchar *>(_buffer.constData());
                _message_id = qFromBigEndian<quint16>(raw);
                _message_len = qFromBigEndian<quint16>(raw + sizeof(quint16));
                ::memmove(_buffer.data(), _buffer.data() + kHeaderLen, _buffer.size() - kHeaderLen);
                _buffer.chop(kHeaderLen);

                if (_message_len > kMaxBodyLen) {
                    qWarning() << "invalid message length:" << _message_len << ", reset parser";
                    _buffer.clear();
                    _b_recv_pending = false;
                    _message_id = 0;
                    _message_len = 0;
                    break;
                }
                _b_recv_pending = true;
            }

            if (_buffer.size() < _message_len) {
                break;
            }

            //修改前
            // const QByteArray messageBody = QByteArray::fromRawData(_buffer.constData(), _message_len);
            // ::memmove(_buffer.data(), _buffer.data() + _message_len, ...);
            
            // // 修改后：
            // const QByteArray messageBody(_buffer.constData(), _message_len);
            // ::memmove(_buffer.data(), _buffer.data() + _message_len, ...);
            // ```
            // and memmove() would corrupt the payload when another frame is already buffered.
            const QByteArray messageBody(_buffer.constData(), _message_len);
            ::memmove(_buffer.data(), _buffer.data() + _message_len, _buffer.size() - _message_len);
            _buffer.chop(_message_len);
            _b_recv_pending = false;
            handleMsg(ReqId(_message_id), _message_len, messageBody);
        }
    });

    QObject::connect(&_socket, &QTcpSocket::errorOccurred, [this](QAbstractSocket::SocketError socketError) {
        qWarning() << "tcp socket error:" << socketErrorName(socketError)
                   << "message:" << _socket.errorString()
                   << "host:" << _host
                   << "port:" << _port;
        const bool wasConnecting = _connecting;
        if (wasConnecting) {
            _connecting = false;
            _connect_timeout_timer.stop();
            emit sig_con_success(false);
        }
    });

    QObject::connect(&_socket, &QTcpSocket::disconnected, [this]() {
        qWarning() << "tcp disconnected. host:" << _host << "port:" << _port
                   << "connecting:" << _connecting;
        if (_connecting) {
            _connecting = false;
            _connect_timeout_timer.stop();
            emit sig_con_success(false);
        }
        _buffer.clear();
        _b_recv_pending = false;
        _message_id = 0;
        _message_len = 0;
        emit sig_connection_closed();
    });

    QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);
}

void TcpMgr::CloseConnection()
{
    qInfo() << "closing tcp connection. host:" << _host << "port:" << _port
            << "state:" << _socket.state();
    _connect_timeout_timer.stop();
    _connecting = false;
    _socket.close();
}

bool TcpMgr::isConnected() const
{
    return !_connecting && _socket.state() == QAbstractSocket::ConnectedState;
}

TcpMgr::~TcpMgr() = default;

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    emit sig_message_received(id, len, data);
}

void TcpMgr::connectToServer(ServerInfo si)
{
    qInfo() << "tcp connect start. host:" << si.Host << "port:" << si.Port << "uid:" << si.Uid;
    _host = si.Host.trimmed();
    if (_host == "0.0.0.0") {
        _host = "127.0.0.1";
    }

    bool portOk = false;
    const uint parsedPort = si.Port.trimmed().toUInt(&portOk);
    if (!portOk || parsedPort == 0 || parsedPort > std::numeric_limits<uint16_t>::max() || _host.isEmpty()) {
        qWarning() << "invalid chat server endpoint, host:" << _host << "port:" << si.Port;
        emit sig_con_success(false);
        return;
    }

    _port = static_cast<uint16_t>(parsedPort);
    if (_socket.state() != QAbstractSocket::UnconnectedState) {
        _socket.abort();
    }

    _buffer.clear();
    _b_recv_pending = false;
    _message_id = 0;
    _message_len = 0;
    _connecting = true;
    _connect_timeout_timer.setInterval(si.ConnectTimeoutMs > 0 ? si.ConnectTimeoutMs : 1200);
    _connect_timeout_timer.start();
    qInfo() << "connecting to chat server, host:" << _host << "port:" << _port
            << "timeout_ms:" << _connect_timeout_timer.interval()
            << "protocol_version:" << si.ProtocolVersion;
    _socket.connectToHost(_host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QByteArray dataBytes)
{
    if (_socket.state() != QAbstractSocket::ConnectedState) {
        qWarning() << "skip send: tcp socket is not connected, req id:" << reqId;
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument requestDoc = QJsonDocument::fromJson(dataBytes, &parseError);
    QString traceId;
    QString requestId;
    QString spanId;
    if (parseError.error == QJsonParseError::NoError && requestDoc.isObject()) {
        QJsonObject obj = requestDoc.object();
        traceId = obj.value("trace_id").toString().trimmed();
        requestId = obj.value("request_id").toString().trimmed();
        spanId = obj.value("span_id").toString().trimmed();
        if (traceId.isEmpty()) {
            traceId = newTraceId();
            obj.insert("trace_id", traceId);
        }
        if (requestId.isEmpty()) {
            requestId = newRequestId();
            obj.insert("request_id", requestId);
        }
        if (spanId.isEmpty()) {
            spanId = newSpanId();
            obj.insert("span_id", spanId);
        }
        dataBytes = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }

    const qint64 startAtMs = QDateTime::currentMSecsSinceEpoch();
    const uint16_t id = reqId;
    const quint16 len = static_cast<quint16>(dataBytes.length());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << id << len;
    block.append(dataBytes);

    _socket.write(block);
    QVariantMap spanAttrs;
    spanAttrs.insert("net.transport", QStringLiteral("ip_tcp"));
    spanAttrs.insert("messaging.system", QStringLiteral("memochat-tcp"));
    spanAttrs.insert("messaging.operation", QStringLiteral("send"));
    spanAttrs.insert("messaging.message_id", static_cast<int>(reqId));
    spanAttrs.insert("net.peer.name", _host);
    spanAttrs.insert("net.peer.port", static_cast<int>(_port));
    exportZipkinSpan(QStringLiteral("TCP %1").arg(static_cast<int>(reqId)),
                     QStringLiteral("CLIENT"),
                     traceId,
                     spanId,
                     QString(),
                     startAtMs,
                     qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startAtMs),
                     spanAttrs);
    if (reqId == ReqId::ID_CHAT_LOGIN) {
        qInfo() << "chat login payload sent. bytes:" << block.size();
    } else {
        qDebug() << "tcp mgr send byte data is " << block;
    }
}
