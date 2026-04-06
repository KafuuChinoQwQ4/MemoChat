#ifdef MEMOCHAT_HAVE_NGHTTP2

#include "WinCompat.h"
#include "NgHttp2Server.h"
#include "Http2Routes.h"
#include "Http2Handlers.h"
#include "Http2AuthHandlers.h"
#include "Http2CallHandlers.h"
#include "Http2MediaHandlers.h"
#include "Http2ProfileHandlers.h"
#include "Http2MomentsSupport.h"
#include "CertUtil.h"
#include "logging/Logger.h"

// Define NGHTTP2_STATICLIB before including nghttp2.h so it produces extern
// declarations without any DLL import/export annotations (no __declspec).
#define NGHTTP2_STATICLIB

// MSVC on Windows x64: define ssize_t before nghttp2.h sees it.
// nghttp2.h uses ssize_t in callback signatures. The system <sys/types.h>
// may not expose it correctly in all include-order situations with Windows SDK.
#include <sys/types.h>
#if defined(_MSC_VER) && !defined(_SSIZE_T_DEFINED) && !defined(__ssize_t_defined)
typedef ptrdiff_t ssize_t;
#define _SSIZE_T_DEFINED
#define __ssize_t_defined
#endif

#include <nghttp2/nghttp2.h>

#include <boost/asio.hpp>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/tls1.h>

#include <iostream>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
#include <functional>
#include <filesystem>

namespace net = boost::asio;
using tcp = net::ip::tcp;

// ── Constants ─────────────────────────────────────────────────────────────────

static constexpr int DEFAULT_H2_PORT = 8444;

// ── Global server state ───────────────────────────────────────────────────────

static net::io_context* g_ioc = nullptr;
static SSL_CTX* g_raw_ssl_ctx = nullptr;
static tcp::acceptor* g_acceptor = nullptr;
static std::atomic<bool> g_running{true};
static bool g_initialized = false;

// ── Data provider registry for nghttp2 DATA frames ──────────────────────────────
// nghttp2's data_provider needs a plain function pointer, but we need connection state.
// Store pending body shared_ptr keyed by stream_id, retrieved in the static callback.
struct BodyBuf { std::vector<uint8_t> data; };
static std::map<int32_t, std::shared_ptr<BodyBuf>> g_body_map;
static std::mutex g_body_mutex;

static nghttp2_ssize StaticDataCallback(
    nghttp2_session*, int32_t stream_id,
    uint8_t* buf, size_t length,
    uint32_t* data_flags, nghttp2_data_source*, void*) {
    try {
        std::shared_ptr<BodyBuf> body;
        {
            std::lock_guard<std::mutex> lock(g_body_mutex);
            auto it = g_body_map.find(stream_id);
            if (it != g_body_map.end()) {
                body = it->second;
            }
        }
        if (!body || body->data.empty()) {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            return 0;
        }
        size_t avail = body->data.size();
        size_t n = std::min(avail, length);
        std::memcpy(buf, body->data.data(), n);
        body->data.erase(body->data.begin(), body->data.begin() + n);
        if (body->data.empty()) {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        }
        return static_cast<nghttp2_ssize>(n);
    } catch (...) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
}

// ── Forward declaration ───────────────────────────────────────────────────────

class NgHttp2Connection;

// ── nghttp2 callbacks (all static, user_data is NgHttp2Connection*) ────────────

static int cb_header(nghttp2_session*, const nghttp2_frame* frame,
                     const uint8_t* name, size_t namelen,
                     const uint8_t* value, size_t valuelen,
                     uint8_t, void* user_data);

static int cb_begin_headers(nghttp2_session*, const nghttp2_frame* frame, void* user_data);

static int cb_frame_recv(nghttp2_session*, const nghttp2_frame* frame, void* user_data);

static int cb_data_chunk_recv(nghttp2_session*, uint8_t, int32_t stream_id,
                             const uint8_t* data, size_t len, void* user_data);

static int cb_stream_close(nghttp2_session*, int32_t stream_id,
                           uint32_t, void* user_data);

