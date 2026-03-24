#include "GateHttp3Listener.h"
#include "CertUtil.h"
#include "LogicSystem.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#ifdef MEMOCHAT_ENABLE_HTTP3

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <msquic.h>
#include <nghttp3/nghttp3.h>

#include <fstream>
#include <sstream>
#include <filesystem>

static const QUIC_API_TABLE* g_QuicApi = nullptr;
static HQUIC g_Registration = nullptr;

struct StreamContext {
    enum State {
        STATE_READING_HEADERS = 0,
        STATE_READING_BODY = 1,
        STATE_DISPATCHING = 2,
        STATE_SENDING_RESPONSE = 3,
    };
    HQUIC Connection = nullptr;
    HQUIC Stream = nullptr;
    std::string Method;
    std::string Path;
    std::string Body;
    std::string HeadersRaw;   // holds raw bytes while parsing headers
    std::unordered_map<std::string, std::string> Headers;
    State State = STATE_READING_HEADERS;
    bool HeadersComplete = false;
    bool BodyComplete = false;
    std::string ResponseBody;
    int ResponseStatus = 200;
    std::string ResponseContentType = "application/json";
    bool ResponseSent = false;
    std::string TraceId;
    std::string RequestId;
    int64_t ContentLength = -1;
    int64_t BodyReceived = 0;
    GateHttp3Listener* Listener = nullptr;
    StreamContext() = default;
    ~StreamContext() = default;
};

struct GateHttp3Listener::Impl : public std::enable_shared_from_this<Impl> {
    GateHttp3Listener* listener_ = nullptr;
    boost::asio::io_context& ioc_;
    LogicSystem& logic_;
    int port_;
    std::atomic<bool>& running_;
    std::mutex conn_mu_;
    std::mutex stream_mu_;
    std::unordered_map<HQUIC, std::shared_ptr<StreamContext>> conn_map_;
    std::unordered_map<HQUIC, std::shared_ptr<StreamContext>> stream_map_;
    HQUIC Listener = nullptr;
    HQUIC Configuration = nullptr;
    Impl(boost::asio::io_context& ioc, LogicSystem& logic, int port,
         std::atomic<bool>& running)
        : ioc_(ioc), logic_(logic), port_(port), running_(running) {}
    ~Impl() {
        if (Configuration) {
            g_QuicApi->ConfigurationClose(Configuration);
            Configuration = nullptr;
        }
        if (Listener) {
            g_QuicApi->ListenerClose(Listener);
            Listener = nullptr;
        }
    }
    nghttp3_conn* nghttp3Conn = nullptr;
};

static QUIC_STATUS ConnectionCallback(HQUIC, void*, QUIC_CONNECTION_EVENT*);
static QUIC_STATUS StreamCallback(HQUIC, void*, QUIC_STREAM_EVENT*);

