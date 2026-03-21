#include "client.hpp"

#include "logger.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include <chrono>
#include <thread>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")

// ws2tcpip.h / windows.h / winhttp.h define min/max/DEBUG macros — undef before STL usage
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef DEBUG
#undef DEBUG
#endif

namespace memochat {
namespace loadtest {

// =====================================================================
// HttpSessionPool — reuses WinHTTP session+connect handles
// (Handle struct is defined in client.hpp)
// =====================================================================

HttpSessionPool::HttpSessionPool(int pool_size) : pool_size_(pool_size) {
    handles_.resize(pool_size_);
}

HttpSessionPool::~HttpSessionPool() {
    for (auto& h : handles_) {
        if (h.connect) WinHttpCloseHandle(h.connect);
        if (h.session) WinHttpCloseHandle(h.session);
    }
}

HttpSessionPool::Handle* HttpSessionPool::acquire(const std::string& host, int port) {
    std::lock_guard<std::mutex> lk(mu_);

    // First pass: find an existing free handle for the same host:port
    for (auto& h : handles_) {
        if (!h.in_use && h.session && h.host == host && h.port == port) {
            h.in_use = true;
            return &h;
        }
    }

    // Second pass: find any free slot and create a new session
    for (auto& h : handles_) {
        if (!h.in_use) {
            if (h.session) {
                if (h.connect) { WinHttpCloseHandle(h.connect); h.connect = nullptr; }
                WinHttpCloseHandle(h.session);
            }
            std::wstring w_host(host.begin(), host.end());
            h.session = WinHttpOpen(
                L"MemoChatLoadTest/1.0",
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME,
                WINHTTP_NO_PROXY_BYPASS,
                0);
            if (!h.session) {
                h.session = nullptr;
                return nullptr;
            }
            h.connect = WinHttpConnect(h.session, w_host.c_str(), (INTERNET_PORT)port, 0);
            if (!h.connect) {
                WinHttpCloseHandle(h.session);
                h.session = nullptr;
                return nullptr;
            }
            h.host = host;
            h.port = port;
            h.in_use = true;
            return &h;
        }
    }

    // Pool exhausted — create a temporary handle (not pooled)
    // Fall back to old behavior for overflow
    return nullptr;
}

void HttpSessionPool::release(Handle* h) {
    if (!h) return;
    std::lock_guard<std::mutex> lk(mu_);
    h->in_use = false;
}

static HttpSessionPool* g_http_pool = nullptr;

HttpSessionPool& http_pool() {
    if (!g_http_pool) {
        // Use a pool size of 200 to handle 200+ concurrent workers
        g_http_pool = new HttpSessionPool(200);
    }
    return *g_http_pool;
}

// =====================================================================
// Utilities
// =====================================================================
std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char ch : s) {
        if (ch == '"') out += "\\\"";
        else if (ch == '\\') out += "\\\\";
        else if (ch == '\n') out += "\\n";
        else if (ch == '\r') out += "\\r";
        else if (ch == '\t') out += "\\t";
        else out += ch;
    }
    return out;
}

std::string make_trace_id() {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << (rand() & 0xFFFFFFFF)
       << std::setw(8) << (rand() & 0xFFFFFFFF);
    return ss.str();
}