static ssize_t cb_send(nghttp2_session*, const uint8_t* data,
                       size_t length, int, void* user_data);

// ── NgHttp2Connection ─────────────────────────────────────────────────────────

class NgHttp2Connection : public std::enable_shared_from_this<NgHttp2Connection> {
public:
    NgHttp2Connection(net::io_context& ioc, SSL_CTX* ssl_ctx, tcp::socket socket);
    ~NgHttp2Connection();

    void Start();
    void Close();

    // Called by nghttp2 send callback
    int SendToPeer(const uint8_t* data, size_t len);

    void OnBeginHeaders(int32_t stream_id);
    void OnHeader(int32_t stream_id, const std::string& key, const std::string& value);
    void OnBodyData(int32_t stream_id, const uint8_t* data, size_t len);
    void OnRequestComplete(int32_t stream_id);
    void OnStreamClose(int32_t stream_id, uint32_t error_code);

    nghttp2_session* session() { return session_; }

private:
    void HandleSslRead(boost::system::error_code ec, std::size_t bytes);
    void ProcessSslBuffer();
    void DrainWriteBio();
    void StartHttp2Session();
    void StartAsyncRead();
    void HandleRead(boost::system::error_code ec, std::size_t bytes);
    void AsyncWrite();
    void HandleWrite(boost::system::error_code ec, std::size_t bytes);

    net::io_context& ioc_;
    tcp::socket socket_;
    SSL* ssl_ = nullptr;

    nghttp2_session* session_ = nullptr;

    boost::asio::streambuf read_buffer_{8192};

    std::vector<uint8_t> write_buf_;
    std::mutex write_mutex_;
    bool writing_ = false;
    bool closed_ = false;
    bool http2_mode_ = false;

    // Per-stream state
    Http2Request current_req_;
    std::vector<uint8_t> body_buf_;
    int32_t current_stream_id_ = -1;
    // Shared ownership of body buffer with nghttp2's send path.
    // nghttp2 reads it via StaticDataCallback (registered in g_body_map),
    // and we release it when OnStreamClose fires.
    std::shared_ptr<BodyBuf> pending_body_;
};

// ── nghttp2 callback implementations ───────────────────────────────────────────

static int cb_header(nghttp2_session*, const nghttp2_frame* frame,
                     const uint8_t* name, size_t namelen,
                     const uint8_t* value, size_t valuelen,
                     uint8_t, void* user_data) {
    try {
        auto* conn = static_cast<NgHttp2Connection*>(user_data);
        if (frame->hd.type != NGHTTP2_HEADERS) return 0;
        std::string key(reinterpret_cast<const char*>(name), namelen);
        std::string val(reinterpret_cast<const char*>(value), valuelen);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        conn->OnHeader(frame->hd.stream_id, key, val);
    } catch (...) {
        // suppress — do not let exceptions propagate through nghttp2 C callbacks
    }
    return 0;
}

static int cb_begin_headers(nghttp2_session*, const nghttp2_frame* frame, void* user_data) {
    try {
        auto* conn = static_cast<NgHttp2Connection*>(user_data);
        if (frame->hd.type == NGHTTP2_HEADERS) {
            conn->OnBeginHeaders(frame->hd.stream_id);
        }
    } catch (...) {}
    return 0;
}

static int cb_frame_recv(nghttp2_session*, const nghttp2_frame* frame, void* user_data) {
    try {
        auto* conn = static_cast<NgHttp2Connection*>(user_data);
        if (frame->hd.type == NGHTTP2_HEADERS) {
            conn->OnRequestComplete(frame->hd.stream_id);
        } else if (frame->hd.type == NGHTTP2_SETTINGS) {
            if ((frame->settings.hd.flags & NGHTTP2_FLAG_ACK) == 0) {
                nghttp2_session_send(conn->session());
            }
        } else if (frame->hd.type == NGHTTP2_PING) {
            if ((frame->ping.hd.flags & NGHTTP2_FLAG_ACK) == 0) {
                uint8_t opaque[8];
                std::memcpy(opaque, frame->ping.opaque_data, 8);
                nghttp2_submit_ping(conn->session(), NGHTTP2_FLAG_ACK, opaque);
                nghttp2_session_send(conn->session());
            }
        }
    } catch (...) {}
    return 0;
}

