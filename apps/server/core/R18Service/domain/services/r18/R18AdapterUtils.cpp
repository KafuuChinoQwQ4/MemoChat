#include "r18/R18AdapterUtils.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <openssl/evp.h>
#include <openssl/ssl.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

import memochat.r18.adapter_utils_algorithms;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

namespace memochat::r18::detail
{
namespace
{

void SetError(std::string* error, std::string message)
{
    if (error != nullptr)
    {
        *error = std::move(message);
    }
}

bool ConfigureTlsPeerVerification(ssl::context& ctx,
                                  beast::ssl_stream<beast::tcp_stream>& stream,
                                  const std::string& host,
                                  std::string* error)
{
    beast::error_code ec;
    ctx.set_default_verify_paths(ec);
    if (!ec)
    {
        ctx.set_verify_mode(ssl::verify_peer, ec);
    }
    if (!ec)
    {
        stream.set_verify_mode(ssl::verify_peer, ec);
    }
    if (!ec)
    {
        stream.set_verify_callback(ssl::host_name_verification(host), ec);
    }
    if (ec)
    {
        SetError(error, ec.message());
        return false;
    }
    return true;
}

} // namespace

bool ParseUrl(const std::string& url, ParsedUrl* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "URL output pointer is null");
        return false;
    }
    const auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos)
    {
        SetError(error, adapter_utils::modules::MissingSchemeMessage());
        return false;
    }
    ParsedUrl parsed;
    parsed.scheme = url.substr(0, scheme_end);
    const auto authority_begin = scheme_end + 3;
    const auto authority_end = url.find_first_of("/?#", authority_begin);
    std::string authority = authority_end == std::string::npos
                                ? url.substr(authority_begin)
                                : url.substr(authority_begin, authority_end - authority_begin);
    parsed.has_fragment = url.find('#', authority_begin) != std::string::npos;
    const auto userinfo_end = authority.rfind('@');
    parsed.has_userinfo = userinfo_end != std::string::npos;
    if (parsed.has_userinfo)
        authority.erase(0, userinfo_end + 1);

    if (adapter_utils::modules::ShouldUseDefaultTarget(authority_end == std::string::npos))
        parsed.target = adapter_utils::modules::DefaultTarget();
    else
    {
        const auto fragment_begin = url.find('#', authority_end);
        parsed.target = url.substr(authority_end, fragment_begin - authority_end);
        if (!parsed.target.empty() && parsed.target.front() == '?')
            parsed.target.insert(parsed.target.begin(), '/');
    }

    if (!authority.empty() && authority.front() == '[')
    {
        const auto bracket_end = authority.find(']');
        if (bracket_end == std::string::npos)
        {
            SetError(error, adapter_utils::modules::MissingHostMessage());
            return false;
        }
        parsed.host = authority.substr(1, bracket_end - 1);
        if (bracket_end + 1 < authority.size() && authority[bracket_end + 1] == ':')
            parsed.port = authority.substr(bracket_end + 2);
        else
            parsed.port = adapter_utils::modules::SelectDefaultPort(parsed.scheme == "https");
    }
    else if (const auto port_pos = authority.rfind(':'); port_pos != std::string::npos)
    {
        parsed.host = authority.substr(0, port_pos);
        parsed.port = authority.substr(port_pos + 1);
    }
    else
    {
        parsed.host = authority;
        parsed.port = adapter_utils::modules::SelectDefaultPort(parsed.scheme == "https");
    }
    if (adapter_utils::modules::ShouldThrowMissingHost(parsed.host.empty()) || parsed.port.empty())
    {
        SetError(error, adapter_utils::modules::MissingHostMessage());
        return false;
    }
    *out = std::move(parsed);
    return true;
}

bool PreferIpv4Endpoints(const tcp::resolver::results_type& endpoints, std::vector<tcp::endpoint>* ordered)
{
    if (ordered == nullptr)
        return false;
    ordered->clear();
    std::vector<tcp::endpoint> other;
    for (const auto& entry : endpoints)
    {
        const auto ep = entry.endpoint();
        if (ep.address().is_v4())
            ordered->push_back(ep);
        else
            other.push_back(ep);
    }
    ordered->insert(ordered->end(), other.begin(), other.end());
    return !ordered->empty();
}