// =====================================================================
// Pooled HTTP POST — uses session pool for connection reuse
// =====================================================================
HttpResponse http_post_json_pooled(const std::string& url,
                                   const std::string& body_json,
                                   double timeout_sec,
                                   const std::string& trace_id) {
    HttpResponse rsp;

    // Parse URL
    std::string host, path;
    int port = 80;
    bool use_ssl = false;
    {
        std::string u = url;
        if (u.find("https://") == 0) {
            use_ssl = true;
            port = 443;
            u = u.substr(8);
        } else if (u.find("http://") == 0) {
            u = u.substr(7);
        }

        auto slash = u.find('/');
        if (slash == std::string::npos) {
            host = u;
            path = "/";
        } else {
            host = u.substr(0, slash);
            path = u.substr(slash);
        }

        auto colon = host.find(':');
        if (colon != std::string::npos) {
            port = std::stoi(host.substr(colon + 1));
            host = host.substr(0, colon);
        }
    }

    DWORD timeout_ms = (DWORD)(timeout_sec * 1000);
    HttpSessionPool::Handle* h = nullptr;
    HINTERNET request = nullptr;

    // Try pool first
    h = http_pool().acquire(host, port);
    bool use_pool = (h != nullptr);

    if (!use_pool) {
        // Pool exhausted — create temporary session (slow path)
        std::wstring w_host(host.begin(), host.end());
        HINTERNET session = WinHttpOpen(
            L"MemoChatLoadTest/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
        if (!session) {
            rsp.error = "WinHttpOpen failed";
            return rsp;
        }
        HINTERNET connect = WinHttpConnect(session, w_host.c_str(), (INTERNET_PORT)port, 0);
        if (!connect) {
            WinHttpCloseHandle(session);
            rsp.error = "WinHttpConnect failed";
            return rsp;
        }

        DWORD flags = use_ssl ? WINHTTP_FLAG_SECURE : 0;
        request = WinHttpOpenRequest(connect, L"POST",
            std::wstring(path.begin(), path.end()).c_str(),
            nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!request) {
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            rsp.error = "WinHttpOpenRequest failed";
            return rsp;
        }

        WinHttpSetTimeouts(request, timeout_ms, timeout_ms, timeout_ms, timeout_ms);

        std::wstring headers = L"Content-Type: application/json";
        if (!trace_id.empty()) {
            headers += L"\r\nX-Trace-Id: ";
            headers += std::wstring(trace_id.begin(), trace_id.end());
        }

        std::vector<char> body(body_json.begin(), body_json.end());
        BOOL ok = WinHttpSendRequest(request, headers.c_str(), headers.length(),
            body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
        if (!ok) {
            rsp.error = "WinHttpSendRequest failed: " + std::to_string(GetLastError());
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return rsp;
        }

        ok = WinHttpReceiveResponse(request, nullptr);
        if (!ok) {
            rsp.error = "WinHttpReceiveResponse failed: " + std::to_string(GetLastError());
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return rsp;
        }

        DWORD status_code = 0;
        DWORD status_code_len = sizeof(status_code);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &status_code_len, WINHTTP_NO_HEADER_INDEX);
        rsp.status_code = (int)status_code;

        std::string response_body;
        DWORD size = 0;
        while (WinHttpQueryDataAvailable(request, &size) && size > 0) {
            std::vector<char> buf((size_t)size + 1);
            DWORD downloaded = 0;
            if (WinHttpReadData(request, buf.data(), size, &downloaded)) {
                buf[downloaded] = '\0';
                response_body += buf.data();
            } else break;
            size = 0;
        }
        rsp.body = response_body;

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return rsp;
    }

    // ---- Fast path: use pooled session ----
    DWORD flags = use_ssl ? WINHTTP_FLAG_SECURE : 0;
    request = WinHttpOpenRequest(h->connect, L"POST",
        std::wstring(path.begin(), path.end()).c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        rsp.error = "WinHttpOpenRequest failed: " + std::to_string(GetLastError());
        http_pool().release(h);
        return rsp;
    }

    WinHttpSetTimeouts(request, timeout_ms, timeout_ms, timeout_ms, timeout_ms);

    std::wstring headers = L"Content-Type: application/json";
    if (!trace_id.empty()) {
        headers += L"\r\nX-Trace-Id: ";
        headers += std::wstring(trace_id.begin(), trace_id.end());
    }

    std::vector<char> body(body_json.begin(), body_json.end());
    BOOL ok = WinHttpSendRequest(request, headers.c_str(), headers.length(),
        body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);

    if (!ok) {
        rsp.error = "WinHttpSendRequest failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(request);
        http_pool().release(h);
        return rsp;
    }

    ok = WinHttpReceiveResponse(request, nullptr);
    if (!ok) {
        rsp.error = "WinHttpReceiveResponse failed: " + std::to_string(GetLastError());
        WinHttpCloseHandle(request);
        http_pool().release(h);
        return rsp;
    }

    DWORD status_code = 0;
    DWORD status_code_len = sizeof(status_code);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &status_code_len, WINHTTP_NO_HEADER_INDEX);
    rsp.status_code = (int)status_code;

    std::string response_body;
    DWORD size = 0;
    while (WinHttpQueryDataAvailable(request, &size) && size > 0) {
        std::vector<char> buf((size_t)size + 1);
        DWORD downloaded = 0;
        if (WinHttpReadData(request, buf.data(), size, &downloaded)) {
            buf[downloaded] = '\0';
            response_body += buf.data();
        } else break;
        size = 0;
    }
    rsp.body = response_body;

    WinHttpCloseHandle(request);
    http_pool().release(h);

    return rsp;
}