static int cb_data_chunk_recv(nghttp2_session*, uint8_t,
                               int32_t stream_id, const uint8_t* data,
                               size_t len, void* user_data) {
    try {
        auto* conn = static_cast<NgHttp2Connection*>(user_data);
        conn->OnBodyData(stream_id, data, len);
    } catch (...) {}
    return 0;
}

static int cb_stream_close(nghttp2_session*, int32_t stream_id,
                           uint32_t, void* user_data) {
    try {
        auto* conn = static_cast<NgHttp2Connection*>(user_data);
        conn->OnStreamClose(stream_id, 0);
    } catch (...) {}
    return 0;
}

static ssize_t cb_send(nghttp2_session*, const uint8_t* data,
                       size_t length, int, void* user_data) {
    auto* conn = static_cast<NgHttp2Connection*>(user_data);
    return conn->SendToPeer(data, length);
}

// ── NgHttp2Connection methods ─────────────────────────────────────────────────

NgHttp2Connection::NgHttp2Connection(net::io_context& ioc, SSL_CTX* ssl_ctx, tcp::socket socket)
    : ioc_(ioc), socket_(std::move(socket)), ssl_(SSL_new(ssl_ctx)) {
    SSL_set_app_data(ssl_, this);
}

NgHttp2Connection::~NgHttp2Connection() {
    Close();
    if (session_) {
        nghttp2_session_del(session_);
        session_ = nullptr;
    }
}

void NgHttp2Connection::Start() {
    auto self = shared_from_this();
    BIO* bio = SSL_get_rbio(ssl_);
    BIO* wbio = SSL_get_wbio(ssl_);
    if (!bio || !wbio) {
        memolog::LogWarn("nghttp2.ssl.bio.fail", "SSL BIO handles unavailable");
        Close();
        return;
    }
    BIO_set_fd(bio, static_cast<int>(socket_.native_handle()), BIO_NOCLOSE);
    BIO_set_fd(wbio, static_cast<int>(socket_.native_handle()), BIO_NOCLOSE);

    SSL_set_accept_state(ssl_);

    net::async_read(socket_, read_buffer_.prepare(8192),
        [self](boost::system::error_code ec, std::size_t bytes) {
            self->HandleSslRead(ec, bytes);
        });
}

void NgHttp2Connection::Close() {
    std::lock_guard<std::mutex> lock(write_mutex_);
    if (closed_) return;
    closed_ = true;
    boost::system::error_code ec;
    socket_.close(ec);
}