static QUIC_STATUS ListenerCallback(HQUIC, void* Context, QUIC_LISTENER_EVENT* Event) {
    auto* impl = static_cast<GateHttp3Listener::Impl*>(Context);
    switch (Event->Type) {
    case QUIC_LISTENER_EVENT_NEW_CONNECTION: {
        GateHttp3Listener* listener = impl->listener_;
        if (!listener) return QUIC_STATUS_SUCCESS;

        auto* conn_ctx = new StreamContext();
        conn_ctx->Connection = Event->NEW_CONNECTION.Connection;
        conn_ctx->Listener = listener;
        g_QuicApi->SetContext(Event->NEW_CONNECTION.Connection, conn_ctx);
        g_QuicApi->SetCallbackHandler(
            Event->NEW_CONNECTION.Connection,
            reinterpret_cast<void*>(ConnectionCallback), conn_ctx);

        const QUIC_NEW_CONNECTION_INFO* info = Event->NEW_CONNECTION.Info;
        char alpn[32] = {0};
        if (info->NegotiatedAlpn && info->NegotiatedAlpnLength > 0) {
            memcpy(alpn, info->NegotiatedAlpn,
                   std::min((size_t)info->NegotiatedAlpnLength, sizeof(alpn) - 1));
        }
        std::map<std::string, std::string> fields;
        fields["alpn"] = alpn;
        fields["trace_id"] = conn_ctx->TraceId;
        memolog::LogInfo("http3.conn.incoming",
            "HTTP/3 incoming connection", fields);

        QUIC_STATUS status = g_QuicApi->ConnectionSetConfiguration(
            Event->NEW_CONNECTION.Connection, impl->Configuration);
        if (QUIC_FAILED(status)) {
            std::map<std::string, std::string> efields;
            efields["status"] = std::to_string(status);
            memolog::LogError("http3.conn.config.fail",
                "Failed to set configuration", efields);
            delete conn_ctx;
        }
        return QUIC_STATUS_SUCCESS;
    }
    case QUIC_LISTENER_EVENT_STOP_COMPLETE:
        memolog::LogInfo("http3.listener.stopped",
            "HTTP/3 listener stopped", {});
        return QUIC_STATUS_SUCCESS;
    default:
        return QUIC_STATUS_NOT_SUPPORTED;
    }
}

static QUIC_STATUS ConnectionCallback(HQUIC Connection, void* Context,
                                     QUIC_CONNECTION_EVENT* Event) {
    auto* stream_ctx = static_cast<StreamContext*>(Context);
    switch (Event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED: {
        std::string tid = stream_ctx ? stream_ctx->TraceId : "";
        std::map<std::string, std::string> fields;
        fields["trace_id"] = tid;
        memolog::LogInfo("http3.conn.connected",
            "HTTP/3 connection established", fields);
        return QUIC_STATUS_SUCCESS;
    }
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT: {
        std::string tid = stream_ctx ? stream_ctx->TraceId : "";
        std::map<std::string, std::string> fields;
        fields["error"] = std::to_string(Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
        fields["trace_id"] = tid;
        memolog::LogWarn("http3.conn.transport_shutdown",
            "Connection shutdown by transport", fields);
        return QUIC_STATUS_SUCCESS;
    }
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER: {
        std::string tid = stream_ctx ? stream_ctx->TraceId : "";
        std::map<std::string, std::string> fields;
        fields["trace_id"] = tid;
        memolog::LogWarn("http3.conn.peer_shutdown",
            "Connection shutdown by peer", fields);
        return QUIC_STATUS_SUCCESS;
    }
    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        if (stream_ctx) delete stream_ctx;
        return QUIC_STATUS_SUCCESS;
    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
        auto* new_ctx = new StreamContext();
        new_ctx->Connection = Connection;
        new_ctx->Stream = Event->PEER_STREAM_STARTED.Stream;
        new_ctx->TraceId = memolog::TraceContext::NewId();
        new_ctx->RequestId = memolog::TraceContext::NewId();
        new_ctx->Listener = stream_ctx ? stream_ctx->Listener : nullptr;
        g_QuicApi->SetContext(Event->PEER_STREAM_STARTED.Stream, new_ctx);
        g_QuicApi->SetCallbackHandler(
            Event->PEER_STREAM_STARTED.Stream,
            reinterpret_cast<void*>(StreamCallback), new_ctx);
        std::map<std::string, std::string> fields;
        fields["trace_id"] = new_ctx->TraceId;
        memolog::LogInfo("http3.stream.started",
            "HTTP/3 stream started", fields);
        return QUIC_STATUS_SUCCESS;
    }
    default:
        return QUIC_STATUS_SUCCESS;
    }
}