// =====================================================================
// Original session-per-request HTTP POST (kept for compatibility)
// =====================================================================
HttpResponse http_post_json(const std::string& url,
                            const std::string& body_json,
                            double timeout_sec,
                            const std::string& trace_id) {
    HttpResponse rsp;

    std::string host, path;
    int port = 80;
    bool use_ssl = false;

    {
        std::string u = url;
        if (u.find("https://") == 0) {
            use_ssl = true;
            port = 443;
            u = u.substr(8);
        } else if (u.find("http://") == 0) {
            u = u.substr(7);
        }

        auto slash = u.find('/');
        if (slash == std::string::npos) {
            host = u;
            path = "/";
        } else {
            host = u.substr(0, slash);
            path = u.substr(slash);
        }

        auto colon = host.find(':');
        if (colon != std::string::npos) {
            port = std::stoi(host.substr(colon + 1));
            host = host.substr(0, colon);
        }
    }

    DWORD timeout_ms = (DWORD)(timeout_sec * 1000);

    HINTERNET session = WinHttpOpen(
        L"MemoChatLoadTest/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!session) {
        rsp.error = "WinHttpOpen failed";
        return rsp;
    }

    HINTERNET connect = WinHttpConnect(
        session,
        std::wstring(host.begin(), host.end()).c_str(),
        (INTERNET_PORT)port,
        0);
    if (!connect) {
        WinHttpCloseHandle(session);
        rsp.error = "WinHttpConnect failed";
        return rsp;
    }

    DWORD flags = use_ssl ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect,
        L"POST",
        std::wstring(path.begin(), path.end()).c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        rsp.error = "WinHttpOpenRequest failed";
        return rsp;
    }

    WinHttpSetTimeouts(request, timeout_ms, timeout_ms, timeout_ms, timeout_ms);

    std::wstring headers = L"Content-Type: application/json";
    if (!trace_id.empty()) {
        headers += L"\r\nX-Trace-Id: ";
        headers += std::wstring(trace_id.begin(), trace_id.end());
    }

    std::vector<char> body(body_json.begin(), body_json.end());
    BOOL ok = WinHttpSendRequest(request,
        headers.c_str(), headers.length(),
        body.data(), (DWORD)body.size(),
        (DWORD)body.size(), 0);

    if (!ok) {
        DWORD err = GetLastError();
        rsp.error = "WinHttpSendRequest failed: " + std::to_string(err);
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return rsp;
    }

    ok = WinHttpReceiveResponse(request, nullptr);
    if (!ok) {
        DWORD err = GetLastError();
        rsp.error = "WinHttpReceiveResponse failed: " + std::to_string(err);
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return rsp;
    }

    DWORD status_code = 0;
    DWORD status_code_len = sizeof(status_code);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &status_code_len, WINHTTP_NO_HEADER_INDEX);

    rsp.status_code = (int)status_code;

    std::string response_body;
    DWORD size = 0;
    while (WinHttpQueryDataAvailable(request, &size) && size > 0) {
        std::vector<char> buf((size_t)size + 1);
        DWORD downloaded = 0;
        if (WinHttpReadData(request, buf.data(), size, &downloaded)) {
            buf[downloaded] = '\0';
            response_body += buf.data();
        } else {
            break;
        }
        size = 0;
    }

    rsp.body = response_body;

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    return rsp;
}