int NgHttp2Connection::SendToPeer(const uint8_t* data, size_t len) {
    try {
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (closed_) return NGHTTP2_ERR_CALLBACK_FAILURE;
        write_buf_.insert(write_buf_.end(), data, data + len);
        if (!writing_) {
            writing_ = true;
            auto self = shared_from_this();
            boost::asio::async_write(socket_, boost::asio::buffer(write_buf_),
                [self](boost::system::error_code ec, std::size_t bytes) {
                    self->HandleWrite(ec, bytes);
                });
        }
    } catch (...) {
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    return static_cast<ssize_t>(len);
}

void NgHttp2Connection::HandleWrite(boost::system::error_code ec, std::size_t bytes) {
    try {
        if (ec && ec != boost::asio::error::operation_aborted) {
            memolog::LogWarn("nghttp2.write.fail", ec.message());
        }
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (bytes > 0 && bytes <= write_buf_.size()) {
            write_buf_.erase(write_buf_.begin(), write_buf_.begin() + bytes);
        } else {
            write_buf_.clear();
        }
        if (write_buf_.empty()) {
            writing_ = false;
            if (closed_) {
                boost::system::error_code ec2;
                socket_.close(ec2);
            }
        } else {
            auto self = shared_from_this();
            boost::asio::async_write(socket_, boost::asio::buffer(write_buf_),
                [self](boost::system::error_code ec2, std::size_t n) {
                    self->HandleWrite(ec2, n);
                });
        }
    } catch (const std::exception& e) {
        memolog::LogWarn("nghttp2.write.handler.exc", e.what());
    } catch (...) {
        memolog::LogWarn("nghttp2.write.handler.exc", "unknown");
    }
}

void NgHttp2Connection::HandleSslRead(boost::system::error_code ec, std::size_t bytes) {
    try {
        if (ec) {
            memolog::LogWarn("nghttp2.ssl.read.fail", ec.message());
            return;
        }
        read_buffer_.commit(bytes);
        ProcessSslBuffer();
    } catch (const std::exception& e) {
        memolog::LogWarn("nghttp2.ssl.read.exc", e.what());
    } catch (...) {
        memolog::LogWarn("nghttp2.ssl.read.exc", "unknown");
    }
}

void NgHttp2Connection::ProcessSslBuffer() {
    // Feed readable data into SSL via the read BIO
    const size_t avail = read_buffer_.size();
    if (avail == 0) {
        auto self = shared_from_this();
        net::async_read(socket_, read_buffer_.prepare(8192),
            [self](boost::system::error_code ec, std::size_t bytes) {
                self->HandleSslRead(ec, bytes);
            });
        return;
    }

    // Pull data from the streambuf into the SSL input BIO
    const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(read_buffer_.data().data()));

    BIO* bio = SSL_get_rbio(ssl_);
    BIO* wbio = SSL_get_wbio(ssl_);

    // Feed all available bytes into SSL
    int n = BIO_write(bio, data, static_cast<int>(avail));
    if (n > 0) {
        read_buffer_.consume(static_cast<size_t>(n));
    }

    if (!SSL_is_init_finished(ssl_)) {
        int ret = SSL_accept(ssl_);
        int err = SSL_get_error(ssl_, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_NONE) {
            // Continue handshake
            if (n > 0) {
                ProcessSslBuffer();
            } else {
                auto self = shared_from_this();
                net::async_read(socket_, read_buffer_.prepare(8192),
                    [self](boost::system::error_code ec, std::size_t bytes) {
                        self->HandleSslRead(ec, bytes);
                    });
            }
        } else {
            memolog::LogWarn("nghttp2.ssl.hs.fail", "SSL handshake error: " + std::to_string(err));
            Close();
        }
        // Drain any pending write BIO data to the socket
        DrainWriteBio();
        return;
    }

    // Handshake complete — check ALPN
    const unsigned char* alpn_data = nullptr;
    unsigned int alpn_len = 0;
    SSL_get0_alpn_selected(ssl_, &alpn_data, &alpn_len);

    if (alpn_data && alpn_len == 2 && alpn_data[0] == 'h' && alpn_data[1] == '2') {
        http2_mode_ = true;
        memolog::LogInfo("nghttp2.alpn", "HTTP/2 negotiated via ALPN");
        StartHttp2Session();
    } else {
        memolog::LogInfo("nghttp2.alpn", "ALPN did not select h2 — closing");
        Close();
    }
}

void NgHttp2Connection::DrainWriteBio() {
    BIO* wbio = SSL_get_wbio(ssl_);
    if (!wbio) return;
    char buf[8192];
    int n;
    while ((n = BIO_read(wbio, buf, sizeof(buf))) > 0) {
        boost::asio::write(socket_, boost::asio::buffer(buf, static_cast<size_t>(n)));
    }
}

