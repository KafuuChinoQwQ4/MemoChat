#include "QuicChatServer.h"

#include "ConfigMgr.h"
#include "MsgNode.h"
#include "QuicSession.h"
#include "logging/Logger.h"

#include <atomic>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#if MEMOCHAT_ENABLE_MSQUIC
#include "../common/WinSdkCompat.h"
#include <wincrypt.h>
#include <msquic.h>
#endif

namespace memochat::chatserver {

namespace {
constexpr const char* kDefaultAlpn = "memochat-chat";

std::string ReadConfigOrDefault(const char* section, const char* key, const char* fallback)
{
    auto& cfg = ConfigMgr::Inst();
    auto value = cfg.GetValue(section, key);
    if (value.empty()) {
        value = fallback;
    }
    return value;
}

#if MEMOCHAT_ENABLE_MSQUIC
// Optimized frame encoder: pre-allocates vector capacity to reduce reallocations.
// Frame format: [msg_id(2B big-endian)][payload_len(2B big-endian)][payload]
// This matches the TCP protocol for cross-transport compatibility.
std::vector<uint8_t> EncodeFrame(short msgid, const std::string& payload)
{
    std::vector<uint8_t> frame;
    // Pre-reserve to avoid reallocations (major perf gain for high-throughput paths)
    frame.reserve(HEAD_TOTAL_LEN + payload.size());
    frame.resize(HEAD_TOTAL_LEN);
    const short network_id = boost::asio::detail::socket_ops::host_to_network_short(msgid);
    const short network_len = boost::asio::detail::socket_ops::host_to_network_short(static_cast<short>(payload.size()));
    std::memcpy(frame.data(), &network_id, HEAD_ID_LEN);
    std::memcpy(frame.data() + HEAD_ID_LEN, &network_len, HEAD_DATA_LEN);
    if (!payload.empty()) {
        frame.insert(frame.end(), payload.begin(), payload.end());
    }
    return frame;
}
#endif
}

#if MEMOCHAT_ENABLE_MSQUIC
struct QuicChatServer::Impl {
    struct SendContext {
        std::vector<uint8_t> bytes;
        QUIC_BUFFER buffer{};
    };

    struct StreamContext;

    struct ConnectionContext {
        QuicChatServer* owner = nullptr;
        HQUIC handle = nullptr;
        HQUIC stream = nullptr;
        std::shared_ptr<QuicSession> session;
        std::unique_ptr<StreamContext> stream_context;
        const QUIC_API_TABLE* api = nullptr;
    };

    struct StreamContext {
        ConnectionContext* connection = nullptr;
        std::vector<uint8_t> recv_buffer;
        bool recv_pending = false;
        short msg_id = 0;
        short msg_len = 0;
        bool start_complete = false;
        std::vector<std::pair<short, std::string>> pending_sends;
    };

    const QUIC_API_TABLE* api = nullptr;
    HQUIC registration = nullptr;
    HQUIC configuration = nullptr;
    HQUIC listener = nullptr;
    QUIC_BUFFER alpn_buffer{};
    std::string alpn = kDefaultAlpn;
    std::string cert_file;
    std::string key_file;

    static QUIC_STATUS QUIC_API ListenerCallback(HQUIC listener, void* context, QUIC_LISTENER_EVENT* event);
    static QUIC_STATUS QUIC_API ConnectionCallback(HQUIC connection, void* context, QUIC_CONNECTION_EVENT* event);
    static QUIC_STATUS QUIC_API StreamCallback(HQUIC stream, void* context, QUIC_STREAM_EVENT* event);

