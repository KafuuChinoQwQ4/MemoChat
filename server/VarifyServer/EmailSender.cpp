#include "EmailSender.h"
#include "ConfigMgr.h"
#include "logging/Logger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace {

#ifdef _WIN32
using SocketType = SOCKET;
const SocketType INVALID_SOCKET_VALUE = INVALID_SOCKET;
const decltype(&closesocket) CLOSE_SOCKET = closesocket;
#else
using SocketType = int;
const SocketType INVALID_SOCKET_VALUE = -1;
const int CLOSE_SOCKET = ::close;
#endif

int sock_errno() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

std::string b64_encode(const std::string& input) {
    BIO* bio = nullptr;
    BIO* b64 = nullptr;
    BUF_MEM* buffer_ptr = nullptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.data(), static_cast<int>(input.size()));
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);

    std::string result(buffer_ptr->data, buffer_ptr->length);
    BIO_free_all(bio);
    return result;
}

void b64_encode_stream(std::ostream& out, const std::string& input) {
    BIO* bio = nullptr;
    BIO* b64 = nullptr;
    BUF_MEM* buffer_ptr = nullptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.data(), static_cast<int>(input.size()));
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);
    out.write(buffer_ptr->data, buffer_ptr->length);
    BIO_free_all(bio);
}

bool send_all(SocketType sock, const char* data, int len) {
    int sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int n = ::send(sock, data + sent, len - sent, 0);
#else
        int n = ::send(sock, data + sent, len - sent, 0);
#endif
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

bool recv_line(SocketType sock, std::string& line) {
    line.clear();
    char buf[2] = {0, 0};
    while (true) {
#ifdef _WIN32
        int n = ::recv(sock, buf, 1, 0);
#else
        int n = ::recv(sock, buf, 1, 0);
#endif
        if (n <= 0) return false;
        if (buf[0] == '\n') break;
        line += buf[0];
    }
    return true;
}

bool expect_code(SocketType sock, int expected_code) {
    std::string line;
    if (!recv_line(sock, line)) return false;
    if (line.size() < 3) return false;
    int code = 0;
    try {
        code = std::stoi(line.substr(0, 3));
    } catch (...) {
        return false;
    }
    return code == expected_code;
}

bool send_command(SocketType sock, const std::string& cmd) {
    std::string full = cmd + "\r\n";
    return send_all(sock, full.data(), static_cast<int>(full.size()));
}

} // anonymous namespace

namespace varifyservice {

bool EmailSender::Send(const std::string& to_email, const std::string& code) {
    auto& cfg = ConfigMgr::Inst();

    std::string host = cfg["Email"]["SMTPHost"];
    std::string port_str = cfg["Email"]["SMTPPort"];
    std::string user = cfg["Email"]["SMTPUser"];
    std::string pass = cfg["Email"]["SMTPPass"];
    std::string from = cfg["Email"]["From"];

    int port = 465;
    if (!port_str.empty()) {
        try { port = std::stoi(port_str); } catch (...) {}
    }

    bool use_ssl = (port == 465);

    memolog::LogInfo("varify.email.send_start", "sending email",
                     {{"to", to_email}, {"smtp_host", host}, {"smtp_port", std::to_string(port)}});

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        memolog::LogError("varify.email.send_failed", "WSAStartup failed",
                         {{"error", std::to_string(WSAGetLastError())}});
        return false;
    }
#endif

