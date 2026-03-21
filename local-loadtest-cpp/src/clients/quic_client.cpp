#include "client.hpp"
#include "quic_lib.hpp"

#if defined(MEMOCHAT_ENABLE_MSQUIC)
#include <msquic.h>
#endif

#include <memory>
#include <vector>
#include <chrono>
#include <atomic>
#include <cstring>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
// Undef Windows min/max macros before STL usage
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

namespace memochat {
namespace loadtest {

#if !defined(MEMOCHAT_ENABLE_MSQUIC)
// Stub implementation when msquic is not available
QuicChatClient::QuicChatClient(const std::string&, int,
                                const std::string&,
                                const std::string&,
                                double) {}
QuicChatClient::~QuicChatClient() = default;
bool QuicChatClient::connect() { last_error_ = "msquic not available"; return false; }
bool QuicChatClient::send_chat_login(
    const std::unordered_map<std::string, std::string>&,
    const std::string&, int) { return false; }
int QuicChatClient::wait_chat_login_response(double) { return -1; }
void QuicChatClient::close() {}
const std::string& QuicChatClient::last_error() const { return last_error_; }

#else // MEMOCHAT_ENABLE_MSQUIC

// Cast QUIC_API_TABLE* from opaque void*
static inline const QUIC_API_TABLE* to_api(const void* p) {
    return static_cast<const QUIC_API_TABLE*>(p);
}

// ---- QUIC Chat Client (msquic v2 API) ----
struct QuicChatClient::Impl {
    std::string host_;
    int port_ = 0;
    std::string server_name_;
    std::string alpn_;
    double timeout_sec_ = 5.0;
    std::string last_error_;

    // msquic handles
    HQUIC Configuration = nullptr;

    // Per-connection state
    HQUIC Connection = nullptr;
    HQUIC Stream = nullptr;

    std::atomic<bool> connected_{false};
    std::atomic<bool> response_ready_{false};
    std::atomic<bool> stream_closed_{false};
    std::string response_body_;
    int response_error_code_ = -1;

    explicit Impl(const std::string& host, int port,
                 const std::string& server_name,
                 const std::string& alpn,
                 double timeout_sec)
        : host_(host), port_(port), server_name_(server_name),
          alpn_(alpn), timeout_sec_(timeout_sec) {}

    ~Impl() { close(); }

    const QUIC_API_TABLE* api() const {
        return to_api(QuicLibrary::instance().api());
    }

    bool create_configuration(const QUIC_API_TABLE* api, HQUIC registration) {
        QUIC_BUFFER alpnBuffer{};
        alpnBuffer.Buffer = (uint8_t*)alpn_.c_str();
        alpnBuffer.Length = static_cast<uint32_t>(alpn_.size());

        QUIC_SETTINGS settings{};
        settings.IdleTimeoutMs = 30000;
        settings.IsSet.IdleTimeoutMs = TRUE;
        settings.PeerBidiStreamCount = 1;
        settings.IsSet.PeerBidiStreamCount = TRUE;

        QUIC_STATUS status = api->ConfigurationOpen(
            registration, &alpnBuffer, 1, &settings, sizeof(settings), nullptr, &Configuration);
        if (!QUIC_SUCCEEDED(status)) {
            last_error_ = "ConfigurationOpen failed: " + std::to_string(status);
            return false;
        }
        return true;
    }

    bool connect() {
        QuicLibrary& lib = QuicLibrary::instance();
        if (!lib.is_ready()) {
            if (!lib.init()) {
                last_error_ = "QuicLibrary::init failed: " + lib.last_error();
                return false;
            }
        }

        const QUIC_API_TABLE* api = this->api();
        if (!api) {
            last_error_ = "QuicLibrary api is null";
            return false;
        }

        // Create configuration (needed for ConnectionStart in v2)
        if (!create_configuration(api, lib.registration())) {
            return false;
        }

        // Connection callback
        QUIC_CONNECTION_CALLBACK_HANDLER conn_cb = [](HQUIC conn, void* ctx,
                                                      QUIC_CONNECTION_EVENT* ev) -> QUIC_STATUS {
            Impl* self = static_cast<Impl*>(ctx);
            (void)conn;
            switch (ev->Type) {
                case QUIC_CONNECTION_EVENT_CONNECTED:
                    self->connected_.store(true);
                    return QUIC_STATUS_SUCCESS;
                case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
                    const QUIC_API_TABLE* api_cb = self->api();
                    if (!api_cb) return QUIC_STATUS_INVALID_PARAMETER;
                    self->Stream = ev->PEER_STREAM_STARTED.Stream;
                    api_cb->SetCallbackHandler(self->Stream,
                        reinterpret_cast<void*>(stream_callback), self);
                    return QUIC_STATUS_SUCCESS;
                }
                case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
                case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
                    self->response_ready_.store(true);
                    return QUIC_STATUS_SUCCESS;
                default:
                    return QUIC_STATUS_SUCCESS;
            }
        };