    bool ensureInitialized(QuicChatServer* owner, std::string* error);
    void shutdownHandles();
    static void closeConnectionHandles(Impl* impl, ConnectionContext* ctx);
    static void dispatchFrames(StreamContext* stream_context);
};
#else
struct QuicChatServer::Impl {
    // Stub implementation when QUIC is disabled
    bool ensureInitialized(QuicChatServer* owner, std::string* error);
    void shutdownHandles();
};
#endif

#if MEMOCHAT_ENABLE_MSQUIC
bool QuicChatServer::Impl::ensureInitialized(QuicChatServer* owner, std::string* error)
{
    (void)owner;
    if (api != nullptr && registration != nullptr && configuration != nullptr) {
        return true;
    }

    QUIC_STATUS status = MsQuicOpen2(&api);
    if (QUIC_FAILED(status) || api == nullptr) {
        if (error) {
            *error = "msquic_open_failed";
        }
        return false;
    }

    QUIC_REGISTRATION_CONFIG reg_config = { "MemoChatServer", QUIC_EXECUTION_PROFILE_LOW_LATENCY };
    status = api->RegistrationOpen(&reg_config, &registration);
    if (QUIC_FAILED(status)) {
        if (error) {
            *error = "registration_open_failed";
        }
        shutdownHandles();
        return false;
    }

    alpn = ReadConfigOrDefault("Quic", "Alpn", kDefaultAlpn);
    alpn_buffer.Buffer = reinterpret_cast<uint8_t*>(alpn.data());
    alpn_buffer.Length = static_cast<uint32_t>(alpn.size());

    QUIC_SETTINGS settings{};
    settings.IdleTimeoutMs = 30000;
    settings.IsSet.IdleTimeoutMs = TRUE;
    settings.PeerBidiStreamCount = 8;
    settings.IsSet.PeerBidiStreamCount = TRUE;

    status = api->ConfigurationOpen(registration, &alpn_buffer, 1, &settings, sizeof(settings), nullptr, &configuration);
    if (QUIC_FAILED(status)) {
        if (error) {
            *error = "configuration_open_failed";
        }
        shutdownHandles();
        return false;
    }

    std::string cert_file = ReadConfigOrDefault("Quic", "CertFile", "");
    std::string key_file  = ReadConfigOrDefault("Quic", "PrivateKeyFile", "");
    std::string pfx_file  = ReadConfigOrDefault("Quic", "PfxFile", "");
    std::string pfx_password = ReadConfigOrDefault("Quic", "PfxPassword", "");
    std::string cert_thumbprint = ReadConfigOrDefault("Quic", "CertThumbprint", "");
    std::string cert_store_name = ReadConfigOrDefault("Quic", "CertStore", "My");
    std::string cert_store_location = ReadConfigOrDefault("Quic", "CertStoreLocation", "CurrentUser");

    bool cred_loaded = false;

    // Approach 1: Try CERTIFICATE_CONTEXT via Windows Certificate Store (thumbprint)
    if (!cred_loaded && !cert_thumbprint.empty()) {
        HCERTSTORE hStore = CertOpenSystemStoreW(NULL, L"My");
        if (hStore) {
            PCCERT_CONTEXT found_ctx = nullptr;
            std::vector<BYTE> targetHash;
            for (size_t i = 0; i < cert_thumbprint.size(); i += 2) {
                std::string byte_str = cert_thumbprint.substr(i, 2);
                targetHash.push_back(static_cast<BYTE>(std::stoi(byte_str, nullptr, 16)));
            }
            for (PCCERT_CONTEXT ctx = CertEnumCertificatesInStore(hStore, nullptr);
                 ctx != nullptr; ctx = CertEnumCertificatesInStore(hStore, ctx)) {
                DWORD cbHash = 0;
                CertGetCertificateContextProperty(ctx, CERT_HASH_PROP_ID, nullptr, &cbHash);
                if (cbHash == targetHash.size()) {
                    std::vector<BYTE> ctxHash(cbHash);
                    CertGetCertificateContextProperty(ctx, CERT_HASH_PROP_ID, ctxHash.data(), &cbHash);
                    if (ctxHash == targetHash) {
                        found_ctx = CertDuplicateCertificateContext(ctx);
                        break;
                    }
                }
            }
            if (found_ctx) {
                QUIC_CREDENTIAL_CONFIG cred{};
                cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_CONTEXT;
                cred.CertificateContext = const_cast<CERT_CONTEXT*>(found_ctx);
                cred.Flags = QUIC_CREDENTIAL_FLAG_NONE;
                status = api->ConfigurationLoadCredential(configuration, &cred);
                if (QUIC_SUCCEEDED(status)) {
                    cred_loaded = true;
                    memolog::LogInfo("quic.credential.loaded",
                        "ChatServer QUIC credential loaded via CERT_CONTEXT",
                        std::map<std::string, std::string>{{"thumbprint", cert_thumbprint}});
                }
                CertFreeCertificateContext(found_ctx);
            }
            CertCloseStore(hStore, 0);
        }
    }

    // Approach 2: Try CERTIFICATE_FILE_PROTECTED (cert + key with password)
    if (!cred_loaded && !cert_file.empty() && !key_file.empty()) {
        std::string pwd_override = ReadConfigOrDefault("Quic", "KeyPassword", pfx_password.empty() ? "" : pfx_password.c_str());
        if (!pwd_override.empty()) {
            QUIC_CERTIFICATE_FILE_PROTECTED cert{};
            cert.CertificateFile = cert_file.c_str();
            cert.PrivateKeyFile = key_file.c_str();
            cert.PrivateKeyPassword = pwd_override.c_str();
            QUIC_CREDENTIAL_CONFIG cred{};
            cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE_PROTECTED;
            cred.CertificateFileProtected = &cert;
            cred.Flags = QUIC_CREDENTIAL_FLAG_NONE;
            status = api->ConfigurationLoadCredential(configuration, &cred);
            if (QUIC_SUCCEEDED(status)) {
                cred_loaded = true;
                memolog::LogInfo("quic.credential.loaded",
                    "ChatServer QUIC credential loaded via FILE_PROTECTED",
                    std::map<std::string, std::string>{{"cert_file", cert_file}});
            }
        }
    }

    // Approach 3: Try CERTIFICATE_FILE (cert + key, no password)
    if (!cred_loaded && !cert_file.empty() && !key_file.empty() &&
        std::filesystem::exists(cert_file) && std::filesystem::exists(key_file)) {
        QUIC_CERTIFICATE_FILE cert{};
        cert.CertificateFile = cert_file.c_str();
        cert.PrivateKeyFile = key_file.c_str();
        QUIC_CREDENTIAL_CONFIG cred{};
        cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
        cred.CertificateFile = &cert;
        cred.Flags = QUIC_CREDENTIAL_FLAG_NONE;
        status = api->ConfigurationLoadCredential(configuration, &cred);
        if (QUIC_SUCCEEDED(status)) {
            cred_loaded = true;
            memolog::LogInfo("quic.credential.loaded",
                "ChatServer QUIC credential loaded via CERTIFICATE_FILE",
                std::map<std::string, std::string>{{"cert_file", cert_file}});
        } else {
            if (error) {
                *error = "credential_load_failed";
            }
            shutdownHandles();
            return false;
        }
    }

    // Approach 4: Try CERTIFICATE_PKCS12 (PFX blob with password) — mirrors GateServer fallback
    if (!cred_loaded && !pfx_file.empty() && std::filesystem::exists(pfx_file)) {
        std::ifstream pfx_stream(pfx_file, std::ios::binary);
        if (pfx_stream) {
            std::vector<uint8_t> pfx_data(
                (std::istreambuf_iterator<char>(pfx_stream)),
                std::istreambuf_iterator<char>());
            pfx_stream.close();
            if (!pfx_data.empty()) {
                std::string pwd = pfx_password.empty() ? "memochat" : pfx_password;
                QUIC_CERTIFICATE_PKCS12 pkcs12{};
                pkcs12.Asn1Blob = pfx_data.data();
                pkcs12.Asn1BlobLength = static_cast<uint32_t>(pfx_data.size());
                pkcs12.PrivateKeyPassword = const_cast<char*>(pwd.c_str());
                QUIC_CREDENTIAL_CONFIG cred{};
                cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_PKCS12;
                cred.Flags = QUIC_CREDENTIAL_FLAG_NONE;
                cred.CertificatePkcs12 = &pkcs12;
                status = api->ConfigurationLoadCredential(configuration, &cred);
                if (QUIC_SUCCEEDED(status)) {
                    cred_loaded = true;
                    memolog::LogInfo("quic.credential.loaded",
                        "ChatServer QUIC credential loaded via PKCS12 blob",
                        std::map<std::string, std::string>{{"pfx_file", pfx_file}});
                }
            }
        }
    }

    if (!cred_loaded) {
        if (error) {
            *error = "quic_certificate_missing";
        }
        shutdownHandles();
        return false;
    }

    return true;
}
#endif

#if MEMOCHAT_ENABLE_MSQUIC
void QuicChatServer::Impl::shutdownHandles()
{
    if (listener != nullptr && api != nullptr) {
        api->ListenerStop(listener);
        api->ListenerClose(listener);
        listener = nullptr;
    }
    if (configuration != nullptr && api != nullptr) {
        api->ConfigurationClose(configuration);
        configuration = nullptr;
    }
    if (registration != nullptr && api != nullptr) {
        api->RegistrationClose(registration);
        registration = nullptr;
    }
    if (api != nullptr) {
        MsQuicClose(api);
        api = nullptr;
    }
}
#endif

#if MEMOCHAT_ENABLE_MSQUIC
void QuicChatServer::Impl::closeConnectionHandles(Impl* impl, ConnectionContext* ctx)
{
    if (ctx == nullptr || impl == nullptr || impl->api == nullptr) {
        return;
    }
    if (ctx->stream != nullptr) {
        impl->api->StreamClose(ctx->stream);
        ctx->stream = nullptr;
    }
    if (ctx->handle != nullptr) {
        impl->api->ConnectionClose(ctx->handle);
        ctx->handle = nullptr;
    }
}
#endif

#if MEMOCHAT_ENABLE_MSQUIC
void QuicChatServer::Impl::dispatchFrames(StreamContext* stream_context)
{
    if (stream_context == nullptr || stream_context->connection == nullptr || !stream_context->connection->session) {
        return;
    }

    auto& buffer = stream_context->recv_buffer;
    while (true) {
        if (!stream_context->recv_pending) {
            if (buffer.size() < HEAD_TOTAL_LEN) {
                return;
            }

            short msg_id = 0;
            short msg_len = 0;
            std::memcpy(&msg_id, buffer.data(), HEAD_ID_LEN);
            std::memcpy(&msg_len, buffer.data() + HEAD_ID_LEN, HEAD_DATA_LEN);
            msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
            msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);

            if (msg_len < 0 || msg_len > MAX_LENGTH) {
                stream_context->connection->session->Close();
                return;
            }

            buffer.erase(buffer.begin(), buffer.begin() + HEAD_TOTAL_LEN);
            stream_context->recv_pending = true;
            stream_context->msg_id = msg_id;
            stream_context->msg_len = msg_len;
        }

        if (static_cast<short>(buffer.size()) < stream_context->msg_len) {
            return;
        }

        std::string payload(buffer.begin(), buffer.begin() + stream_context->msg_len);
        buffer.erase(buffer.begin(), buffer.begin() + stream_context->msg_len);
        stream_context->recv_pending = false;
        stream_context->connection->session->HandleInboundMessage(stream_context->msg_id, payload);
    }
}
#endif