struct OutboundProxyConfig
{
    bool enabled = false;
    std::string host;
    std::string port;
};

std::string TrimAscii(std::string value)
{
    auto not_space = [](unsigned char ch)
    {
        return !std::isspace(ch);
    };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

bool HostMatchesNoProxy(const std::string& host, const std::string& pattern_raw)
{
    const std::string pattern = TrimAscii(pattern_raw);
    if (pattern.empty())
        return false;
    if (pattern == "*")
        return true;
    std::string host_l = host;
    std::string pat_l = pattern;
    std::transform(host_l.begin(),
                   host_l.end(),
                   host_l.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    std::transform(pat_l.begin(),
                   pat_l.end(),
                   pat_l.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    if (pat_l.rfind("*.", 0) == 0)
    {
        const std::string suffix = pat_l.substr(1); // keep leading '.'
        return host_l.size() >= suffix.size() &&
               host_l.compare(host_l.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    if (!pat_l.empty() && pat_l.front() == '.')
    {
        return host_l.size() >= pat_l.size() && host_l.compare(host_l.size() - pat_l.size(), pat_l.size(), pat_l) == 0;
    }
    return host_l == pat_l;
}

bool HostBypassesProxy(const std::string& host)
{
    if (host.empty())
        return true;
    if (host == "localhost" || host == "127.0.0.1" || host == "::1")
        return true;

    const char* no_proxy_env = std::getenv("no_proxy");
    if (no_proxy_env == nullptr || *no_proxy_env == '\0')
        no_proxy_env = std::getenv("NO_PROXY");
    if (no_proxy_env == nullptr || *no_proxy_env == '\0')
        return false;

    std::string list = no_proxy_env;
    std::size_t begin = 0;
    while (begin <= list.size())
    {
        const std::size_t end = list.find(',', begin);
        const std::string item =
            TrimAscii(end == std::string::npos ? list.substr(begin) : list.substr(begin, end - begin));
        if (HostMatchesNoProxy(host, item))
            return true;
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    return false;
}

bool ParseHttpProxyUrl(const std::string& proxy_url, OutboundProxyConfig* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "proxy output is null");
        return false;
    }
    *out = {};
    const std::string trimmed = TrimAscii(proxy_url);
    if (trimmed.empty())
        return true;

    ParsedUrl parsed;
    if (!ParseUrl(trimmed, &parsed, error))
        return false;
    if (parsed.scheme != "http" && parsed.scheme != "https")
    {
        SetError(error, "only http/https outbound proxies are supported");
        return false;
    }
    if (parsed.has_userinfo)
    {
        SetError(error, "proxy credentials are not supported");
        return false;
    }
    out->enabled = true;
    out->host = parsed.host;
    out->port = parsed.port;
    return true;
}

bool ResolveOutboundProxy(const std::string& target_host, bool is_https, OutboundProxyConfig* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "proxy output is null");
        return false;
    }
    *out = {};
    if (HostBypassesProxy(target_host))
        return true;

    const char* proxy_env = nullptr;
    if (is_https)
    {
        proxy_env = std::getenv("https_proxy");
        if (proxy_env == nullptr || *proxy_env == '\0')
            proxy_env = std::getenv("HTTPS_PROXY");
    }
    if (proxy_env == nullptr || *proxy_env == '\0')
    {
        proxy_env = std::getenv("http_proxy");
        if (proxy_env == nullptr || *proxy_env == '\0')
            proxy_env = std::getenv("HTTP_PROXY");
    }
    if (proxy_env == nullptr || *proxy_env == '\0')
        return true;
    return ParseHttpProxyUrl(proxy_env, out, error);
}

bool ConnectEndpointsPreferIpv4(net::io_context& ioc,
                                beast::tcp_stream& stream,
                                const tcp::resolver::results_type& endpoints,
                                beast::error_code& ec,
                                int timeout_seconds)
{
    std::vector<tcp::endpoint> ordered;
    if (!PreferIpv4Endpoints(endpoints, &ordered))
    {
        ec = net::error::host_not_found;
        return false;
    }

    const auto timeout = std::chrono::seconds(timeout_seconds < 1 ? 1 : timeout_seconds);
    for (const auto& ep : ordered)
    {
        stream.close();
        stream.expires_after(timeout);
        stream.async_connect(ep,
                             [&](const beast::error_code& attempt_ec)
                             {
                                 ec = attempt_ec;
                             });
        ioc.restart();
        ioc.run();
        if (!ec)
            return true;
        stream.close();
    }
    return false;
}

bool ConnectTcpPreferIpv4(beast::tcp_stream& stream,
                          const tcp::resolver::results_type& endpoints,
                          beast::error_code& ec,
                          int timeout_seconds)
{
    auto& ioc = static_cast<net::io_context&>(stream.get_executor().context());
    return ConnectEndpointsPreferIpv4(ioc, stream, endpoints, ec, timeout_seconds);
}

bool ConnectSslPreferIpv4(beast::ssl_stream<beast::tcp_stream>& stream,
                          const tcp::resolver::results_type& endpoints,
                          beast::error_code& ec,
                          int timeout_seconds)
{
    auto& lowest = beast::get_lowest_layer(stream);
    auto& ioc = static_cast<net::io_context&>(lowest.get_executor().context());
    return ConnectEndpointsPreferIpv4(ioc, lowest, endpoints, ec, timeout_seconds);
}

bool HttpConnectThroughProxy(beast::tcp_stream& stream,
                             const OutboundProxyConfig& proxy,
                             const std::string& target_host,
                             const std::string& target_port,
                             beast::error_code& ec,
                             int timeout_seconds,
                             std::string* error)
{
    auto& ioc = static_cast<net::io_context&>(stream.get_executor().context());
    tcp::resolver resolver(ioc);
    const auto endpoints = resolver.resolve(proxy.host, proxy.port, ec);
    if (ec)
    {
        SetError(error, "proxy resolve failed: " + ec.message());
        return false;
    }
    if (!ConnectEndpointsPreferIpv4(ioc, stream, endpoints, ec, timeout_seconds))
    {
        SetError(error, "proxy connect failed: " + ec.message());
        return false;
    }

    const std::string authority = target_host + ":" + target_port;
    http::request<http::empty_body> connect_req{http::verb::connect, authority, 11};
    connect_req.set(http::field::host, authority);
    connect_req.set(http::field::proxy_connection, "keep-alive");
    connect_req.set(http::field::connection, "keep-alive");
    connect_req.set(http::field::user_agent, adapter_utils::modules::DefaultUserAgent());

    stream.expires_after(std::chrono::seconds(timeout_seconds < 1 ? 1 : timeout_seconds));
    http::write(stream, connect_req, ec);
    if (ec)
    {
        SetError(error, "proxy CONNECT write failed: " + ec.message());
        return false;
    }

    beast::flat_buffer buffer;
    http::response_parser<http::empty_body> parser;
    parser.skip(true);
    http::read_header(stream, buffer, parser, ec);
    if (ec)
    {
        SetError(error, "proxy CONNECT read failed: " + ec.message());
        return false;
    }
    const auto& connect_res = parser.get();
    if (connect_res.result_int() < 200 || connect_res.result_int() >= 300)
    {
        SetError(error, "proxy CONNECT HTTP " + std::to_string(connect_res.result_int()));
        ec = http::error::bad_status;
        return false;
    }
    buffer.consume(buffer.size());
    return true;
}

bool EstablishOutboundTcp(beast::tcp_stream& stream,
                          const ParsedUrl& target,
                          bool is_https,
                          int timeout_seconds,
                          beast::error_code& ec,
                          std::string* error)
{
    OutboundProxyConfig proxy;
    if (!ResolveOutboundProxy(target.host, is_https, &proxy, error))
        return false;

    if (proxy.enabled)
    {
        if (!HttpConnectThroughProxy(stream, proxy, target.host, target.port, ec, timeout_seconds, error))
            return false;
        return true;
    }

    auto& ioc = static_cast<net::io_context&>(stream.get_executor().context());
    tcp::resolver resolver(ioc);
    const auto endpoints = resolver.resolve(target.host, target.port, ec);
    if (ec)
    {
        SetError(error, ec.message());
        return false;
    }
    if (!ConnectEndpointsPreferIpv4(ioc, stream, endpoints, ec, timeout_seconds))
    {
        SetError(error, ec.message());
        return false;
    }
    return true;
}

bool HttpGet(const std::string& url,
             const std::vector<std::pair<std::string, std::string>>& headers,
             HttpResult* out,
             std::string* error,
             int timeout_seconds)
{
    if (out == nullptr)
    {
        SetError(error, "HTTP output pointer is null");
        return false;
    }
    ParsedUrl parsed;
    if (!ParseUrl(url, &parsed, error))
    {
        return false;
    }
    const bool is_https = adapter_utils::modules::IsHttpsScheme(parsed.scheme == "https");
    const bool is_http = adapter_utils::modules::IsHttpScheme(parsed.scheme == "http");
    if (!is_https && !is_http)
    {
        SetError(error, adapter_utils::modules::UnsupportedSchemeMessage());
        return false;
    }
    net::io_context ioc;

    http::request<http::empty_body> req{http::verb::get, parsed.target, 11};
    req.set(http::field::host, parsed.host);
    req.set(http::field::user_agent, adapter_utils::modules::DefaultUserAgent());
    req.set(http::field::accept_encoding, "identity");
    for (const auto& [key, value] : headers)
        req.set(key, value);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    beast::error_code ec;
    if (is_https)
    {
        SSL_CTX* native_context = SSL_CTX_new(TLS_client_method());
        if (native_context == nullptr)
        {
            SetError(error, "failed to create TLS context");
            return false;
        }
        ssl::context ctx(native_context);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
        if (!ConfigureTlsPeerVerification(ctx, stream, parsed.host, error))
        {
            return false;
        }
        if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str()))
        {
            SetError(error, "failed to set TLS SNI host");
            return false;
        }
        if (!EstablishOutboundTcp(beast::get_lowest_layer(stream), parsed, true, timeout_seconds, ec, error))
        {
            return false;
        }
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds < 1 ? 1 : timeout_seconds));
        stream.handshake(ssl::stream_base::client, ec);
        if (!ec)
        {
            http::write(stream, req, ec);
        }
        if (!ec)
        {
            http::read(stream, buffer, res, ec);
        }
        if (ec)
        {
            SetError(error, ec.message());
            return false;
        }
        beast::error_code shutdown_error;
        stream.shutdown(shutdown_error);
    }
    else
    {
        beast::tcp_stream stream(ioc);
        if (!EstablishOutboundTcp(stream, parsed, false, timeout_seconds, ec, error))
        {
            return false;
        }
        stream.expires_after(std::chrono::seconds(timeout_seconds < 1 ? 1 : timeout_seconds));
        http::write(stream, req, ec);
        if (!ec)
        {
            http::read(stream, buffer, res, ec);
        }
        if (ec)
        {
            SetError(error, ec.message());
            return false;
        }
        beast::error_code shutdown_error;
        stream.socket().shutdown(tcp::socket::shutdown_both, shutdown_error);
    }
    HttpResult result;
    result.status = res.result_int();
    result.body = std::move(res.body());
    auto ct = res.find(http::field::content_type);
    if (ct != res.end())
        result.content_type.assign(ct->value().data(), ct->value().size());
    *out = std::move(result);
    return true;
}