        QUIC_STATUS status = api->ConnectionOpen(
            lib.registration(), conn_cb, this, &Connection);
        if (!QUIC_SUCCEEDED(status)) {
            last_error_ = "ConnectionOpen failed: " + std::to_string(status);
            return false;
        }

        status = api->ConnectionStart(
            Connection, Configuration, AF_INET, server_name_.c_str(), (uint16_t)port_);
        if (!QUIC_SUCCEEDED(status)) {
            last_error_ = "ConnectionStart failed: " + std::to_string(status);
            api->ConnectionClose(Connection);
            Connection = nullptr;
            return false;
        }

        // Wait for handshake completion
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::milliseconds(int(timeout_sec_ * 1000));
        while (!connected_.load() && std::chrono::steady_clock::now() < deadline) {
            Sleep(1);
        }

        if (!connected_.load()) {
            last_error_ = "QUIC handshake timeout";
            api->ConnectionClose(Connection);
            Connection = nullptr;
            return false;
        }

        return true;
    }

    static QUIC_STATUS QUIC_API stream_callback(HQUIC stream, void* ctx,
                                                 QUIC_STREAM_EVENT* ev) {
        Impl* self = static_cast<Impl*>(ctx);
        (void)stream;
        switch (ev->Type) {
            case QUIC_STREAM_EVENT_RECEIVE: {
                const QUIC_BUFFER* buf = ev->RECEIVE.Buffers;
                uint32_t count = ev->RECEIVE.BufferCount;
                for (uint32_t i = 0; i < count; ++i) {
                    if (buf[i].Buffer && buf[i].Length > 0) {
                        self->on_stream_data(buf[i].Buffer, buf[i].Length);
                    }
                }
                return QUIC_STATUS_SUCCESS;
            }
            case QUIC_STREAM_EVENT_SEND_COMPLETE:
                return QUIC_STATUS_SUCCESS;
            case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
                self->stream_closed_.store(true);
                return QUIC_STATUS_SUCCESS;
            case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
                self->stream_closed_.store(true);
                return QUIC_STATUS_SUCCESS;
            case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
                self->response_ready_.store(true);
                return QUIC_STATUS_SUCCESS;
            default:
                return QUIC_STATUS_SUCCESS;
        }
    }

    void on_stream_data(const void* data, uint32_t len) {
        response_body_.append(static_cast<const char*>(data), len);
        parse_response();
    }

    void parse_response() {
        // Frame: uint16_t msg_id, uint16_t body_len, body
        while (response_body_.size() >= 4) {
            uint16_t msg_id = (static_cast<uint8_t>(response_body_[0]) << 8)
                            | static_cast<uint8_t>(response_body_[1]);
            uint16_t body_len = (static_cast<uint8_t>(response_body_[2]) << 8)
                              | static_cast<uint8_t>(response_body_[3]);

            if (response_body_.size() < 4 + body_len) break;

            std::string body = response_body_.substr(4, body_len);
            response_body_.erase(0, 4 + body_len);

            if (msg_id == 1006) { // CHAT_LOGIN_RSP
                auto pos = body.find("\"error\"");
                if (pos != std::string::npos) {
                    size_t colon = body.find(':', pos);
                    if (colon != std::string::npos) {
                        size_t end = colon + 1;
                        while (end < body.size()
                               && (body[end] == ' ' || body[end] == '\t')) ++end;
                        size_t comma = body.find(',', end);
                        size_t brace = body.find('}', end);
                        size_t tok_end = std::min(comma, brace);
                        std::string token = body.substr(end, tok_end - end);
                        try {
                            response_error_code_ = std::stoi(token);
                        } catch (...) {
                            response_error_code_ = -1;
                        }
                    }
                }
                response_ready_.store(true);
            }
        }
    }