#if MEMOCHAT_ENABLE_MSQUIC
QUIC_STATUS QUIC_API QuicChatServer::Impl::ListenerCallback(HQUIC, void* context, QUIC_LISTENER_EVENT* event)
{
    auto* owner = static_cast<QuicChatServer*>(context);
    if (owner == nullptr || owner->_impl == nullptr || event == nullptr) {
        return QUIC_STATUS_INVALID_STATE;
    }

    if (event->Type != QUIC_LISTENER_EVENT_NEW_CONNECTION) {
        return QUIC_STATUS_SUCCESS;
    }

    auto* impl = owner->_impl.get();
    auto* connection_context = new ConnectionContext();
    connection_context->owner = owner;
    connection_context->handle = event->NEW_CONNECTION.Connection;
    connection_context->api = impl->api;
    connection_context->session = std::make_shared<QuicSession>(
        owner->_io_context,
        [connection_context](const std::string& payload, short msgid) -> bool {
            if (connection_context->stream == nullptr || connection_context->api == nullptr) {
                return false;
            }
            auto* ctx = connection_context->stream_context.get();
            if (ctx == nullptr) {
                return false;
            }

            QUIC_SEND_FLAGS flags = QUIC_SEND_FLAG_NONE;
            if (!ctx->start_complete) {
                ctx->pending_sends.emplace_back(msgid, payload);
                return true;
            }

            auto* send_ctx = new Impl::SendContext();
            send_ctx->bytes = EncodeFrame(msgid, payload);
            send_ctx->buffer.Buffer = send_ctx->bytes.data();
            send_ctx->buffer.Length = static_cast<uint32_t>(send_ctx->bytes.size());

            const QUIC_STATUS send_status = connection_context->api->StreamSend(
                connection_context->stream,
                &send_ctx->buffer,
                1,
                flags,
                send_ctx);
            if (QUIC_FAILED(send_status)) {
                delete send_ctx;
                return false;
            }
            return true;
        },
        [owner](const std::string& session_id) {
            owner->cleanupClosedSession(session_id);
        });

    {
        std::lock_guard<std::mutex> lock(owner->_session_mutex);
        owner->_sessions.emplace(connection_context->session->GetSessionId(), connection_context->session);
    }

    impl->api->SetCallbackHandler(event->NEW_CONNECTION.Connection,
        reinterpret_cast<void*>(ConnectionCallback),
        connection_context);

    const QUIC_STATUS status = impl->api->ConnectionSetConfiguration(
        event->NEW_CONNECTION.Connection,
        impl->configuration);
    if (QUIC_FAILED(status)) {
        owner->cleanupClosedSession(connection_context->session->GetSessionId());
        delete connection_context;
        return status;
    }

    return QUIC_STATUS_SUCCESS;
}
#endif