bool HttpPost(const std::string& url,
              const std::vector<std::pair<std::string, std::string>>& headers,
              const std::string& body,
              HttpResult* out,
              std::string* error,
              int timeout_seconds)
{
    if (out == nullptr)
    {
        SetError(error, "HTTP output pointer is null");
        return false;
    }
    ParsedUrl parsed;
    if (!ParseUrl(url, &parsed, error))
    {
        return false;
    }
    const bool is_https = adapter_utils::modules::IsHttpsScheme(parsed.scheme == "https");
    const bool is_http = adapter_utils::modules::IsHttpScheme(parsed.scheme == "http");
    if (!is_https && !is_http)
    {
        SetError(error, adapter_utils::modules::UnsupportedSchemeMessage());
        return false;
    }
    net::io_context ioc;

    http::request<http::string_body> req{http::verb::post, parsed.target, 11};
    req.set(http::field::host, parsed.host);
    req.set(http::field::user_agent, adapter_utils::modules::DefaultUserAgent());
    req.set(http::field::accept_encoding, "identity");
    for (const auto& [key, value] : headers)
        req.set(key, value);
    if (req.find(http::field::content_type) == req.end())
        req.set(http::field::content_type, "application/json; charset=UTF-8");
    req.body() = body;
    req.prepare_payload();

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    beast::error_code ec;
    if (is_https)
    {
        SSL_CTX* native_context = SSL_CTX_new(TLS_client_method());
        if (native_context == nullptr)
        {
            SetError(error, "failed to create TLS context");
            return false;
        }
        ssl::context ctx(native_context);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
        if (!ConfigureTlsPeerVerification(ctx, stream, parsed.host, error))
        {
            return false;
        }
        if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str()))
        {
            SetError(error, "failed to set TLS SNI host");
            return false;
        }
        if (!EstablishOutboundTcp(beast::get_lowest_layer(stream), parsed, true, timeout_seconds, ec, error))
        {
            return false;
        }
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds < 1 ? 1 : timeout_seconds));
        stream.handshake(ssl::stream_base::client, ec);
        if (!ec)
        {
            http::write(stream, req, ec);
        }
        if (!ec)
        {
            http::read(stream, buffer, res, ec);
        }
        if (ec)
        {
            SetError(error, ec.message());
            return false;
        }
        beast::error_code shutdown_error;
        stream.shutdown(shutdown_error);
    }
    else
    {
        beast::tcp_stream stream(ioc);
        if (!EstablishOutboundTcp(stream, parsed, false, timeout_seconds, ec, error))
        {
            return false;
        }
        stream.expires_after(std::chrono::seconds(timeout_seconds < 1 ? 1 : timeout_seconds));
        http::write(stream, req, ec);
        if (!ec)
        {
            http::read(stream, buffer, res, ec);
        }
        if (ec)
        {
            SetError(error, ec.message());
            return false;
        }
        beast::error_code shutdown_error;
        stream.socket().shutdown(tcp::socket::shutdown_both, shutdown_error);
    }
    HttpResult result;
    result.status = res.result_int();
    result.body = std::move(res.body());
    auto ct = res.find(http::field::content_type);
    if (ct != res.end())
        result.content_type.assign(ct->value().data(), ct->value().size());
    *out = std::move(result);
    return true;
}

