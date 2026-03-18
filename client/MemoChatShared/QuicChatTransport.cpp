#include "QuicChatTransport.h"

#include <QDataStream>
#include <QDebug>
#include <QtEndian>
#include <QtGlobal>

#if MEMOCHAT_HAS_MSQUIC
#include <msquic.h>

struct QuicChatTransport::SendContext {
    QByteArray bytes;
    QUIC_BUFFER buffer{};
};
#endif

namespace {
constexpr const char *kDefaultAlpn = "memochat-chat";
}

QuicChatTransport::QuicChatTransport()
{
    _connect_timer.setSingleShot(true);
    QObject::connect(&_connect_timer, &QTimer::timeout, [this]() {
        if (!_connecting) {
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
    _recvPending = false;
    _messageId = 0;
    _messageLen = 0;
    _recvBuffer.clear();
}

void QuicChatTransport::handleReceivedBytes(const QByteArray &bytes)
{
    _recvBuffer.append(bytes);
    while (true) {
        if (!_recvPending) {
            if (_recvBuffer.size() < 4) {
                return;
            }

            const auto *raw = reinterpret_cast<const uchar *>(_recvBuffer.constData());
            _messageId = qFromBigEndian<quint16>(raw);
            _messageLen = qFromBigEndian<quint16>(raw + sizeof(quint16));
            _recvBuffer.remove(0, 4);
            _recvPending = true;
        }

        if (_recvBuffer.size() < _messageLen) {
            return;
        }

        QByteArray payload = _recvBuffer.left(_messageLen);
        _recvBuffer.remove(0, _messageLen);
        _recvPending = false;
        emit sig_message_received(static_cast<ReqId>(_messageId), _messageLen, payload);
    }
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
    if (wasConnected) {
        emit sig_connection_closed();
    }
}

bool QuicChatTransport::isConnected() const
{
    return _connected && _streamReady;
}

void QuicChatTransport::connectToServer(ServerInfo serverInfo)
{
    _host = serverInfo.Host.trimmed();
    _serverName = serverInfo.ServerName.trimmed();
    bool portOk = false;
    const uint parsedPort = serverInfo.Port.trimmed().toUInt(&portOk);
    _port = portOk ? static_cast<uint16_t>(parsedPort) : 0;
    _connecting = false;
    _connected = false;
    _streamReady = false;
    resetParser();

    if (_host.isEmpty() || _port == 0) {
        qWarning() << "quic transport invalid endpoint. host:" << _host << "port:" << serverInfo.Port;
        emit sig_con_success(false);
        return;
    }

#if MEMOCHAT_HAS_MSQUIC
    QString errorText;
    if (!ensureQuicReady(&errorText)) {
        qWarning() << "quic transport initialization failed:" << errorText;
        emit sig_con_success(false);
        return;
    }

    _connecting = true;
    _connect_timer.setInterval(serverInfo.ConnectTimeoutMs > 0 ? serverInfo.ConnectTimeoutMs : 1200);
    _connect_timer.start();
    const auto hostUtf8 = _host.toUtf8();
    const QUIC_STATUS status = _api->ConnectionOpen(
        _registration,
        reinterpret_cast<QUIC_CONNECTION_CALLBACK_HANDLER>(connectionCallback),
        this,
        &_connection);
    if (QUIC_FAILED(status)) {
        qWarning() << "quic transport connection open failed. status:" << status;
        closeHandles();
        _connecting = false;
        emit sig_con_success(false);
        return;
    }

    const QUIC_STATUS startStatus = _api->ConnectionStart(
        _connection,
        _configuration,
        QUIC_ADDRESS_FAMILY_UNSPEC,
        hostUtf8.constData(),
        _port);
    if (QUIC_FAILED(startStatus)) {
        qWarning() << "quic transport connection start failed. status:" << startStatus;
        closeHandles();
        _connecting = false;
        emit sig_con_success(false);
        return;
    }

    qInfo() << "quic connect start. host:" << _host << "port:" << _port
            << "server:" << _serverName
            << "protocol_version:" << serverInfo.ProtocolVersion;
#else
    qInfo() << "quic transport connect skipped: MsQuic dependency not present";
    emit sig_con_success(false);
#endif
}

void QuicChatTransport::slot_send_data(ReqId reqId, QByteArray data)
{
#if MEMOCHAT_HAS_MSQUIC
    if (!_streamReady || _stream == nullptr) {
        qWarning() << "skip quic send: stream not ready, req id:" << reqId;
        return;
    }

    auto *ctx = new SendContext();
    QDataStream out(&ctx->bytes, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << static_cast<quint16>(reqId) << static_cast<quint16>(data.size());
    ctx->bytes.append(data);
    ctx->buffer.Buffer = reinterpret_cast<uint8_t *>(ctx->bytes.data());
    ctx->buffer.Length = static_cast<uint32_t>(ctx->bytes.size());

    const QUIC_STATUS status = _api->StreamSend(
        _stream,
        &ctx->buffer,
        1,
        QUIC_SEND_FLAG_NONE,
        ctx);
    if (QUIC_FAILED(status)) {
        qWarning() << "quic stream send failed. status:" << status << "req id:" << reqId;
        delete ctx;
        CloseConnection();
    }
#else
    Q_UNUSED(reqId);
    Q_UNUSED(data);
    qWarning() << "quic transport send skipped: MsQuic dependency not present";
#endif
}

#if MEMOCHAT_HAS_MSQUIC
bool QuicChatTransport::ensureQuicReady(QString *errorText)
{
    if (_api != nullptr && _registration != nullptr && _configuration != nullptr) {
        return true;
    }

    QUIC_STATUS status = MsQuicOpen2(&_api);
    if (QUIC_FAILED(status) || _api == nullptr) {
        if (errorText) {
            *errorText = QStringLiteral("msquic_open_failed");
        }
        return false;
    }

    QUIC_REGISTRATION_CONFIG regConfig = { "MemoChatClient", QUIC_EXECUTION_PROFILE_LOW_LATENCY };
    status = _api->RegistrationOpen(&regConfig, &_registration);
    if (QUIC_FAILED(status)) {
        if (errorText) {
            *errorText = QStringLiteral("registration_open_failed");
        }
        closeHandles();
        return false;
    }

    _alpn = QByteArray(kDefaultAlpn);
    QUIC_BUFFER alpnBuffer;
    alpnBuffer.Buffer = reinterpret_cast<uint8_t *>(_alpn.data());
    alpnBuffer.Length = static_cast<uint32_t>(_alpn.size());

    QUIC_SETTINGS settings{};
    settings.IdleTimeoutMs = 30000;
    settings.IsSet.IdleTimeoutMs = TRUE;
    settings.PeerBidiStreamCount = 1;
    settings.IsSet.PeerBidiStreamCount = TRUE;

    status = _api->ConfigurationOpen(_registration, &alpnBuffer, 1, &settings, sizeof(settings), nullptr, &_configuration);
    if (QUIC_FAILED(status)) {
        if (errorText) {
            *errorText = QStringLiteral("configuration_open_failed");
        }
        closeHandles();
        return false;
    }

    QUIC_CREDENTIAL_CONFIG cred{};
    cred.Type = QUIC_CREDENTIAL_TYPE_NONE;
    cred.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;
    status = _api->ConfigurationLoadCredential(_configuration, &cred);
    if (QUIC_FAILED(status)) {
        if (errorText) {
            *errorText = QStringLiteral("credential_load_failed");
        }
        closeHandles();
        return false;
    }

    return true;
}

void QuicChatTransport::closeHandles()
{
    if (_stream != nullptr && _api != nullptr) {
        _api->StreamShutdown(_stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
        _api->StreamClose(_stream);
        _stream = nullptr;
    }
    if (_connection != nullptr && _api != nullptr) {
        _api->ConnectionShutdown(_connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
        _api->ConnectionClose(_connection);
        _connection = nullptr;
    }
}

int QUIC_API QuicChatTransport::connectionCallback(HQUIC, void *context, void *event)
{
    return static_cast<QuicChatTransport *>(context)->onConnectionEvent(event);
}

int QUIC_API QuicChatTransport::streamCallback(HQUIC, void *context, void *event)
{
    return static_cast<QuicChatTransport *>(context)->onStreamEvent(event);
}

int QuicChatTransport::onConnectionEvent(void *eventPtr)
{
    auto *event = static_cast<QUIC_CONNECTION_EVENT *>(eventPtr);
    if (event == nullptr) {
        return QUIC_STATUS_INVALID_STATE;
    }

    switch (event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED: {
        const QUIC_STATUS openStatus = _api->StreamOpen(
            _connection,
            QUIC_STREAM_OPEN_FLAG_NONE,
            reinterpret_cast<QUIC_STREAM_CALLBACK_HANDLER>(streamCallback),
            this,
            &_stream);
        if (QUIC_FAILED(openStatus)) {
            qWarning() << "quic stream open failed. status:" << openStatus;
            CloseConnection();
            emit sig_con_success(false);
            return QUIC_STATUS_SUCCESS;
        }

        const QUIC_STATUS startStatus = _api->StreamStart(_stream, QUIC_STREAM_START_FLAG_NONE);
        if (QUIC_FAILED(startStatus)) {
            qWarning() << "quic stream start failed. status:" << startStatus;
            CloseConnection();
            emit sig_con_success(false);
            return QUIC_STATUS_SUCCESS;
        }
        return QUIC_STATUS_SUCCESS;
    }
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        if (_connecting) {
            _connect_timer.stop();
            _connecting = false;
            emit sig_con_success(false);
        } else if (_connected) {
            CloseConnection();
        }
        return QUIC_STATUS_SUCCESS;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        if (_connected) {
            CloseConnection();
        }
        return QUIC_STATUS_SUCCESS;
    default:
        return QUIC_STATUS_SUCCESS;
    }
}

int QuicChatTransport::onStreamEvent(void *eventPtr)
{
    auto *event = static_cast<QUIC_STREAM_EVENT *>(eventPtr);
    if (event == nullptr) {
        return QUIC_STATUS_INVALID_STATE;
    }

    switch (event->Type) {
    case QUIC_STREAM_EVENT_START_COMPLETE:
        _connect_timer.stop();
        _connecting = false;
        _connected = true;
        _streamReady = true;
        emit sig_con_success(true);
        return QUIC_STATUS_SUCCESS;
    case QUIC_STREAM_EVENT_RECEIVE: {
        for (uint32_t i = 0; i < event->RECEIVE.BufferCount; ++i) {
            const auto &buffer = event->RECEIVE.Buffers[i];
            handleReceivedBytes(QByteArray(reinterpret_cast<const char *>(buffer.Buffer), static_cast<int>(buffer.Length)));
        }
        return QUIC_STATUS_SUCCESS;
    }
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        delete static_cast<SendContext *>(event->SEND_COMPLETE.ClientContext);
        return QUIC_STATUS_SUCCESS;
    case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        if (_connected) {
            CloseConnection();
        }
        return QUIC_STATUS_SUCCESS;
    default:
        return QUIC_STATUS_SUCCESS;
    }
}
#endif