void NgHttp2Connection::StartHttp2Session() {
    nghttp2_session_callbacks* cbs = nullptr;
    nghttp2_session_callbacks_new(&cbs);
    nghttp2_session_callbacks_set_on_header_callback(cbs, cb_header);
    nghttp2_session_callbacks_set_on_begin_headers_callback(cbs, cb_begin_headers);
    nghttp2_session_callbacks_set_on_frame_recv_callback(cbs, cb_frame_recv);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(cbs, cb_data_chunk_recv);
    nghttp2_session_callbacks_set_on_stream_close_callback(cbs, cb_stream_close);
    nghttp2_session_callbacks_set_send_callback(cbs, cb_send);

    nghttp2_session* sess = nullptr;
    nghttp2_session_server_new(&sess, cbs, this);
    session_ = sess;
    nghttp2_session_callbacks_del(cbs);

    // Submit server's SETTINGS frame. nghttp2 will send the HTTP/2 connection
    // preface + our SETTINGS via SendCallback when nghttp2_session_send() is called.
    nghttp2_settings_entry settings[2] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100},
        {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 65535}
    };
    nghttp2_submit_settings(session_, NGHTTP2_FLAG_NONE, settings, 2);

    // Trigger sending of the server preface (connection preface + SETTINGS frame)
    // via SendCallback, which writes directly to the TLS stream.
    nghttp2_session_send(session_);

    // Flush any pending bytes in the SSL write BIO
    DrainWriteBio();

    StartAsyncRead();
}

void NgHttp2Connection::StartAsyncRead() {
    if (!session_ || closed_) return;
    auto self = shared_from_this();
    socket_.async_read_some(read_buffer_.prepare(8192),
        [self](boost::system::error_code ec, std::size_t bytes) {
            self->HandleRead(ec, bytes);
        });
}

void NgHttp2Connection::HandleRead(boost::system::error_code ec, std::size_t bytes) {
    try {
        if (ec == boost::asio::error::eof) {
            if (session_) {
                nghttp2_session_mem_recv(session_, nullptr, 0);
            }
            return;
        }
        if (ec) {
            memolog::LogWarn("nghttp2.read.fail", ec.message());
            return;
        }

        read_buffer_.commit(bytes);

        BIO* bio = SSL_get_rbio(ssl_);
        BIO* wbio = SSL_get_wbio(ssl_);
        const size_t avail = read_buffer_.size();
        if (avail > 0) {
            const uint8_t* data = static_cast<const uint8_t*>(static_cast<const void*>(read_buffer_.data().data()));
            int n = BIO_write(bio, data, static_cast<int>(avail));
            if (n > 0) {
                read_buffer_.consume(static_cast<size_t>(n));
            }
        }

        ssize_t consumed = 0;
        while (true) {
            uint8_t decrypted[8192];
            int n = SSL_read(ssl_, decrypted, sizeof(decrypted));
            if (n > 0) {
                if (session_) {
                    ssize_t r = nghttp2_session_mem_recv(session_, decrypted, static_cast<size_t>(n));
                    if (r > 0) consumed += r;
                }
            } else {
                break;
            }
        }

        DrainWriteBio();

        if (consumed < 0) {
            memolog::LogWarn("nghttp2.frame.fail", "nghttp2 error: " + std::to_string(consumed));
            return;
        }
        StartAsyncRead();
    } catch (const std::exception& e) {
        memolog::LogWarn("nghttp2.read.handler.exc", e.what());
    } catch (...) {
        memolog::LogWarn("nghttp2.read.handler.exc", "unknown");
    }
}

void NgHttp2Connection::OnBeginHeaders(int32_t stream_id) {
    current_req_ = Http2Request();
    current_stream_id_ = stream_id;
    body_buf_.clear();
}

void NgHttp2Connection::OnHeader(int32_t stream_id, const std::string& key,
                                const std::string& value) {
    if (stream_id != current_stream_id_) return;

    if (key == ":method") {
        current_req_.method = value;
    } else if (key == ":path") {
        current_req_.path = value;
        size_t qpos = value.find('?');
        if (qpos != std::string::npos) {
            current_req_.path = value.substr(0, qpos);
            current_req_.query = value.substr(qpos + 1);
        }
    } else if (key == ":scheme") {
        // ignore
    } else if (key == ":authority") {
        current_req_.headers["host"] = value;
    } else if (key == "x-trace-id") {
        current_req_.trace_id = value;
    } else {
        current_req_.headers[key] = value;
    }
}