    SocketType sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_VALUE) {
        memolog::LogError("varify.email.send_failed", "socket creation failed",
                         {{"error", std::to_string(sock_errno())}});
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    struct hostent* he = gethostbyname(host.c_str());
    if (!he) {
        memolog::LogError("varify.email.send_failed", "DNS lookup failed",
                         {{"host", host}, {"error", std::to_string(sock_errno())}});
        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(static_cast<u_short>(port));
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], static_cast<size_t>(he->h_length));

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0) {
        memolog::LogError("varify.email.send_failed", "connect failed",
                         {{"host", host}, {"port", std::to_string(port)},
                          {"error", std::to_string(sock_errno())}});
        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    if (use_ssl) {
#ifdef _WIN32
        SSL_library_init();
        SSL_load_error_strings();
        const SSL_METHOD* method = TLSv1_2_client_method();
#else
        SSL_library_init();
        SSL_load_error_strings();
        const SSL_METHOD* method = TLS_client_method();
#endif
        SSL_CTX* ctx = SSL_CTX_new(method);
        if (!ctx) {
            memolog::LogError("varify.email.send_failed", "SSL_CTX_new failed");
            CLOSE_SOCKET(sock);
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        SSL* ssl = SSL_new(ctx);
        if (!ssl) {
            memolog::LogError("varify.email.send_failed", "SSL_new failed");
            SSL_CTX_free(ctx);
            CLOSE_SOCKET(sock);
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        SSL_set_fd(ssl, static_cast<int>(sock));
        if (SSL_connect(ssl) != 1) {
            memolog::LogError("varify.email.send_failed", "SSL_connect failed",
                             {{"error", std::to_string(sock_errno())}});
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            CLOSE_SOCKET(sock);
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        auto ssl_send = [&ssl](const char* data, int len) -> int {
            return SSL_write(ssl, data, len);
        };
        auto ssl_recv_line = [&ssl](std::string& line) -> bool {
            line.clear();
            char buf[2] = {0, 0};
            while (true) {
                int n = SSL_read(ssl, buf, 1);
                if (n <= 0) return false;
                if (buf[0] == '\n') break;
                line += buf[0];
            }
            return true;
        };
        auto ssl_expect_code = [&ssl_recv_line](int expected_code) -> bool {
            std::string line;
            if (!ssl_recv_line(line)) return false;
            if (line.size() < 3) return false;
            int code = 0;
            try { code = std::stoi(line.substr(0, 3)); } catch (...) { return false; }
            return code == expected_code;
        };
        auto ssl_send_cmd = [&ssl_send](const std::string& cmd) -> bool {
            std::string full = cmd + "\r\n";
            return ssl_send(full.data(), static_cast<int>(full.size())) > 0;
        };

        bool ok = ssl_expect_code(220);
        if (!ok) {
            memolog::LogError("varify.email.send_failed", "SMTP greeting failed (SSL)");
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            CLOSE_SOCKET(sock);
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        ok = ok && ssl_send_cmd("EHLO localhost");
        ok = ok && ssl_expect_code(250);

        ok = ok && ssl_send_cmd("AUTH LOGIN");
        ok = ok && ssl_expect_code(334);

        std::string user_b64 = b64_encode(user);
        ok = ok && ssl_send_cmd(user_b64);
        ok = ok && ssl_expect_code(334);

        std::string pass_b64 = b64_encode(pass);
        ok = ok && ssl_send_cmd(pass_b64);
        ok = ok && ssl_expect_code(235);

        ok = ok && ssl_send_cmd("MAIL FROM:<" + from + ">");
        ok = ok && ssl_expect_code(250);

        ok = ok && ssl_send_cmd("RCPT TO:<" + to_email + ">");
        ok = ok && ssl_expect_code(250);

        ok = ok && ssl_send_cmd("DATA");
        ok = ok && ssl_expect_code(354);

        std::stringstream ss;
        ss << "From: " << from << "\r\n"
           << "To: " << to_email << "\r\n"
           << "Subject: =?UTF-8?B?";
        b64_encode_stream(ss, std::string("\xe9\xaa\x8c\xe8\xaf\x81\xe7\xa0\x81"));
        ss << "?=\r\n"
           << "Content-Type: text/plain; charset=UTF-8\r\n"
           << "\r\n"
           << "\xe6\x82\xa8\xe7\x9a\x84\xe9\xaa\x8c\xe8\xaf\x81\xe7\xa0\x81\xe4\xb8\xba"
           << code
           << "\xe8\xaf\xb7\xe4\xb8\x89\xe5\x88\x86\xe9\x92\x9f\xe5\x86\x85\xe5\xae\x8c\xe6\x88\x90\xe6\xb3\xa8\xe5\x86\x8c\r\n"
           << ".\r\n";
        std::string body = ss.str();
        ok = ok && (ssl_send(body.data(), static_cast<int>(body.size())) > 0);
        ok = ok && ssl_expect_code(250);

        ssl_send_cmd("QUIT");
        ssl_expect_code(221);

        SSL_free(ssl);
        SSL_CTX_free(ctx);
    } else {
        if (!expect_code(sock, 220)) {
            memolog::LogError("varify.email.send_failed", "SMTP greeting failed");
            CLOSE_SOCKET(sock);
#ifdef _WIN32
            WSACleanup();
#endif
            return false;
        }

        bool ok = send_command(sock, "EHLO localhost");
        ok = ok && expect_code(sock, 250);

        ok = ok && send_command(sock, "STARTTLS");
        ok = ok && expect_code(sock, 220);

        {
            SSL_library_init();
            SSL_load_error_strings();
            const SSL_METHOD* method = TLS_client_method();
            SSL_CTX* ctx = SSL_CTX_new(method);
            SSL* ssl = SSL_new(ctx);
            SSL_set_fd(ssl, static_cast<int>(sock));
            SSL_connect(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
        }

        ok = ok && send_command(sock, "EHLO localhost");
        ok = ok && expect_code(sock, 250);

        ok = ok && send_command(sock, "AUTH LOGIN");
        ok = ok && expect_code(sock, 334);
        ok = ok && send_command(sock, b64_encode(user));
        ok = ok && expect_code(sock, 334);
        ok = ok && send_command(sock, b64_encode(pass));
        ok = ok && expect_code(sock, 235);

        ok = ok && send_command(sock, "MAIL FROM:<" + from + ">");
        ok = ok && expect_code(sock, 250);
        ok = ok && send_command(sock, "RCPT TO:<" + to_email + ">");
        ok = ok && expect_code(sock, 250);
        ok = ok && send_command(sock, "DATA");
        ok = ok && expect_code(sock, 354);

        std::stringstream ss;
        ss << "From: " << from << "\r\n"
           << "To: " << to_email << "\r\n"
           << "Subject: =?UTF-8?B?";
        b64_encode_stream(ss, std::string("\xe9\xaa\x8c\xe8\xaf\x81\xe7\xa0\x81"));
        ss << "?=\r\n"
           << "Content-Type: text/plain; charset=UTF-8\r\n"
           << "\r\n"
           << "\xe6\x82\xa8\xe7\x9a\x84\xe9\xaa\x8c\xe8\xaf\x81\xe7\xa0\x81\xe4\xb8\xba"
           << code
           << "\xe8\xaf\xb7\xe4\xb8\x89\xe5\x88\x86\xe9\x92\x9f\xe5\x86\x85\xe5\xae\x8c\xe6\x88\x90\xe6\xb3\xa8\xe5\x86\x8c\r\n"
           << ".\r\n";
        std::string body = ss.str();
        ok = ok && send_all(sock, body.data(), static_cast<int>(body.size()));
        ok = ok && expect_code(sock, 250);

        send_command(sock, "QUIT");
        expect_code(sock, 221);
    }

    CLOSE_SOCKET(sock);
#ifdef _WIN32
    WSACleanup();
#endif

    memolog::LogInfo("varify.email.send_ok", "email sent",
                    {{"to", to_email}});
    return true;
}

} // namespace varifyservice
