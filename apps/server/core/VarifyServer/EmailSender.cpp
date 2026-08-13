#include "EmailSender.hpp"
#include "ConfigMgr.hpp"
#include "logging/Logger.hpp"

#ifdef _WIN32
#include "../common/WinsockCompat.hpp"
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
#include <charconv>
#include <chrono>
#include <cstring>
#include <random>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

import memochat.varify.email_sender_algorithms;

namespace email_sender_modules = memochat::varify::email_sender::modules;

namespace
{

#ifdef _WIN32
using SocketType = SOCKET;
const SocketType INVALID_SOCKET_VALUE = INVALID_SOCKET;
const decltype(&closesocket) CLOSE_SOCKET = closesocket;
#else
using SocketType = int;
const SocketType INVALID_SOCKET_VALUE = -1;
const decltype(&::close) CLOSE_SOCKET = ::close;
#endif

int sock_errno()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

std::string b64_encode(const std::string& input)
{
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

void b64_encode_stream(std::ostream& out, const std::string& input)
{
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

bool send_all(SocketType sock, const char* data, int len)
{
    int sent = 0;
    while (sent < len)
    {
#ifdef _WIN32
        int n = ::send(sock, data + sent, len - sent, 0);
#else
        int n = ::send(sock, data + sent, len - sent, 0);
#endif
        if (n <= 0)
            return false;
        sent += n;
    }
    return true;
}

bool recv_line(SocketType sock, std::string& line)
{
    line.clear();
    char buf[2] = {0, 0};
    while (true)
    {
#ifdef _WIN32
        int n = ::recv(sock, buf, 1, 0);
#else
        int n = ::recv(sock, buf, 1, 0);
#endif
        if (n <= 0)
            return false;
        if (buf[0] == '\n')
            break;
        line += buf[0];
    }
    return true;
}

bool parse_smtp_status_line(const std::string& line, int* code, bool* more_lines)
{
    if (!code || !more_lines || !email_sender_modules::HasStatusCodePrefix(line.size()))
        return false;

    const char* begin = line.data();
    const char* end = begin + 3;
    const auto [ptr, ec] = std::from_chars(begin, end, *code);
    if (ec != std::errc{} || ptr != end)
    {
        return false;
    }

    *more_lines = email_sender_modules::IsMultilineReply(line.size(), line.size() > 3 ? line[3] : '\0');
    return true;
}

bool expect_code(SocketType sock, int expected_code)
{
    bool more_lines = false;
    do
    {
        std::string line;
        if (!recv_line(sock, line))
            return false;

        int code = 0;
        if (!parse_smtp_status_line(line, &code, &more_lines))
            return false;
        if (!email_sender_modules::IsExpectedStatusCode(code, expected_code))
            return false;
    } while (more_lines);

    return true;
}

bool send_command(SocketType sock, const std::string& cmd)
{
    std::string full = cmd + "\r\n";
    return send_all(sock, full.data(), static_cast<int>(full.size()));
}

bool ssl_send_all(SSL* ssl, const char* data, std::size_t len)
{
    std::size_t sent = 0;
    while (sent < len)
    {
        std::size_t written = 0;
        if (SSL_write_ex(ssl, data + sent, len - sent, &written) != 1 || written == 0)
        {
            return false;
        }
        sent += written;
    }
    return true;
}

bool ssl_recv_line(SSL* ssl, std::string& line)
{
    constexpr std::size_t kMaxSmtpReplyLineLength = 16 * 1024;

    line.clear();
    while (line.size() < kMaxSmtpReplyLineLength)
    {
        char character = '\0';
        std::size_t received = 0;
        if (SSL_read_ex(ssl, &character, 1, &received) != 1 || received != 1)
        {
            return false;
        }
        if (character == '\n')
        {
            return true;
        }
        line += character;
    }
    return false;
}

bool ssl_expect_code(SSL* ssl, int expected_code)
{
    bool more_lines = false;
    do
    {
        std::string line;
        if (!ssl_recv_line(ssl, line))
        {
            return false;
        }

        int code = 0;
        if (!parse_smtp_status_line(line, &code, &more_lines) ||
            !email_sender_modules::IsExpectedStatusCode(code, expected_code))
        {
            return false;
        }
    } while (more_lines);

    return true;
}

bool ssl_send_command(SSL* ssl, const std::string& command)
{
    const std::string full = command + "\r\n";
    return ssl_send_all(ssl, full.data(), full.size());
}

bool create_verified_tls_session(SocketType sock, const std::string& host, SSL_CTX** output_ctx, SSL** output_ssl)
{
    if (!output_ctx || !output_ssl)
    {
        return false;
    }
    *output_ctx = nullptr;
    *output_ssl = nullptr;

    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        return false;
    }
    if (SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION) != 1 || SSL_CTX_set_default_verify_paths(ctx) != 1)
    {
        SSL_CTX_free(ctx);
        return false;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);

    SSL* ssl = SSL_new(ctx);
    if (!ssl)
    {
        SSL_CTX_free(ctx);
        return false;
    }
    if (SSL_set_tlsext_host_name(ssl, host.c_str()) != 1 || SSL_set1_host(ssl, host.c_str()) != 1 ||
        SSL_set_fd(ssl, static_cast<int>(sock)) != 1 || SSL_connect(ssl) != 1 ||
        SSL_get_verify_result(ssl) != X509_V_OK)
    {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return false;
    }

    *output_ctx = ctx;
    *output_ssl = ssl;
    return true;
}

bool smtp_transaction(SSL* ssl,
                      const std::string& user,
                      const std::string& pass,
                      const std::string& from,
                      const std::string& to_email,
                      const std::string& code)
{
    bool smtp_ok = ssl_send_command(ssl, "EHLO localhost");
    smtp_ok = smtp_ok && ssl_expect_code(ssl, 250);

    smtp_ok = smtp_ok && ssl_send_command(ssl, "AUTH LOGIN");
    smtp_ok = smtp_ok && ssl_expect_code(ssl, 334);
    smtp_ok = smtp_ok && ssl_send_command(ssl, b64_encode(user));
    smtp_ok = smtp_ok && ssl_expect_code(ssl, 334);
    smtp_ok = smtp_ok && ssl_send_command(ssl, b64_encode(pass));
    smtp_ok = smtp_ok && ssl_expect_code(ssl, 235);

    smtp_ok = smtp_ok && ssl_send_command(ssl, "MAIL FROM:<" + from + ">");
    smtp_ok = smtp_ok && ssl_expect_code(ssl, 250);
    smtp_ok = smtp_ok && ssl_send_command(ssl, "RCPT TO:<" + to_email + ">");
    smtp_ok = smtp_ok && ssl_expect_code(ssl, 250);
    smtp_ok = smtp_ok && ssl_send_command(ssl, "DATA");
    smtp_ok = smtp_ok && ssl_expect_code(ssl, 354);

    std::stringstream body_stream;
    body_stream << "From: " << from << "\r\n"
                << "To: " << to_email << "\r\n"
                << "Subject: =?UTF-8?B?";
    b64_encode_stream(body_stream, std::string("\xe9\xaa\x8c\xe8\xaf\x81\xe7\xa0\x81"));
    body_stream
        << "?=\r\n"
        << "Content-Type: text/plain; charset=UTF-8\r\n"
        << "\r\n"
        << "\xe6\x82\xa8\xe7\x9a\x84\xe9\xaa\x8c\xe8\xaf\x81\xe7\xa0\x81\xe4\xb8\xba" << code
        << "\xe8\xaf\xb7\xe4\xb8\x89\xe5\x88\x86\xe9\x92\x9f\xe5\x86\x85\xe5\xae\x8c\xe6\x88\x90\xe6\xb3\xa8\xe5\x86"
           "\x8c\r\n"
        << ".\r\n";
    const std::string body = body_stream.str();
    smtp_ok = smtp_ok && ssl_send_all(ssl, body.data(), body.size());
    smtp_ok = smtp_ok && ssl_expect_code(ssl, 250);

    if (smtp_ok)
    {
        ssl_send_command(ssl, "QUIT");
        ssl_expect_code(ssl, 221);
    }
    return smtp_ok;
}

} // anonymous namespace