bool Md5Hex(const std::string& input, std::string* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "MD5 output pointer is null");
        return false;
    }
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        SetError(error, "EVP_MD_CTX_new failed");
        return false;
    }
    const bool ok = EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) == 1 &&
                    EVP_DigestUpdate(ctx, input.data(), input.size()) == 1 &&
                    EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) == 1;
    EVP_MD_CTX_free(ctx);
    if (!ok)
    {
        SetError(error, "MD5 failed");
        return false;
    }
    std::ostringstream formatted;
    formatted << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i)
        formatted << std::setw(2) << static_cast<int>(digest[i]);
    *out = formatted.str();
    return true;
}

bool Aes256EcbDecrypt(const std::string& cipher_text, const std::string& key, std::string* out, std::string* error)
{
    // NOTE: AES-256-ECB is used here ONLY because the upstream JM/Picacg API mandates
    // this mode as part of its wire protocol. ECB is cryptographically weak (no IV,
    // deterministic) and MUST NOT be used for any internal MemoChat encryption.
    // Third-party protocol constraint — do not copy this pattern.
    // AUDIT: CWE-327 accepted as third-party constraint (2026-07-05).
    if (key.size() != 32)
    {
        SetError(error, "JM AES-256-ECB key must be exactly 32 bytes");
        return false;
    }
    if (out == nullptr)
    {
        SetError(error, "AES output pointer is null");
        return false;
    }
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        SetError(error, "EVP_CIPHER_CTX_new failed");
        return false;
    }
    std::vector<unsigned char> plaintext(cipher_text.size() + EVP_MAX_BLOCK_LENGTH);
    int out_len_1 = 0;
    int out_len_2 = 0;
    const bool ok = EVP_DecryptInit_ex(ctx,
                                       EVP_aes_256_ecb(),
                                       nullptr,
                                       reinterpret_cast<const unsigned char*>(key.data()),
                                       nullptr) == 1 &&
                    EVP_CIPHER_CTX_set_padding(ctx, 1) == 1 &&
                    EVP_DecryptUpdate(ctx,
                                      plaintext.data(),
                                      &out_len_1,
                                      reinterpret_cast<const unsigned char*>(cipher_text.data()),
                                      static_cast<int>(cipher_text.size())) == 1 &&
                    EVP_DecryptFinal_ex(ctx, plaintext.data() + out_len_1, &out_len_2) == 1;
    EVP_CIPHER_CTX_free(ctx);
    if (!ok)
    {
        SetError(error, "AES decrypt failed");
        return false;
    }
    *out = std::string(reinterpret_cast<char*>(plaintext.data()), static_cast<std::size_t>(out_len_1 + out_len_2));
    return true;
}