void NgHttp2Connection::OnBodyData(int32_t stream_id, const uint8_t* data, size_t len) {
    if (stream_id != current_stream_id_) return;
    body_buf_.insert(body_buf_.end(), data, data + len);
}

void NgHttp2Connection::OnRequestComplete(int32_t stream_id) {
    if (stream_id != current_stream_id_) return;
    current_req_.body.assign(reinterpret_cast<const char*>(body_buf_.data()), body_buf_.size());
    body_buf_.clear();

    Http2Response h2resp;
    h2resp.SetHeader("X-Trace-Id", current_req_.trace_id);
    Http2Routes::HandleRequest(current_req_, h2resp);

    int http_status = h2resp.status_code;

    // Build response header NV array (lowercase names, no copy)
    std::vector<nghttp2_nv> nva;
    nva.reserve(4 + h2resp.headers.size());

    auto add_nv = [&](const char* name, const char* value, size_t vlen) {
        nva.push_back({(uint8_t*)name, (uint8_t*)value,
                       static_cast<uint16_t>(std::strlen(name)), static_cast<uint16_t>(vlen),
                       NGHTTP2_NV_FLAG_NO_COPY_NAME | NGHTTP2_NV_FLAG_NO_COPY_VALUE});
    };

    add_nv(":status", std::to_string(http_status).c_str(),
           std::to_string(http_status).size());

    if (!h2resp.content_type.empty()) {
        add_nv("content-type", h2resp.content_type.c_str(),
               h2resp.content_type.size());
    }
    if (!h2resp.body.empty()) {
        add_nv("content-length", std::to_string(h2resp.body.size()).c_str(),
               std::to_string(h2resp.body.size()).size());
    }
    for (const auto& h : h2resp.headers) {
        add_nv(h.first.c_str(), h.second.c_str(), h.second.size());
    }

    nghttp2_submit_headers(session_, NGHTTP2_FLAG_END_STREAM,
                           stream_id, nullptr, nva.data(),
                           static_cast<size_t>(nva.size()), nullptr);

    if (!h2resp.body.empty()) {
        auto body = std::make_shared<BodyBuf>();
        body->data.assign(reinterpret_cast<const uint8_t*>(h2resp.body.data()),
                          reinterpret_cast<const uint8_t*>(h2resp.body.data()) + h2resp.body.size());
        pending_body_ = body;  // keep alive on connection

        // Register body in global map so the static callback can retrieve it
        {
            std::lock_guard<std::mutex> lock(g_body_mutex);
            g_body_map[stream_id] = body;
        }

        // Use nghttp2_submit_data2 with a data provider backed by StaticDataCallback
        nghttp2_data_provider2 prd2;
        prd2.source.ptr = nullptr;  // body looked up by stream_id in callback
        prd2.read_callback = StaticDataCallback;

        nghttp2_submit_data2(session_, NGHTTP2_FLAG_END_STREAM, stream_id, &prd2);
    }

    nghttp2_session_send(session_);
}

void NgHttp2Connection::OnStreamClose(int32_t stream_id, uint32_t) {
    if (stream_id == current_stream_id_) {
        current_stream_id_ = -1;
        pending_body_.reset();  // free body buffer now that nghttp2 consumed it
    }
    // Unregister body from global map
    {
        std::lock_guard<std::mutex> lock(g_body_mutex);
        g_body_map.erase(stream_id);
    }
}

// ── Acceptor ──────────────────────────────────────────────────────────────────

static void StartAccept() {
    auto& acc = *g_acceptor;
    acc.async_accept(
        [&](boost::system::error_code ec, tcp::socket socket) {
            try {
                if (ec) {
                    memolog::LogWarn("nghttp2.accept.fail", ec.message());
                    if (g_running) StartAccept();
                    return;
                }
                if (g_running) {
                    auto conn = std::make_shared<NgHttp2Connection>(*g_ioc, g_raw_ssl_ctx, std::move(socket));
                    conn->Start();
                    StartAccept();
                }
            } catch (const std::exception& e) {
                memolog::LogWarn("nghttp2.accept.exc", e.what());
                if (g_running) StartAccept();
            } catch (...) {
                memolog::LogWarn("nghttp2.accept.exc", "unknown");
                if (g_running) StartAccept();
            }
        });
}