namespace varifyservice
{

bool EmailSender::Send(const std::string& to_email, const std::string& code)
{
    auto& cfg = ConfigMgr::Inst();

    std::string host = cfg["Email"]["SMTPHost"];
    std::string port_str = cfg["Email"]["SMTPPort"];
    std::string user = cfg["Email"]["SMTPUser"];
    std::string pass = cfg["Email"]["SMTPPass"];
    std::string from = cfg["Email"]["From"];

    int port = email_sender_modules::DefaultSmtpPort();
    if (!port_str.empty())
    {
        int configured_port = 0;
        const auto [ptr, ec] = std::from_chars(port_str.data(), port_str.data() + port_str.size(), configured_port);
        if (ec == std::errc{} && ptr == port_str.data() + port_str.size() && configured_port > 0 &&
            configured_port <= 65535)
        {
            port = configured_port;
        }
    }

    bool use_ssl = email_sender_modules::ShouldUseImplicitSsl(port);

    if (host.empty() || user.empty() || pass.empty() || from.empty())
    {
        std::vector<std::string> missing_fields;
        if (host.empty())
        {
            missing_fields.emplace_back("SMTPHost");
        }
        if (user.empty())
        {
            missing_fields.emplace_back("SMTPUser");
        }
        if (pass.empty())
        {
            missing_fields.emplace_back("SMTPPass");
        }
        if (from.empty())
        {
            missing_fields.emplace_back("From");
        }

        std::ostringstream missing;
        for (std::size_t index = 0; index < missing_fields.size(); ++index)
        {
            if (index != 0)
            {
                missing << ",";
            }
            missing << missing_fields[index];
        }

        memolog::LogError("varify.email.config_missing",
                          "SMTP config missing required fields",
                          {{"missing_fields", missing.str()}});
        return false;
    }

    memolog::LogInfo("varify.email.send_start",
                     "sending email",
                     {{"to_email", to_email}, {"smtp_host", host}, {"smtp_port", std::to_string(port)}});

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        memolog::LogError("varify.email.send_failed",
                          "WSAStartup failed",
                          {{"error", std::to_string(WSAGetLastError())}});
        return false;
    }
#endif