    bool send_chat_login_frame(const std::unordered_map<std::string, std::string>& gate_response,
                                const std::string& trace_id,
                                int protocol_version) {
        if (!Connection || !Stream) return false;

        std::ostringstream ss;
        ss << "{\"protocol_version\":" << protocol_version
           << ",\"trace_id\":\"" << json_escape(trace_id) << "\"";
        if (protocol_version >= 3) {
            auto it = gate_response.find("login_ticket");
            if (it != gate_response.end()) {
                ss << ",\"login_ticket\":\"" << json_escape(it->second) << "\"";
            }
        } else {
            auto uid_it = gate_response.find("uid");
            auto token_it = gate_response.find("token");
            if (uid_it != gate_response.end() && token_it != gate_response.end()) {
                ss << ",\"uid\":" << uid_it->second
                   << ",\"token\":\"" << json_escape(token_it->second) << "\"";
            }
        }
        ss << "}";

        std::string payload = ss.str();

        // Frame header: msg_id(2 bytes) + body_len(2 bytes) + body
        std::vector<uint8_t> frame;
        uint16_t msg_id = 1005;
        uint16_t body_len = static_cast<uint16_t>(payload.size());
        frame.push_back(static_cast<uint8_t>(msg_id >> 8));
        frame.push_back(static_cast<uint8_t>(msg_id & 0xFF));
        frame.push_back(static_cast<uint8_t>(body_len >> 8));
        frame.push_back(static_cast<uint8_t>(body_len & 0xFF));
        frame.insert(frame.end(), payload.begin(), payload.end());

        QUIC_BUFFER buf{};
        buf.Buffer = frame.data();
        buf.Length = static_cast<uint32_t>(frame.size());

        const QUIC_API_TABLE* api = this->api();
        if (!api) return false;

        QUIC_STATUS status = api->StreamSend(Stream, &buf, 1, QUIC_SEND_FLAG_FIN, nullptr);
        if (!QUIC_SUCCEEDED(status)) {
            last_error_ = "StreamSend failed: " + std::to_string(status);
            return false;
        }
        return true;
    }

    int wait_response() {
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::milliseconds(int(timeout_sec_ * 1000));
        while (!response_ready_.load()
               && std::chrono::steady_clock::now() < deadline) {
            Sleep(1);
        }
        return response_error_code_;
    }

    void close() {
        const QUIC_API_TABLE* api = this->api();
        if (Stream && api) {
            api->StreamClose(Stream);
            Stream = nullptr;
        }
        if (Connection && api) {
            api->ConnectionClose(Connection);
            Connection = nullptr;
        }
        if (Configuration && api) {
            api->ConfigurationClose(Configuration);
            Configuration = nullptr;
        }
    }
};

// ---- Public API ----
QuicChatClient::QuicChatClient(const std::string& host, int port,
                                const std::string& server_name,
                                const std::string& alpn,
                                double timeout_sec)
    : impl_(std::make_unique<Impl>(host, port, server_name, alpn, timeout_sec)) {}

QuicChatClient::~QuicChatClient() = default;

bool QuicChatClient::connect() { return impl_->connect(); }

bool QuicChatClient::send_chat_login(
    const std::unordered_map<std::string, std::string>& gate_response,
    const std::string& trace_id,
    int protocol_version) {
    return impl_->send_chat_login_frame(gate_response, trace_id, protocol_version);
}

int QuicChatClient::wait_chat_login_response(double /*timeout_sec*/) {
    return impl_->wait_response();
}

void QuicChatClient::close() { impl_->close(); }

const std::string& QuicChatClient::last_error() const { return impl_->last_error_; }

#endif // MEMOCHAT_ENABLE_MSQUIC

} // namespace loadtest
} // namespace memochat