static QUIC_STATUS StreamCallback(HQUIC Stream, void* Context,
                                  QUIC_STREAM_EVENT* Event) {
    auto* stream_ctx = static_cast<StreamContext*>(Context);
    if (!stream_ctx) return QUIC_STATUS_SUCCESS;

    switch (Event->Type) {
    case QUIC_STREAM_EVENT_RECEIVE: {
        const QUIC_BUFFER* buffers = Event->RECEIVE.Buffers;
        uint32_t buffer_count = Event->RECEIVE.BufferCount;
        bool fin = (Event->RECEIVE.Flags & QUIC_RECEIVE_FLAG_FIN) != 0;
        uint64_t total_len = 0;

        for (uint32_t i = 0; i < buffer_count; ++i) {
            total_len += buffers[i].Length;
        }

        // State machine for HTTP request parsing
        switch (stream_ctx->State) {
        case StreamContext::STATE_READING_HEADERS: {
            // Append data to headers buffer
            for (uint32_t i = 0; i < buffer_count; ++i) {
                const uint8_t* data = buffers[i].Buffer;
                uint32_t len = buffers[i].Length;
                stream_ctx->HeadersRaw.append(
                    reinterpret_cast<const char*>(data), len);
                stream_ctx->BodyReceived += len;
            }

            // Look for \r\n\r\n header boundary
            size_t header_end = stream_ctx->HeadersRaw.find("\r\n\r\n");
            if (header_end == std::string::npos) {
                // Incomplete headers, keep accumulating
                std::map<std::string, std::string> fields;
                fields["len"] = std::to_string(total_len);
                fields["trace_id"] = stream_ctx->TraceId;
                memolog::LogDebug("http3.stream.header_partial",
                    "HTTP/3 partial header received", fields);
                break;
            }

            // Parse request line + headers
            std::string header_part = stream_ctx->HeadersRaw.substr(0, header_end);
            std::istringstream ss(header_part);
            std::string line;
            bool first = true;
            while (std::getline(ss, line)) {
                if (line.empty() || line == "\r") break;
                if (first) {
                    std::istringstream req(line);
                    req >> stream_ctx->Method >> stream_ctx->Path;
                    first = false;
                    continue;
                }
                auto colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string key = line.substr(0, colon);
                    std::string val = line.substr(colon + 1);
                    while (!val.empty() && (val[0] == ' ' || val[0] == '\t')) {
                        val.erase(val.begin());
                    }
                    if (!val.empty() && val.back() == '\r') val.pop_back();
                    if (key == "content-length" || key == "Content-Length") {
                        try { stream_ctx->ContentLength = std::stoll(val); }
                        catch (...) { stream_ctx->ContentLength = -1; }
                    }
                    stream_ctx->Headers[key] = val;
                }
            }

            // Extract body: everything after \r\n\r\n in HeadersRaw
            // (HeadersRaw already contains the full stream data up to now)
            // Since we're accumulating all bytes in HeadersRaw, the body starts
            // at header_end + 4 (skip the \r\n\r\n delimiter)
            size_t body_start = header_end + 4;
            stream_ctx->Body = stream_ctx->HeadersRaw.substr(body_start);
            // Clear HeadersRaw to save memory
            stream_ctx->HeadersRaw.clear();
            stream_ctx->State = StreamContext::STATE_READING_BODY;

            std::map<std::string, std::string> fields;
            fields["method"] = stream_ctx->Method;
            fields["path"] = stream_ctx->Path;
            fields["content_length"] = std::to_string(stream_ctx->ContentLength);
            fields["trace_id"] = stream_ctx->TraceId;
            memolog::LogInfo("http3.stream.headers_done",
                "HTTP/3 headers parsed", fields);

            // If no body expected, dispatch immediately
            if (stream_ctx->ContentLength <= 0) {
                stream_ctx->BodyComplete = true;
            } else if (static_cast<int64_t>(stream_ctx->Body.size()) >= stream_ctx->ContentLength) {
                stream_ctx->BodyComplete = true;
            }
            // If FIN already received and no Content-Length, treat as complete
            if (fin && stream_ctx->ContentLength < 0) {
                stream_ctx->BodyComplete = true;
            }

            if (stream_ctx->BodyComplete) {
                goto do_dispatch;
            }
            break;
        }
        case StreamContext::STATE_READING_BODY: {
            // Append additional body bytes
            for (uint32_t i = 0; i < buffer_count; ++i) {
                stream_ctx->Body.append(
                    reinterpret_cast<const char*>(buffers[i].Buffer),
                    buffers[i].Length);
            }

            // Check if body is complete
            if (stream_ctx->ContentLength >= 0 &&
                static_cast<int64_t>(stream_ctx->Body.size()) >= stream_ctx->ContentLength) {
                stream_ctx->BodyComplete = true;
            } else if (fin) {
                // Peer sent FIN — no more body data coming
                stream_ctx->BodyComplete = true;
            }

            if (stream_ctx->BodyComplete) {
                // Trim excess bytes (shouldn't happen, but be safe)
                if (stream_ctx->ContentLength >= 0 &&
                    static_cast<int64_t>(stream_ctx->Body.size()) > stream_ctx->ContentLength) {
                    stream_ctx->Body.resize(stream_ctx->ContentLength);
                }
                goto do_dispatch;
            }
            break;
        }
        default:
            // Already dispatched or in sending state — ignore more data
            break;
        }

        if (fin && stream_ctx->State != StreamContext::STATE_DISPATCHING) {
            // FIN received but we haven't dispatched yet (edge case: empty body)
            stream_ctx->BodyComplete = true;
            goto do_dispatch;
        }

        if (total_len > 0) {
            g_QuicApi->StreamReceiveComplete(Stream, total_len);
        }
        return QUIC_STATUS_SUCCESS;

do_dispatch:
        // === Dispatch to LogicSystem ===
        stream_ctx->State = StreamContext::STATE_DISPATCHING;
        GateHttp3Listener* listener = stream_ctx->Listener;
        if (!listener) {
            g_QuicApi->StreamReceiveComplete(Stream, total_len);
            return QUIC_STATUS_SUCCESS;
        }

        std::string uri = stream_ctx->Path;
        auto qpos = uri.find('?');
        std::string path = (qpos != std::string::npos) ? uri.substr(0, qpos) : uri;
        std::string query_string = (qpos != std::string::npos) ? uri.substr(qpos + 1) : "";

        auto conn = std::make_shared<GateHttp3Connection>();
        conn->SetConnIndex(0);
        conn->SetStreamId(0);
        conn->GetRequestMethodRef() = stream_ctx->Method;
        conn->GetRequestPathRef() = path;
        conn->GetContentLengthRef() = stream_ctx->ContentLength;
        conn->SetQueryString(query_string);
        conn->SetHeadersComplete();
        conn->OnStreamOpen(0);

        // Pass body bytes to the connection
        // Use OnStreamRead to populate request_body_ via reflection
        // (direct assignment via internal mechanism)
        // Since GateHttp3Connection stores body in request_body_ which is private,
        // we need a helper. The HandlePost/Get functions in handlers use
        // GateHttp3JsonSupport::ParseJsonBody which calls connection->GetRequestBody().
        // We need to set the body on the connection. Since request_body_ is private,
        // we use a dedicated method or the fact that GetRequestBody() returns
        // request_body_ by const ref. We need to expose a setter.
        // Approach: call HandlePost/Get which will call ParseJsonBody which calls
        // GetRequestBody(). We must set the body first.
        // Solution: add a method SetRequestBody() to GateHttp3Connection.

        conn->SetRequestBody(stream_ctx->Body);

        bool handled = false;
        if (stream_ctx->Method == "POST" || stream_ctx->Method == "post") {
            handled = listener->GetLogicSystem().HandlePost(path, conn);
            if (!handled) {
                std::map<std::string, std::string> f;
                f["path"] = path;
                f["trace_id"] = stream_ctx->TraceId;
                memolog::LogWarn("http3.route.not_found",
                    "HTTP/3 POST route not found", f);
            }
        } else {
            handled = listener->GetLogicSystem().HandleGet(path, conn);
            if (!handled) {
                std::map<std::string, std::string> f;
                f["path"] = path;
                f["trace_id"] = stream_ctx->TraceId;
                memolog::LogWarn("http3.route.not_found",
                    "HTTP/3 GET route not found", f);
            }
        }

        std::string resp_body;
        int resp_status = 404;
        std::string ct = "application/json";
        if (conn->IsResponseComplete()) {
            resp_body = conn->ResponseBody();
            resp_status = conn->ResponseStatus();
            ct = conn->ResponseContentType();
        } else {
            resp_body = "{\"error\":404,\"message\":\"not found\"}";
        }

        stream_ctx->State = StreamContext::STATE_SENDING_RESPONSE;
        stream_ctx->ResponseSent = true;

        std::ostringstream http_resp;
        http_resp << "HTTP/3 " << resp_status << " \r\n"
                  << "content-type: " << ct << "\r\n"
                  << "content-length: " << resp_body.size() << "\r\n"
                  << "\r\n" << resp_body;
        std::string resp_str = http_resp.str();

        QUIC_BUFFER qbuf{};
        qbuf.Buffer = reinterpret_cast<uint8_t*>(const_cast<char*>(resp_str.data()));
        qbuf.Length = static_cast<uint32_t>(resp_str.size());

        QUIC_STATUS send_st = g_QuicApi->StreamSend(
            Stream, &qbuf, 1, QUIC_SEND_FLAG_FIN, nullptr);
        if (QUIC_SUCCEEDED(send_st)) {
            std::map<std::string, std::string> f;
            f["status"] = std::to_string(resp_status);
            f["body_len"] = std::to_string(resp_body.size());
            f["trace_id"] = stream_ctx->TraceId;
            memolog::LogInfo("http3.response.sent",
                "HTTP/3 response sent", f);
        } else {
            std::map<std::string, std::string> f;
            f["status"] = std::to_string(send_st);
            f["trace_id"] = stream_ctx->TraceId;
            memolog::LogWarn("http3.response.send_fail",
                "HTTP/3 response send failed", f);
        }

        if (total_len > 0) {
            g_QuicApi->StreamReceiveComplete(Stream, total_len);
        }
        return QUIC_STATUS_SUCCESS;
    }
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        return QUIC_STATUS_SUCCESS;
    case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN: {
        // Peer closed their send side — body is complete
        if (stream_ctx && !stream_ctx->BodyComplete && !stream_ctx->ResponseSent) {
            stream_ctx->BodyComplete = true;
            std::map<std::string, std::string> f;
            f["trace_id"] = stream_ctx->TraceId;
            f["state"] = std::to_string(stream_ctx->State);
            memolog::LogInfo("http3.stream.peer_shutdown",
                "HTTP/3 peer send shutdown, dispatching", f);

            if (stream_ctx->State == StreamContext::STATE_READING_BODY) {
                // We have everything, dispatch now
                // Re-invoke receive handler with the accumulated state
                // by calling into the Stream directly
                // Instead, just set a flag and let the next receive complete it
                // Since there's no data to consume, trigger dispatch directly
                GateHttp3Listener* listener = stream_ctx->Listener;
                if (listener) {
                    std::string uri = stream_ctx->Path;
                    auto qpos = uri.find('?');
                    std::string path = (qpos != std::string::npos) ? uri.substr(0, qpos) : uri;
                    std::string query_string = (qpos != std::string::npos) ? uri.substr(qpos + 1) : "";
                    auto conn = std::make_shared<GateHttp3Connection>();
                    conn->SetConnIndex(0);
                    conn->SetStreamId(0);
                    conn->GetRequestMethodRef() = stream_ctx->Method;
                    conn->GetRequestPathRef() = path;
                    conn->GetContentLengthRef() = stream_ctx->ContentLength;
                    conn->SetQueryString(query_string);
                    conn->SetHeadersComplete();
                    conn->OnStreamOpen(0);
                    conn->SetRequestBody(stream_ctx->Body);

                    bool handled = false;
                    if (stream_ctx->Method == "POST" || stream_ctx->Method == "post") {
                        handled = listener->GetLogicSystem().HandlePost(path, conn);
                    } else {
                        handled = listener->GetLogicSystem().HandleGet(path, conn);
                    }

                    std::string resp_body;
                    int resp_status = 404;
                    std::string ct = "application/json";
                    if (conn->IsResponseComplete()) {
                        resp_body = conn->ResponseBody();
                        resp_status = conn->ResponseStatus();
                        ct = conn->ResponseContentType();
                    } else {
                        resp_body = "{\"error\":404,\"message\":\"not found\"}";
                    }

                    stream_ctx->State = StreamContext::STATE_SENDING_RESPONSE;
                    stream_ctx->ResponseSent = true;

                    std::ostringstream http_resp;
                    http_resp << "HTTP/3 " << resp_status << " \r\n"
                              << "content-type: " << ct << "\r\n"
                              << "content-length: " << resp_body.size() << "\r\n"
                              << "\r\n" << resp_body;
                    std::string resp_str = http_resp.str();
                    QUIC_BUFFER qbuf{};
                    qbuf.Buffer = reinterpret_cast<uint8_t*>(const_cast<char*>(resp_str.data()));
                    qbuf.Length = static_cast<uint32_t>(resp_str.size());
                    g_QuicApi->StreamSend(Stream, &qbuf, 1, QUIC_SEND_FLAG_FIN, nullptr);
                }
            }
        }
        return QUIC_STATUS_SUCCESS;
    }
    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED: {
        std::map<std::string, std::string> f;
        f["trace_id"] = stream_ctx->TraceId;
        memolog::LogWarn("http3.stream.aborted",
            "HTTP/3 stream aborted by peer", f);
        return QUIC_STATUS_SUCCESS;
    }
    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE: {
        std::map<std::string, std::string> f;
        f["trace_id"] = stream_ctx->TraceId;
        memolog::LogInfo("http3.stream.closed",
            "HTTP/3 stream closed", f);
        delete stream_ctx;
        return QUIC_STATUS_SUCCESS;
    }
    default:
        return QUIC_STATUS_SUCCESS;
    }
}

