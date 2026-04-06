#ifdef MEMOCHAT_HAVE_H2O

#include "WinCompat.h"
#include "H2oServer.h"
#include "Http2Routes.h"
#include "Http2Handlers.h"
#include "Http2AuthHandlers.h"
#include "Http2CallHandlers.h"
#include "Http2MediaHandlers.h"
#include "Http2ProfileHandlers.h"
#include "Http2MomentsSupport.h"
#include "CertUtil.h"
#include "logging/Logger.h"
#include <h2o.h>
#include <h2o/http1.h>
#include <h2o/http2.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <csignal>

static h2o_globalconf_t g_global_config;
static h2o_hostconf_t* g_host_config = nullptr;
static SSL_CTX* g_ssl_ctx = nullptr;
static bool g_h2o_initialized = false;
static bool g_ssl_enabled = false;
static std::atomic<bool> g_h2o_running{true};

static constexpr int DEFAULT_H2_PORT = 8080;
static constexpr const char* DEFAULT_HOST = "0.0.0.0";
static constexpr int NUM_H2_THREADS = 4;

static const unsigned char H2_ALPN[] = {2, 'h', '2'};

static SSL_CTX* CreateSslContext(const std::string& crt_path, const std::string& key_path) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        std::cerr << "H2oServer: SSL_CTX_new failed" << std::endl;
        return nullptr;
    }
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    if (SSL_CTX_use_certificate_file(ctx, crt_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "H2oServer: failed to load certificate: " << crt_path << std::endl;
        SSL_CTX_free(ctx);
        return nullptr;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "H2oServer: failed to load private key: " << key_path << std::endl;
        SSL_CTX_free(ctx);
        return nullptr;
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        std::cerr << "H2oServer: private key check failed" << std::endl;
        SSL_CTX_free(ctx);
        return nullptr;
    }
    SSL_CTX_set_alpn_protos(ctx, H2_ALPN);
    memolog::LogInfo("h2o.ssl", "H2oServer TLS context ready, cert=" + crt_path);
    return ctx;
}

static int OnRequest(h2o_handler_t* self, h2o_req_t* req) {
    (void)self;

    Http2Request h2req;
    h2req.method = std::string(req->method.base, req->method.len);
    h2req.path = std::string(req->path.base, req->path.len);

    if (req->query_at != SIZE_MAX) {
        h2req.query = std::string(req->path.base + req->query_at + 1,
                                    req->path.len - req->query_at - 1);
    }

    for (size_t i = 0; i < req->headers.size; ++i) {
        const h2o_header_t* hdr = &req->headers.entries[i];
        std::string name(hdr->name.base, hdr->name.len);
        std::string value(hdr->value.base, hdr->value.len);
        h2req.headers[name] = value;
    }

    auto it_trace = h2req.headers.find("X-Trace-Id");
    if (it_trace != h2req.headers.end()) {
        h2req.trace_id = it_trace->second;
    }

    if (req->entity.base != nullptr && req->entity.len > 0) {
        h2req.body.assign(req->entity.base, req->entity.len);
    }

    if (req->conn && req->conn->peeraddr) {
        char addr_str[256];
        struct sockaddr* sa = req->conn->peeraddr;
        if (sa->sa_family == AF_INET) {
            struct sockaddr_in* s = reinterpret_cast<struct sockaddr_in*>(sa);
            snprintf(addr_str, sizeof(addr_str), "%d.%d.%d.%d",
                s->sin_addr.s_addr & 0xFF,
                (s->sin_addr.s_addr >> 8) & 0xFF,
                (s->sin_addr.s_addr >> 16) & 0xFF,
                (s->sin_addr.s_addr >> 24) & 0xFF);
        } else {
            snprintf(addr_str, sizeof(addr_str), "unknown");
        }
        h2req.remote_addr = addr_str;
    }

    Http2Response h2resp;
    h2resp.SetHeader("X-Trace-Id", h2req.trace_id);
    Http2Routes::HandleRequest(h2req, h2resp);

    h2o_res_t res;
    h2o_init_response(&res, req);
    res.status = h2resp.status_code;
    res.reason = h2resp.status_message.c_str();
    res.content_type = h2resp.content_type.c_str();

    for (const auto& h : h2resp.headers) {
        h2o_add_header_by_str(&res.pool, &res.headers, H2O_STRLIT(h.first.c_str()),
                               0, nullptr, h.second.c_str(), h.second.size());
    }

    if (!h2o_start_response(req, &res)) {
        return 0;
    }

    if (!h2resp.body.empty()) {
        h2o_send(req, h2resp.body.c_str(), h2resp.body.size(), H2O_SEND_STATE_FINAL);
    } else {
        h2o_send(req, "", 0, H2O_SEND_STATE_FINAL);
    }

    return 0;
}