#if MEMOCHAT_ENABLE_MSQUIC
QUIC_STATUS QUIC_API QuicChatServer::Impl::ConnectionCallback(HQUIC connection, void* context, QUIC_CONNECTION_EVENT* event)
{
    auto* ctx = static_cast<ConnectionContext*>(context);
    if (ctx == nullptr || ctx->owner == nullptr || ctx->owner->_impl == nullptr || event == nullptr) {
        return QUIC_STATUS_INVALID_STATE;
    }

    auto* impl = ctx->owner->_impl.get();
    switch (event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED:
        memolog::LogInfo("quic.connection.connected", "ChatServer QUIC connection established",
            {{"session_id", ctx->session ? ctx->session->GetSessionId() : ""}});
        return QUIC_STATUS_SUCCESS;
    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
        ctx->stream = event->PEER_STREAM_STARTED.Stream;
        ctx->stream_context = std::make_unique<StreamContext>();
        ctx->stream_context->connection = ctx;
        impl->api->SetCallbackHandler(
            ctx->stream,
            reinterpret_cast<void*>(StreamCallback),
            ctx->stream_context.get());
        impl->api->StreamStart(ctx->stream, QUIC_STREAM_START_FLAG_IMMEDIATE);
        return QUIC_STATUS_SUCCESS;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        if (ctx->session) {
            ctx->session->Close();
        }
        return QUIC_STATUS_SUCCESS;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        if (ctx->session) {
            ctx->owner->cleanupClosedSession(ctx->session->GetSessionId());
        }
        closeConnectionHandles(impl, ctx);
        delete ctx;
        return QUIC_STATUS_SUCCESS;
    default:
        return QUIC_STATUS_SUCCESS;
    }
}
#endif