    SocketType sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_VALUE)
    {
        memolog::LogError("varify.email.send_failed",
                          "socket creation failed",
                          {{"error", std::to_string(sock_errno())}});
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    struct hostent* he = gethostbyname(host.c_str());
    if (!he)
    {
        memolog::LogError("varify.email.send_failed",
                          "DNS lookup failed",
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

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != 0)
    {
        memolog::LogError("varify.email.send_failed",
                          "connect failed",
                          {{"host", host}, {"port", std::to_string(port)}, {"error", std::to_string(sock_errno())}});
        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    if (!use_ssl && (!expect_code(sock, 220) || !send_command(sock, "EHLO localhost") || !expect_code(sock, 250) ||
                     !send_command(sock, "STARTTLS") || !expect_code(sock, 220)))
    {
        memolog::LogError("varify.email.send_failed", "SMTP STARTTLS negotiation failed");
        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
    if (!create_verified_tls_session(sock, host, &ctx, &ssl))
    {
        memolog::LogError("varify.email.send_failed",
                          "SMTP TLS certificate verification failed",
                          {{"smtp_host", host}, {"smtp_port", std::to_string(port)}});
        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    bool smtp_ok = true;
    if (use_ssl)
    {
        smtp_ok = ssl_expect_code(ssl, 220);
    }
    smtp_ok = smtp_ok && smtp_transaction(ssl, user, pass, from, to_email, code);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);

    if (!smtp_ok)
    {
        memolog::LogError("varify.email.send_failed",
                          "SMTP transaction failed",
                          {{"to_email", to_email}, {"smtp_host", host}, {"smtp_port", std::to_string(port)}});
        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    CLOSE_SOCKET(sock);
#ifdef _WIN32
    WSACleanup();
#endif

    memolog::LogInfo("varify.email.send_ok", "email sent", {{"to_email", to_email}});
    return true;
}

} // namespace varifyservice