static bool EnsureSelfSignedCert(std::string& crt_path,
                                 std::string& key_path,
                                 std::string& pfx_path) {
    std::filesystem::path exe_dir = std::filesystem::current_path();
    crt_path = (exe_dir / "server.crt").string();
    key_path = (exe_dir / "server.key").string();
    pfx_path = (exe_dir / "server.pfx").string();

    if (std::filesystem::exists(pfx_path) &&
        std::filesystem::exists(crt_path) &&
        std::filesystem::exists(key_path)) {
        return true;
    }

    return CertUtil::GenerateSelfSignedCertPfx(
        pfx_path, crt_path, key_path, "memochat");
}

static bool LoadSelfSignedCertFromPfx(HQUIC Configuration,
                                      const std::string& pfx_path,
                                      std::string& error_out) {
    if (!std::filesystem::exists(pfx_path)) {
        error_out = "PFX file not found: " + pfx_path;
        return false;
    }

    // Read the PFX file into memory
    std::ifstream pfx_file(pfx_path, std::ios::binary);
    if (!pfx_file) {
        error_out = "Cannot open PFX file: " + pfx_path;
        return false;
    }

    std::vector<uint8_t> pfx_data(
        (std::istreambuf_iterator<char>(pfx_file)),
        std::istreambuf_iterator<char>());
    pfx_file.close();

    if (pfx_data.empty()) {
        error_out = "PFX file is empty: " + pfx_path;
        return false;
    }

    QUIC_CERTIFICATE_PKCS12 pkcs12{};
    pkcs12.Asn1Blob = pfx_data.data();
    pkcs12.Asn1BlobLength = static_cast<uint32_t>(pfx_data.size());
    pkcs12.PrivateKeyPassword = const_cast<char*>("memochat");

    QUIC_CREDENTIAL_CONFIG cred{};
    cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_PKCS12;
    cred.Flags = QUIC_CREDENTIAL_FLAG_NONE;
    cred.CertificatePkcs12 = &pkcs12;

    QUIC_STATUS st = g_QuicApi->ConfigurationLoadCredential(Configuration, &cred);
    if (QUIC_FAILED(st)) {
        error_out = "ConfigurationLoadCredential (PKCS12) failed with status: " + std::to_string(st);
        return false;
    }

    memolog::LogInfo("http3.cert.pfx.ok",
        "Self-signed certificate loaded from PFX for HTTP/3",
        {{"pfx", pfx_path}});
    return true;
}