std::string UrlEncode(const std::string& input)
{
    std::ostringstream out;
    out << std::hex << std::uppercase;
    for (unsigned char c : input)
    {
        if (adapter_utils::modules::IsUrlEncodeUnreserved(c))
            out << static_cast<char>(c);
        else
            out << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return out.str();
}

std::string EscapeXml(std::string value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value)
    {
        if (const char* escaped_text = adapter_utils::modules::XmlEscapeText(ch); escaped_text != nullptr)
            escaped += escaped_text;
        else
            escaped.push_back(ch);
    }
    return escaped;
}

std::string StringValue(const json::JsonValue& value)
{
    if (value.isString())
        return value.asString();
    if (value.isNumber())
        return std::to_string(value.asInt64());
    if (value.isBool())
        return value.asBool() ? "true" : "false";
    return "";
}

std::string FieldString(const json::JsonValue& object, const std::string& key, const std::string& fallback)
{
    if (!object.isObject() || !json::glaze_has_key(object, key))
        return fallback;
    const std::string value = StringValue(json::glaze_get(object, key));
    return value.empty() ? fallback : value;
}

int64_t FieldInt(const json::JsonValue& object, const std::string& key, int64_t fallback)
{
    if (!object.isObject() || !json::glaze_has_key(object, key))
        return fallback;
    const json::JsonValue value = json::glaze_get(object, key);
    if (value.isNumber())
        return value.asInt64();
    if (value.isString())
    {
        const std::string text = value.asString();
        int64_t parsed = 0;
        const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), parsed);
        if (ec == std::errc{} && ptr == text.data() + text.size())
        {
            return parsed;
        }
    }
    return fallback;
}