std::optional<std::unordered_map<std::string, std::string>>
gate_login(const TestConfig& cfg,
           const Account& account,
           const std::string& trace_id) {
    std::string passwd = account.password;
    if (cfg.use_xor_passwd) {
        passwd = xor_encode(passwd);
    }

    std::ostringstream body_ss;
    body_ss << "{\"email\":\"" << json_escape(account.email) << "\","
            << "\"passwd\":\"" << json_escape(passwd) << "\","
            << "\"client_ver\":\"" << json_escape(cfg.client_ver) << "\"}";

    HttpResponse rsp = http_post_json_pooled(
        cfg.gate_url + cfg.login_path,
        body_ss.str(),
        cfg.http_timeout_sec,
        trace_id);

    if (!rsp.success()) {
        return std::nullopt;
    }

    // Parse JSON response
    std::optional<std::unordered_map<std::string, std::string>> result;

    // Simple JSON parser: extract known fields
    auto extract_str = [&](const std::string& json, const std::string& key) -> std::string {
        std::string pattern = "\"" + key + "\"";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return "";
        size_t colon = json.find(':', pos);
        if (colon == std::string::npos) return "";
        size_t quote1 = json.find('"', colon + 1);
        if (quote1 == std::string::npos) return "";
        size_t quote2 = json.find('"', quote1 + 1);
        if (quote2 == std::string::npos) return "";
        return json.substr(quote1 + 1, quote2 - quote1 - 1);
    };

    auto extract_int = [&](const std::string& json, const std::string& key) -> int {
        std::string pattern = "\"" + key + "\"";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return 0;
        size_t colon = json.find(':', pos);
        if (colon == std::string::npos) return 0;
        size_t end = colon + 1;
        while (end < json.size() && (json[end] == ' ' || json[end] == '\t')) ++end;
        size_t comma = json.find(',', end);
        size_t brace = json.find('}', end);
        size_t tok_end = std::min(comma, brace);
        std::string token = json.substr(end, tok_end - end);
        return std::stoi(token);
    };

    std::unordered_map<std::string, std::string> fields;
    fields["email"] = account.email;
    fields["uid"] = std::to_string(extract_int(rsp.body, "uid"));
    fields["token"] = extract_str(rsp.body, "token");
    fields["login_ticket"] = extract_str(rsp.body, "login_ticket");
    fields["host"] = extract_str(rsp.body, "host");
    fields["port"] = std::to_string(extract_int(rsp.body, "port"));
    fields["server_name"] = extract_str(rsp.body, "server_name");
    fields["error"] = std::to_string(extract_int(rsp.body, "error"));

    // Extract chat_endpoints if present
    size_t ep_pos = rsp.body.find("\"chat_endpoints\"");
    if (ep_pos != std::string::npos) {
        // Look for transport=quic endpoint
        size_t quic_pos = rsp.body.find("\"transport\":\"quic\"", ep_pos);
        if (quic_pos != std::string::npos && quic_pos < ep_pos + 2048) {
            // Extract host/port from this endpoint
            std::string sub = rsp.body.substr(quic_pos);
            fields["quic_host"] = extract_str(sub, "host");
            std::string port_str = std::to_string(extract_int(sub, "port"));
            if (!fields["quic_host"].empty() && !port_str.empty() && port_str != "0") {
                fields["quic_port"] = port_str;
            }
        }
    }

    int error_code = std::stoi(fields["error"]);
    if (error_code != 0) {
        return std::nullopt;
    }

    return fields;
}

// ---- TCP Chat Client ----
TcpChatClient::TcpChatClient(double timeout_sec) : timeout_sec_(timeout_sec), sock_fd_(-1) {}

TcpChatClient::~TcpChatClient() { close(); }