GateHttp3Listener::GateHttp3Listener(boost::asio::io_context& ioc,
                                   LogicSystem& logic, int port)
    : ioc_(ioc), logic_(logic), port_(port)
#ifdef MEMOCHAT_ENABLE_HTTP3
      , pImpl_(std::make_unique<Impl>(ioc, logic, port, running_))
#endif
{}

GateHttp3Listener::~GateHttp3Listener() { Stop(); }

bool GateHttp3Listener::Start(std::string& error) {
#ifdef MEMOCHAT_ENABLE_HTTP3
    if (!pImpl_) {
        pImpl_ = std::make_unique<Impl>(ioc_, logic_, port_, running_);
    }

    if (!g_QuicApi) {
        QUIC_STATUS st = MsQuicOpen2(&g_QuicApi);
        if (QUIC_FAILED(st)) {
            error = "MsQuicOpen2 failed: " + std::to_string(st);
            memolog::LogError("http3.msquic.fail",
                error, {});
            return false;
        }
        memolog::LogInfo("http3.msquic.ok", "MsQuic API opened", {});
    }

    if (!g_Registration) {
        QUIC_REGISTRATION_CONFIG cfg{
            "GateServer-H3", QUIC_EXECUTION_PROFILE_LOW_LATENCY};
        QUIC_STATUS st = g_QuicApi->RegistrationOpen(&cfg, &g_Registration);
        if (QUIC_FAILED(st)) {
            error = "RegistrationOpen failed: " + std::to_string(st);
            memolog::LogError("http3.reg.fail", error, {});
            return false;
        }
        memolog::LogInfo("http3.reg.ok",
            "MsQuic registration opened", {});
    }

    const QUIC_BUFFER alpns[] = {
        {sizeof("h3") - 1, (uint8_t*)"h3"},
        {sizeof("h3-29") - 1, (uint8_t*)"h3-29"},
        {sizeof("h3-25") - 1, (uint8_t*)"h3-25"},
    };

    QUIC_SETTINGS settings{};
    memset(&settings, 0, sizeof(settings));
    settings.IsSet.IdleTimeoutMs = 1;
    settings.IsSet.PeerBidiStreamCount = 1;
    settings.IsSet.PeerUnidiStreamCount = 1;
    settings.PeerBidiStreamCount = 256;
    settings.PeerUnidiStreamCount = 128;
    settings.IdleTimeoutMs = 60000;

    QUIC_STATUS st = g_QuicApi->ConfigurationOpen(
        g_Registration, alpns, 3,
        &settings, sizeof(settings),
        nullptr, &pImpl_->Configuration);
    if (QUIC_FAILED(st)) {
        error = "ConfigurationOpen failed: " + std::to_string(st);
        memolog::LogError("http3.cfg.open.fail", error, {});
        return false;
    }

    // Build certificate paths relative to the running executable's directory
    std::filesystem::path exe_dir = std::filesystem::current_path();
    std::string crt_path = (exe_dir / "server.crt").string();
    std::string key_path = (exe_dir / "server.key").string();
    std::string pfx_path = (exe_dir / "server.pfx").string();

    // Ensure the certificate files exist (generate if needed)
    if (!EnsureSelfSignedCert(crt_path, key_path, pfx_path)) {
        error = "Failed to generate self-signed certificate files";
        return false;
    }

    // Load the certificate into the QUIC configuration
    // Try QUIC_CERTIFICATE_FILE_PROTECTED first (PEM + encrypted key, supported natively)
    QUIC_CERTIFICATE_FILE_PROTECTED cert_file{};
    cert_file.CertificateFile = crt_path.c_str();
    cert_file.PrivateKeyFile = key_path.c_str();
    cert_file.PrivateKeyPassword = "memochat";

    QUIC_CREDENTIAL_CONFIG cred{};
    cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE_PROTECTED;
    cred.Flags = QUIC_CREDENTIAL_FLAG_NONE;
    cred.CertificateFileProtected = &cert_file;

    st = g_QuicApi->ConfigurationLoadCredential(
        pImpl_->Configuration, &cred);
    if (QUIC_FAILED(st)) {
        std::map<std::string, std::string> ef;
        ef["status"] = std::to_string(st);
        ef["crt"] = crt_path;
        ef["key"] = key_path;
        memolog::LogError("http3.cred.fail.file_protected",
            "ConfigurationLoadCredential (FILE_PROTECTED) failed", ef);
        // Fallback: try PFX blob loading
        std::string pfx_err;
        if (!LoadSelfSignedCertFromPfx(pImpl_->Configuration, pfx_path, pfx_err)) {
            error = "Both FILE_PROTECTED and PFX certificate loading failed: " + pfx_err;
            return false;
        }
        memolog::LogWarn("http3.cred.pfx.fallback",
            "Successfully loaded certificate via PFX fallback", {});
    }

    pImpl_->listener_ = this;

    st = g_QuicApi->ListenerOpen(
        g_Registration, ListenerCallback,
        pImpl_.get(), &pImpl_->Listener);
    if (QUIC_FAILED(st)) {
        error = "ListenerOpen failed: " + std::to_string(st);
        memolog::LogError("http3.lsn.open.fail", error, {});
        return false;
    }

    QUIC_ADDR addr{};
    QuicAddrSetPort(&addr, static_cast<uint16_t>(port_));
    st = g_QuicApi->ListenerStart(pImpl_->Listener, alpns, 3, &addr);
    if (QUIC_FAILED(st)) {
        error = "ListenerStart failed: " + std::to_string(st);
        std::map<std::string, std::string> ef;
        ef["error"] = error;
        ef["port"] = std::to_string(port_);
        memolog::LogError("http3.lsn.start.fail",
            "ListenerStart failed", ef);
        g_QuicApi->ListenerClose(pImpl_->Listener);
        pImpl_->Listener = nullptr;
        return false;
    }

    running_ = true;
    memolog::LogInfo("gate.http3.started",
        "GateServer HTTP/3 listener started",
        {{"port", std::to_string(port_)}});
    return true;
#else
    (void)error;
    return false;
#endif
}

void GateHttp3Listener::Stop() {
    if (!running_.exchange(false)) return;
#ifdef MEMOCHAT_ENABLE_HTTP3
    if (pImpl_) {
        if (pImpl_->Listener) {
            g_QuicApi->ListenerStop(pImpl_->Listener);
        }
        pImpl_.reset();
    }
    if (g_Registration) {
        g_QuicApi->RegistrationClose(g_Registration);
        g_Registration = nullptr;
    }
    if (g_QuicApi) {
        MsQuicClose(g_QuicApi);
        g_QuicApi = nullptr;
    }
#endif
    memolog::LogInfo("gate.http3.stopped",
        "GateServer HTTP/3 listener stopped", {});
}

#endif  // MEMOCHAT_ENABLE_HTTP3