// ── Singleton ─────────────────────────────────────────────────────────────────

NgHttp2Server& NgHttp2Server::GetInstance() {
    static NgHttp2Server instance;
    return instance;
}

NgHttp2Server::NgHttp2Server() = default;

NgHttp2Server::~NgHttp2Server() { Stop(); }

bool NgHttp2Server::Initialize() {
    if (g_initialized) return true;
    Http2Routes::RegisterRoutes();
    g_initialized = true;
    return true;
}

void NgHttp2Server::Run() {
    net::io_context ioc{1};

    tcp::endpoint endpoint(tcp::v4(), static_cast<unsigned short>(_h2_port));
    tcp::acceptor acceptor{ioc};
    acceptor.open(endpoint.protocol());
    acceptor.set_option(net::socket_base::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen(128);

    SSL_CTX* raw_ssl = SSL_CTX_new(TLS_server_method());
    if (!raw_ssl) {
        memolog::LogWarn("nghttp2.init.fail", "SSL_CTX_new failed");
        return;
    }
    SSL_CTX_set_min_proto_version(raw_ssl, TLS1_2_VERSION);
    SSL_CTX_set_mode(raw_ssl, SSL_MODE_AUTO_RETRY);

    std::filesystem::path exe_dir = std::filesystem::current_path();
    std::string crt = (exe_dir / "server.crt").string();
    std::string key = (exe_dir / "server.key").string();

    if (!std::filesystem::exists(crt) || !std::filesystem::exists(key)) {
        memolog::LogInfo("nghttp2.ssl.gen", "Generating self-signed cert/key");
        if (!CertUtil::GenerateSelfSignedCertPem(crt, key)) {
            memolog::LogWarn("nghttp2.init.fail", "Cert generation failed");
            SSL_CTX_free(raw_ssl);
            return;
        }
    }

    if (SSL_CTX_use_certificate_file(raw_ssl, crt.c_str(), SSL_FILETYPE_PEM) <= 0
        || SSL_CTX_use_PrivateKey_file(raw_ssl, key.c_str(), SSL_FILETYPE_PEM) <= 0
        || !SSL_CTX_check_private_key(raw_ssl)) {
        memolog::LogWarn("nghttp2.init.fail", "Failed to load cert/key");
        SSL_CTX_free(raw_ssl);
        return;
    }

    // ALPN: advertise h2 (HTTP/2 over TLS)
    if (SSL_CTX_set_alpn_protos(raw_ssl, reinterpret_cast<const unsigned char*>("h2"), 2) != 0) {
        memolog::LogWarn("nghttp2.init.fail", "ALPN h2 set failed");
    }

    memolog::LogInfo("nghttp2.listen", "NgHttp2Server listening on 0.0.0.0:" + std::to_string(_h2_port));

    g_ioc = &ioc;
    g_raw_ssl_ctx = raw_ssl;
    g_acceptor = &acceptor;

    g_running = true;
    StartAccept();

    while (g_running) {
        try {
            ioc.run_one_for(std::chrono::milliseconds(100));
        } catch (const std::exception& e) {
            memolog::LogWarn("nghttp2.ioc.exc", e.what());
        } catch (...) {
            memolog::LogWarn("nghttp2.ioc.exc", "unknown exception in nghttp2 event loop");
        }
    }

    SSL_CTX_free(raw_ssl);
    g_ioc = nullptr;
    g_raw_ssl_ctx = nullptr;
    g_acceptor = nullptr;
    g_initialized = false;
    memolog::LogInfo("nghttp2.stop", "NgHttp2Server stopped");
}

void NgHttp2Server::Stop() {
    g_running = false;
    if (_thread.joinable()) _thread.join();
}

#endif  // MEMOCHAT_HAVE_NGHTTP2
