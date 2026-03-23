#pragma once
#ifndef MEMOCHAT_LOADTEST_CLIENT_H
#define MEMOCHAT_LOADTEST_CLIENT_H

#include "config.hpp"
#include "metrics.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>

// winhttp.h / windows.h defines min/max/DEBUG macros — undef before STL usage
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef DEBUG
#undef DEBUG
#endif

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <future>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace memochat {
namespace loadtest {

// ---- HTTP session pool (reuses WinHTTP sessions across requests) ----
class HttpSessionPool {
public:
    struct Handle {
        HINTERNET session = nullptr;
        HINTERNET connect = nullptr;
        std::string host;
        int port = 0;
        bool in_use = false;
    };

    explicit HttpSessionPool(int pool_size = 64);
    ~HttpSessionPool();

    // Disable copy
    HttpSessionPool(const HttpSessionPool&) = delete;
    HttpSessionPool& operator=(const HttpSessionPool&) = delete;

    // Acquire a session for a given host:port (creates or reuses)
    Handle* acquire(const std::string& host, int port);
    void release(Handle* h);

    // Warmup: pre-establish connections to target host:port
    // Returns number of connections successfully warmed up
    int warmup(const std::string& host, int port, int count);

    // Pool statistics
    int pool_size() const { return pool_size_; }
    int available_count() const;
    int in_use_count() const;
    int total_acquires() const { return total_acquires_.load(); }
    int total_releases() const { return total_releases_.load(); }

    // Reset statistics
    void reset_stats();

private:
    std::vector<Handle> handles_;
    std::mutex mu_;
    int pool_size_;
    mutable std::atomic<int> total_acquires_{0};
    mutable std::atomic<int> total_releases_{0};
};

// Global HTTP session pool with configurable size
// Call once at startup with desired pool_size (default: 200)
HttpSessionPool& http_pool(int pool_size = 200);

// Reset global pool (for reconfiguration)
void reset_http_pool();

// ---- HTTP client ----
struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::string error;
    bool success() const { return status_code == 200 && error.empty(); }
};

// Pooled HTTP POST (uses WinHTTP session pool)
HttpResponse http_post_json_pooled(const std::string& url,
                                   const std::string& body_json,
                                   double timeout_sec,
                                   const std::string& trace_id = "");

// Original session-per-request version (kept for compatibility)
HttpResponse http_post_json(const std::string& url,
                            const std::string& body_json,
                            double timeout_sec,
                            const std::string& trace_id = "");

// Gate login via plaintext HTTP/1.1 (gate_url)
std::optional<std::unordered_map<std::string, std::string>>
gate_login(const TestConfig& cfg,
           const Account& account,
           const std::string& trace_id);

// Gate login via HTTPS/HTTP/2 (gate_url_https)
std::optional<std::unordered_map<std::string, std::string>>
gate_login_https(const TestConfig& cfg,
                 const Account& account,
                 const std::string& trace_id);

// ---- TCP chat client ----
class TcpChatClient {
public:
    explicit TcpChatClient(double timeout_sec);
    ~TcpChatClient();

    bool connect(const std::string& host, int port);
    void close();

    bool send_frame(int msg_id, const std::string& body_json);
    bool recv_frame(int expected_msg_id, std::string& out_body, double timeout_sec);

    // Chat login via TCP (msg_id 1005/1006)
    // Returns error_code (0 = success)
    int chat_login(const std::unordered_map<std::string, std::string>& gate_response,
                   const std::string& trace_id,
                   int protocol_version,
                   double timeout_sec);

    const std::string& last_error() const { return last_error_; }

private:
    bool recv_exact(void* buf, size_t n);
    bool send_all(const void* buf, size_t n);

    double timeout_sec_;
    int sock_fd_ = -1;
    std::string last_error_;
};

// ---- QUIC chat client using msquic ----
class QuicChatClient {
public:
    QuicChatClient(const std::string& host, int port,
                   const std::string& server_name,
                   const std::string& alpn,
                   double timeout_sec);
    ~QuicChatClient();

    // Connect and perform QUIC handshake
    bool connect();

    // Send chat login frame (msg_id 1005)
    bool send_chat_login(const std::unordered_map<std::string, std::string>& gate_response,
                         const std::string& trace_id,
                         int protocol_version);

    // Wait for response (msg_id 1006), returns error_code (0 = success)
    int wait_chat_login_response(double timeout_sec);

    void close();

    const std::string& last_error() const;

private:
    std::string host_;
    int port_;
    std::string server_name_;
    std::string alpn_;
    double timeout_sec_;
    std::string last_error_;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ---- Utilities ----
std::string json_escape(const std::string& s);
std::string make_trace_id();
std::string xor_encode(const std::string& raw);
std::vector<Account> filter_ready_accounts(const TestConfig& cfg,
                                            const std::vector<Account>& accounts,
                                            double http_timeout_sec);

// ---- Test result types ----
struct LoginTestResult {
    TestResult base;
    int http_status = 0;
    int chat_error = 0;
    std::string chat_host;
    int chat_port = 0;
    bool used_quic = false;
};

} // namespace loadtest
} // namespace memochat

#endif // MEMOCHAT_LOADTEST_CLIENT_H