json::JsonValue MakeTags(const std::vector<std::string>& tags)
{
    json::JsonValue arr{json::array_t{}};
    for (const auto& tag : tags)
        if (!adapter_utils::modules::ShouldSkipEmptyTag(tag.empty()))
            json::glaze_append(arr, tag);
    return arr;
}

std::string ImageProxyUrl(const std::string& source_id, const std::string& image_url)
{
    if (image_url.empty())
        return "";
    return "/api/r18/image?source_id=" + UrlEncode(source_id) + "&image_url=" + UrlEncode(image_url);
}

R18ImagePayload PlaceholderImage(const std::string& line1, const std::string& line2)
{
    const std::string safe_line1 = EscapeXml(line1);
    const std::string safe_line2 = EscapeXml(line2);
    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"720\" height=\"1080\" viewBox=\"0 0 720 1080\">"
        << "<rect width=\"720\" height=\"1080\" fill=\"#201923\"/>"
        << "<rect x=\"48\" y=\"48\" width=\"624\" height=\"984\" rx=\"18\" fill=\"#2f2734\" stroke=\"#f2a7c5\" "
           "stroke-width=\"3\"/>"
        << "<text x=\"360\" y=\"500\" fill=\"#f8dce7\" font-size=\"36\" text-anchor=\"middle\" font-family=\"Arial\">"
        << safe_line1 << "</text>";
    if (!safe_line2.empty())
        svg << "<text x=\"360\" y=\"558\" fill=\"#d6bac6\" font-size=\"22\" text-anchor=\"middle\" "
               "font-family=\"Arial\">"
            << safe_line2 << "</text>";
    svg << "</svg>";
    R18ImagePayload payload;
    payload.content_type = adapter_utils::modules::PlaceholderContentType();
    payload.body = svg.str();
    return payload;
}