static void ThreadMain(size_t thread_id) {
    h2o_eventloop_t* loop = h2o_evloop_create();
    h2o_context_t ctx;
    h2o_context_init(&ctx, loop, &g_global_config);
    h2o_socket_pool_register(&ctx->pool, nullptr, H2O_SOCKETPOOL_TYPE_NONE);

    while (g_h2o_running) {
        h2o_evloop_run(loop, 100);
    }

    h2o_context_free(&ctx);
    h2o_evloop_destroy(loop);
}

static h2o_socket_t* CreateListener(int port) {
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(DEFAULT_HOST, port_str, &hints, &res) != 0) {
        memolog::LogWarn("h2o.listen.fail", "H2oServer getaddrinfo failed for port " + std::to_string(port));
        return nullptr;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        return nullptr;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(fd, res->ai_addr, (int)res->ai_addrlen) < 0) {
        memolog::LogWarn("h2o.listen.fail", "H2oServer bind failed on port " + std::to_string(port));
        closesocket(fd);
        freeaddrinfo(res);
        return nullptr;
    }

    if (listen(fd, 128) < 0) {
        closesocket(fd);
        freeaddrinfo(res);
        return nullptr;
    }

    freeaddrinfo(res);

    h2o_socket_t* sock = h2o_socket_create(fd, [](h2o_socket_t*) {});
    memolog::LogInfo("h2o.listen", "H2oServer listening on " + std::string(DEFAULT_HOST) + ":" + std::to_string(port));
    return sock;
}

// ── Singleton implementation ────────────────────────────────────────────────

H2oServer& H2oServer::GetInstance() {
    static H2oServer instance;
    return instance;
}

H2oServer::H2oServer() = default;

H2oServer::~H2oServer() {
    Stop();
}

bool H2oServer::Initialize() {
    if (g_h2o_initialized) return true;

    Http2Routes::RegisterRoutes();

    h2o_config_init(&g_global_config);
    g_global_config.max_connections = 100000;
    g_global_config.http2.max_concurrent_streams = 100;
    g_global_config.http2.idle_timeout = 60000;

    g_host_config = h2o_config_register_host(&g_global_config, "default", 65535);
    if (!g_host_config) {
        memolog::LogWarn("h2o.init.fail", "H2oServer failed to register host");
        return false;
    }

    h2o_handler_t* handler = h2o_create_handler(
        h2o_get_hostconf(&g_global_config, H2O_STRLIT("default"), 1)->path,
        sizeof(h2o_handler_t));
    handler->on_req = OnRequest;
    h2o_register_handler(g_host_config, handler);

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    std::filesystem::path exe_dir = std::filesystem::current_path();
    std::string crt_path = (exe_dir / "server.crt").string();
    std::string key_path = (exe_dir / "server.key").string();

    if (!std::filesystem::exists(crt_path) || !std::filesystem::exists(key_path)) {
        memolog::LogInfo("h2o.ssl.gen", "H2oServer generating self-signed cert/key");
        if (!CertUtil::GenerateSelfSignedCertPem(crt_path, key_path)) {
            memolog::LogWarn("h2o.ssl.fail", "H2oServer cert generation failed, running without TLS");
            goto skip_ssl;
        }
    }

    g_ssl_ctx = CreateSslContext(crt_path, key_path);
    if (g_ssl_ctx) {
        h2o_socket_ssl_server_set_conf(
            &g_host_config->hosts,
            g_ssl_ctx,
            crt_path.c_str(),
            key_path.c_str(),
            nullptr, nullptr, nullptr);
        g_ssl_enabled = true;
        memolog::LogInfo("h2o.ssl.ok", "H2oServer HTTPS/HTTP2 enabled on port " + std::to_string(_h2_port));
    } else {
        memolog::LogWarn("h2o.ssl.fail", "H2oServer TLS setup failed");
    }

skip_ssl:
    g_h2o_initialized = true;
    return true;
}

void H2oServer::Run() {
    h2o_socket_t* listener = CreateListener(_h2_port);
    if (!listener) {
        memolog::LogWarn("h2o.listen.fail", "H2oServer failed to create listener on port " + std::to_string(_h2_port));
        return;
    }

    if (g_ssl_enabled) {
        h2o_socket_ssl_server_accept(listener, [](h2o_socket_t* client, h2o_errordesc_t* err) {
            (void)err;
            if (client) {
                h2o_http2_accept(client, &g_global_config);
            }
        });
    } else {
        h2o_http1_accept(listener, &g_global_config);
    }

    memolog::LogInfo("h2o.start", "H2oServer starting event loop with " + std::to_string(NUM_H2_THREADS) + " threads");

    std::vector<std::thread> threads;
    for (int i = 1; i < NUM_H2_THREADS; ++i) {
        threads.emplace_back(ThreadMain, i);
    }

    ThreadMain(0);

    g_h2o_running = false;
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    h2o_config_dispose(&g_global_config);
    if (g_ssl_ctx) {
        SSL_CTX_free(g_ssl_ctx);
        g_ssl_ctx = nullptr;
    }
}

void H2oServer::Stop() {
    g_h2o_running = false;
    if (_thread.joinable()) {
        _thread.join();
    }
}

#endif  // MEMOCHAT_HAVE_H2O