bool TcpChatClient::connect(const std::string& host, int port) {
    close();

    SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        last_error_ = "socket() failed";
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    DWORD timeout = (DWORD)(timeout_sec_ * 1000);
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    if (::connect(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        last_error_ = "connect() failed: " + std::to_string(WSAGetLastError());
        closesocket(s);
        return false;
    }

    sock_fd_ = (int)s;
    return true;
}

void TcpChatClient::close() {
    if (sock_fd_ >= 0) {
        closesocket(sock_fd_);
        sock_fd_ = -1;
    }
}

bool TcpChatClient::recv_exact(void* buf, size_t n) {
    char* p = (char*)buf;
    while (n > 0) {
        int r = recv(sock_fd_, p, (int)n, 0);
        if (r <= 0) {
            last_error_ = "recv() returned " + std::to_string(r);
            return false;
        }
        p += r;
        n -= r;
    }
    return true;
}

bool TcpChatClient::send_all(const void* buf, size_t n) {
    const char* p = (const char*)buf;
    while (n > 0) {
        int r = send(sock_fd_, p, (int)n, 0);
        if (r <= 0) {
            last_error_ = "send() returned " + std::to_string(WSAGetLastError());
            return false;
        }
        p += r;
        n -= r;
    }
    return true;
}

bool TcpChatClient::send_frame(int msg_id, const std::string& body_json) {
    uint16_t id = (uint16_t)msg_id;
    uint16_t len = (uint16_t)body_json.size();
    unsigned char header[4];
    header[0] = (unsigned char)(id >> 8);
    header[1] = (unsigned char)(id & 0xFF);
    header[2] = (unsigned char)(len >> 8);
    header[3] = (unsigned char)(len & 0xFF);

    if (!send_all(header, 4)) return false;
    if (!body_json.empty()) {
        if (!send_all(body_json.data(), body_json.size())) return false;
    }
    return true;
}

bool TcpChatClient::recv_frame(int expected_msg_id, std::string& out_body, double timeout_sec) {
    unsigned char header[4];
    if (!recv_exact(header, 4)) return false;

    uint16_t msg_id = (uint16_t(header[0]) << 8) | uint16_t(header[1]);
    uint16_t body_len = (uint16_t(header[2]) << 8) | uint16_t(header[3]);

    if ((int)msg_id != expected_msg_id) {
        last_error_ = "unexpected msg_id: " + std::to_string(msg_id) + " expected " + std::to_string(expected_msg_id);
        return false;
    }

    if (body_len > 65535) {
        last_error_ = "body too large: " + std::to_string(body_len);
        return false;
    }

    std::string body(body_len, '\0');
    if (body_len > 0 && !recv_exact(body.data(), body_len)) return false;

    out_body = body;
    return true;
}

int TcpChatClient::chat_login(const std::unordered_map<std::string, std::string>& gate_response,
                               const std::string& trace_id,
                               int protocol_version,
                               double timeout_sec) {
    // Build payload
    std::ostringstream ss;
    ss << "{\"protocol_version\":" << protocol_version
       << ",\"trace_id\":\"" << json_escape(trace_id) << "\"";
    if (protocol_version >= 3) {
        ss << ",\"login_ticket\":\"" << json_escape(gate_response.at("login_ticket")) << "\"";
    } else {
        ss << ",\"uid\":" << gate_response.at("uid")
           << ",\"token\":\"" << json_escape(gate_response.at("token")) << "\"";
    }
    ss << "}";

    std::string payload = ss.str();

    // Send CHAT_LOGIN_REQ (1005)
    if (!send_frame(1005, payload)) {
        return -1;
    }

    // Recv CHAT_LOGIN_RSP (1006)
    std::string rsp_body;
    if (!recv_frame(1006, rsp_body, timeout_sec)) {
        return -1;
    }

    // Parse error code from response
    auto extract_int = [&](const std::string& json, const std::string& key) -> int {
        std::string pattern = "\"" + key + "\"";
        size_t pos = json.find(pattern);
        if (pos == std::string::npos) return -1;
        size_t colon = json.find(':', pos);
        if (colon == std::string::npos) return -1;
        size_t end = colon + 1;
        while (end < json.size() && (json[end] == ' ' || json[end] == '\t')) ++end;
        size_t comma = json.find(',', end);
        size_t brace = json.find('}', end);
        size_t tok_end = std::min(comma, brace);
        return std::stoi(json.substr(end, tok_end - end));
    };

    return extract_int(rsp_body, "error");
}

// ---- Account filtering ----
static HttpResponse probe_login(const TestConfig& cfg,
                                 const Account& account) {
    std::string passwd = account.password;
    if (cfg.use_xor_passwd) passwd = xor_encode(passwd);

    std::ostringstream body_ss;
    body_ss << "{\"email\":\"" << json_escape(account.email) << "\","
            << "\"passwd\":\"" << json_escape(passwd) << "\","
            << "\"client_ver\":\"" << json_escape(cfg.client_ver) << "\"}";

    return http_post_json(
        cfg.gate_url + cfg.login_path,
        body_ss.str(),
        cfg.http_timeout_sec,
        "");
}

std::vector<Account> filter_ready_accounts(const TestConfig& cfg,
                                           const std::vector<Account>& accounts,
                                           double http_timeout_sec) {
    std::vector<Account> ready;

    for (const auto& acc : accounts) {
        HttpResponse rsp = probe_login(cfg, acc);
        if (rsp.success()) {
            auto extract_int = [&](const std::string& json, const std::string& key) -> int {
                std::string pattern = "\"" + key + "\"";
                size_t pos = json.find(pattern);
                if (pos == std::string::npos) return -1;
                size_t colon = json.find(':', pos);
                if (colon == std::string::npos) return -1;
                size_t end = colon + 1;
                while (end < json.size() && (json[end] == ' ' || json[end] == '\t')) ++end;
                size_t comma = json.find(',', end);
                size_t brace = json.find('}', end);
                size_t tok_end = std::min(comma, brace);
                return std::stoi(json.substr(end, tok_end - end));
            };
            int err = extract_int(rsp.body, "error");
            if (err == 0) {
                Account enriched = acc;
                enriched.uid = std::to_string(extract_int(rsp.body, "uid"));
                enriched.user_id = std::to_string(extract_int(rsp.body, "user_id"));
                ready.push_back(enriched);
            }
        }
    }
    return ready;
}

} // namespace loadtest
} // namespace memochat