R18ImagePayload FailedImage(std::string error)
{
    R18ImagePayload payload;
    payload.ok = false;
    payload.error = std::move(error);
    return payload;
}

bool ReadCachedImage(const std::filesystem::path& cache_root, const std::string& cache_key, R18ImagePayload* payload)
{
    const auto body_path = cache_root / (cache_key + ".bin");
    const auto meta_path = cache_root / (cache_key + ".meta");
    std::ifstream body_in(body_path, std::ios::binary);
    if (!adapter_utils::modules::ShouldReadCachedImageBody(body_in.is_open()))
        return false;
    payload->body.assign(std::istreambuf_iterator<char>(body_in), std::istreambuf_iterator<char>());
    if (!adapter_utils::modules::HasCachedImageBody(payload->body.empty()))
        return false;
    std::ifstream meta_in(meta_path, std::ios::binary);
    if (meta_in.is_open())
        std::getline(meta_in, payload->content_type);
    if (adapter_utils::modules::ShouldUseDefaultCachedImageContentType(payload->content_type.empty()))
        payload->content_type = adapter_utils::modules::DefaultCachedImageContentType();
    return true;
}

void WriteCachedImage(const std::filesystem::path& cache_root,
                      const std::string& cache_key,
                      const R18ImagePayload& payload)
{
    std::error_code ec;
    std::filesystem::create_directories(cache_root, ec);
    if (ec)
        return;
    std::ofstream body_out(cache_root / (cache_key + ".bin"), std::ios::binary | std::ios::trunc);
    if (!body_out.is_open())
        return;
    body_out.write(payload.body.data(), static_cast<std::streamsize>(payload.body.size()));
    std::ofstream meta_out(cache_root / (cache_key + ".meta"), std::ios::binary | std::ios::trunc);
    if (meta_out.is_open())
        meta_out << payload.content_type;
}

} // namespace memochat::r18::detail

// DecodeBase64 is declared in R18SourceService.h at namespace memochat::r18 scope.
namespace memochat::r18
{

bool DecodeBase64Bounded(const std::string& input, std::string& out, std::size_t max_output_bytes)
{
    const unsigned char invalid = memochat::r18::adapter_utils::modules::Base64InvalidMarker();
    unsigned char table[256];
    std::fill(std::begin(table), std::end(table), invalid);
    for (int i = 0; i < 26; ++i)
    {
        table[static_cast<unsigned char>('A' + i)] = static_cast<unsigned char>(i);
        table[static_cast<unsigned char>('a' + i)] = static_cast<unsigned char>(26 + i);
    }
    for (int i = 0; i < 10; ++i)
        table[static_cast<unsigned char>('0' + i)] = static_cast<unsigned char>(52 + i);
    table[static_cast<unsigned char>('+')] = 62;
    table[static_cast<unsigned char>('/')] = 63;

    out.clear();
    int val = 0;
    int bits = -8;
    for (unsigned char c : input)
    {
        if (memochat::r18::adapter_utils::modules::ShouldSkipBase64Whitespace(c))
            continue;
        if (memochat::r18::adapter_utils::modules::IsBase64Padding(c))
            break;
        if (memochat::r18::adapter_utils::modules::HasInvalidBase64Value(table[c], invalid))
            return false;
        val = (val << 6) + table[c];
        bits += 6;
        if (bits >= 0)
        {
            out.push_back(static_cast<char>((val >> bits) & 0xFF));
            if (out.size() > max_output_bytes)
            {
                out.clear();
                return false;
            }
            bits -= 8;
        }
    }
    return true;
}

bool DecodeBase64(const std::string& input, std::string& out)
{
    return DecodeBase64Bounded(input, out, std::numeric_limits<std::size_t>::max());
}

} // namespace memochat::r18