#if MEMOCHAT_ENABLE_MSQUIC
QUIC_STATUS QUIC_API QuicChatServer::Impl::StreamCallback(HQUIC stream, void* context, QUIC_STREAM_EVENT* event)
{
    auto* stream_context = static_cast<StreamContext*>(context);
    if (stream_context == nullptr || event == nullptr) {
        return QUIC_STATUS_INVALID_STATE;
    }

    switch (event->Type) {
    case QUIC_STREAM_EVENT_START_COMPLETE:
        stream_context->start_complete = true;
        if (stream_context->connection != nullptr && stream_context->connection->api != nullptr) {
            for (auto& [msgid, payload] : stream_context->pending_sends) {
                auto* send_ctx = new Impl::SendContext();
                send_ctx->bytes = EncodeFrame(msgid, payload);
                send_ctx->buffer.Buffer = send_ctx->bytes.data();
                send_ctx->buffer.Length = static_cast<uint32_t>(send_ctx->bytes.size());
                stream_context->connection->api->StreamSend(
                    stream_context->connection->stream,
                    &send_ctx->buffer,
                    1,
                    QUIC_SEND_FLAG_NONE,
                    send_ctx);
            }
            stream_context->pending_sends.clear();
        }
        return QUIC_STATUS_SUCCESS;
    case QUIC_STREAM_EVENT_RECEIVE:
        for (uint32_t i = 0; i < event->RECEIVE.BufferCount; ++i) {
            const auto& one = event->RECEIVE.Buffers[i];
            stream_context->recv_buffer.insert(
                stream_context->recv_buffer.end(),
                one.Buffer,
                one.Buffer + one.Length);
        }
        dispatchFrames(stream_context);
        return QUIC_STATUS_SUCCESS;
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        delete static_cast<SendContext*>(event->SEND_COMPLETE.ClientContext);
        return QUIC_STATUS_SUCCESS;
    case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        if (stream_context->connection != nullptr && stream_context->connection->session) {
            stream_context->connection->session->Close();
        }
        return QUIC_STATUS_SUCCESS;
    default:
        return QUIC_STATUS_SUCCESS;
    }
}
#endif

QuicChatServer::QuicChatServer(boost::asio::io_context& io_context)
    : _io_context(io_context)
{
#if MEMOCHAT_ENABLE_MSQUIC
    _impl = std::make_unique<Impl>();
#endif
}

QuicChatServer::~QuicChatServer()
{
    Stop();
#if MEMOCHAT_ENABLE_MSQUIC
    _impl.reset();
#endif
}

bool QuicChatServer::Start(const std::string& host, const std::string& port, std::string* error)
{
    _host = host;
    _port = port;
    if (_host.empty() || _port.empty()) {
        if (error) {
            *error = "quic_endpoint_missing";
        }
        return false;
    }

#if MEMOCHAT_ENABLE_MSQUIC
    if (_impl == nullptr) {
        _impl = std::make_unique<Impl>();
    }
    if (!_impl->ensureInitialized(this, error)) {
        memolog::LogWarn("quic.ingress.init_failed",
            "ChatServer QUIC ingress initialization failed",
            {{"host", _host}, {"port", _port}, {"error", error ? *error : "unknown"}});
        return false;
    }

    QUIC_STATUS status = _impl->api->ListenerOpen(_impl->registration, &Impl::ListenerCallback, this, &_impl->listener);
    if (QUIC_FAILED(status)) {
        if (error) {
            *error = "listener_open_failed";
        }
        _impl->shutdownHandles();
        return false;
    }

    QUIC_ADDR addr{};
    QuicAddrSetFamily(&addr, QUIC_ADDRESS_FAMILY_UNSPEC);
    QuicAddrSetPort(&addr, static_cast<uint16_t>(std::atoi(_port.c_str())));
    status = _impl->api->ListenerStart(_impl->listener, &_impl->alpn_buffer, 1, &addr);
    if (QUIC_FAILED(status)) {
        if (error) {
            *error = "listener_start_failed";
        }
        _impl->shutdownHandles();
        return false;
    }

    memolog::LogInfo("quic.ingress.start",
        "ChatServer QUIC ingress started",
        {{"host", _host}, {"port", _port}, {"alpn", _impl->alpn}});
    _running = true;
    return true;
#else
    if (error) {
        *error = "msquic_not_available";
    }
    memolog::LogWarn("quic.ingress.unavailable",
        "ChatServer QUIC ingress disabled because msquic is unavailable",
        {{"host", _host}, {"port", _port}});
    return false;
#endif
}

void QuicChatServer::Stop()
{
#if MEMOCHAT_ENABLE_MSQUIC
    if (!_running && _impl == nullptr) {
        return;
    }
    closeAllSessions();
    if (_impl != nullptr) {
        _impl->shutdownHandles();
    }
#else
    if (!_running) {
        return;
    }
    closeAllSessions();
#endif
    _running = false;
#if MEMOCHAT_ENABLE_MSQUIC
    memolog::LogInfo("quic.ingress.stop",
        "ChatServer QUIC ingress stopped",
        {{"host", _host}, {"port", _port}});
#endif
}

bool QuicChatServer::isRunning() const
{
#if MEMOCHAT_ENABLE_MSQUIC
    return _running;
#else
    return false;
#endif
}

std::string QuicChatServer::listenHost() const
{
    return _host;
}

std::string QuicChatServer::listenPort() const
{
    return _port;
}

bool QuicChatServer::cleanupClosedSession(const std::string& session_id)
{
    std::shared_ptr<QuicSession> session;
    {
        std::lock_guard<std::mutex> lock(_session_mutex);
        const auto it = _sessions.find(session_id);
        if (it == _sessions.end()) {
            return false;
        }
        session = it->second;
        _sessions.erase(it);
    }

#if MEMOCHAT_ENABLE_MSQUIC
    (void)session;
#endif
    return true;
}

void QuicChatServer::closeAllSessions()
{
    std::vector<std::shared_ptr<QuicSession>> sessions;
    {
        std::lock_guard<std::mutex> lock(_session_mutex);
        for (auto& [_, session] : _sessions) {
            sessions.push_back(session);
        }
        _sessions.clear();
    }
    for (const auto& session : sessions) {
        if (session) {
            session->Close();
        }
    }
}

} // namespace memochat::chatserver
