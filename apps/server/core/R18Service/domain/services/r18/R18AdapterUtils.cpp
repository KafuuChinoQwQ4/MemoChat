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
#include <cctype>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
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

void ConfigureTlsPeerVerification(ssl::context& ctx,
                                  beast::ssl_stream<beast::tcp_stream>& stream,
                                  const std::string& host)
{
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);
    stream.set_verify_mode(ssl::verify_peer);
    stream.set_verify_callback(ssl::host_name_verification(host));
}

} // namespace

ParsedUrl ParseUrl(const std::string& url)
{
    const auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos)
        throw std::runtime_error(adapter_utils::modules::MissingSchemeMessage());
    ParsedUrl parsed;
    parsed.scheme = url.substr(0, scheme_end);
    const auto authority_begin = scheme_end + 3;
    const auto path_begin = url.find('/', authority_begin);
    std::string authority = path_begin == std::string::npos ? url.substr(authority_begin)
                                                            : url.substr(authority_begin, path_begin - authority_begin);
    if (adapter_utils::modules::ShouldUseDefaultTarget(path_begin == std::string::npos))
        parsed.target = adapter_utils::modules::DefaultTarget();
    else
        parsed.target = url.substr(path_begin);
    const auto port_pos = authority.rfind(':');
    if (port_pos != std::string::npos && authority.find(']') == std::string::npos)
    {
        parsed.host = authority.substr(0, port_pos);
        parsed.port = authority.substr(port_pos + 1);
    }
    else
    {
        parsed.host = authority;
        parsed.port = adapter_utils::modules::SelectDefaultPort(parsed.scheme == "https");
    }
    if (adapter_utils::modules::ShouldThrowMissingHost(parsed.host.empty()))
        throw std::runtime_error(adapter_utils::modules::MissingHostMessage());
    return parsed;
}

HttpResult
HttpGet(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers, int timeout_seconds)
{
    const ParsedUrl parsed = ParseUrl(url);
    net::io_context ioc;
    tcp::resolver resolver(ioc);

    http::request<http::empty_body> req{http::verb::get, parsed.target, 11};
    req.set(http::field::host, parsed.host);
    req.set(http::field::user_agent, adapter_utils::modules::DefaultUserAgent());
    req.set(http::field::accept_encoding, "identity");
    for (const auto& [key, value] : headers)
        req.set(key, value);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    if (adapter_utils::modules::IsHttpsScheme(parsed.scheme == "https"))
    {
        ssl::context ctx(ssl::context::tls_client);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
        ConfigureTlsPeerVerification(ctx, stream, parsed.host);
        if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str()))
            throw std::runtime_error("failed to set TLS SNI host");
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds));
        beast::get_lowest_layer(stream).connect(resolver.resolve(parsed.host, parsed.port));
        stream.handshake(ssl::stream_base::client);
        http::write(stream, req);
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds));
        http::read(stream, buffer, res);
        beast::error_code ec;
        stream.shutdown(ec);
    }
    else if (adapter_utils::modules::IsHttpScheme(parsed.scheme == "http"))
    {
        beast::tcp_stream stream(ioc);
        stream.expires_after(std::chrono::seconds(timeout_seconds));
        stream.connect(resolver.resolve(parsed.host, parsed.port));
        http::write(stream, req);
        stream.expires_after(std::chrono::seconds(timeout_seconds));
        http::read(stream, buffer, res);
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }
    else
    {
        throw std::runtime_error(adapter_utils::modules::UnsupportedSchemeMessage());
    }

    HttpResult result;
    result.status = res.result_int();
    result.body = std::move(res.body());
    auto ct = res.find(http::field::content_type);
    if (ct != res.end())
        result.content_type.assign(ct->value().data(), ct->value().size());
    return result;
}

std::string Md5Hex(const std::string& input)
{
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        throw std::runtime_error("EVP_MD_CTX_new failed");
    const bool ok = EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) == 1 &&
                    EVP_DigestUpdate(ctx, input.data(), input.size()) == 1 &&
                    EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) == 1;
    EVP_MD_CTX_free(ctx);
    if (!ok)
        throw std::runtime_error("MD5 failed");
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i)
        out << std::setw(2) << static_cast<int>(digest[i]);
    return out.str();
}

std::string Aes256EcbDecrypt(const std::string& cipher_text, const std::string& key)
{
    // NOTE: AES-256-ECB is used here ONLY because the upstream JM/Picacg API mandates
    // this mode as part of its wire protocol. ECB is cryptographically weak (no IV,
    // deterministic) and MUST NOT be used for any internal MemoChat encryption.
    // Third-party protocol constraint — do not copy this pattern.
    // AUDIT: CWE-327 accepted as third-party constraint (2026-07-05).
    if (key.size() != 32)
        throw std::runtime_error("JM AES-256-ECB key must be exactly 32 bytes");
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    std::vector<unsigned char> out(cipher_text.size() + EVP_MAX_BLOCK_LENGTH);
    int out_len_1 = 0;
    int out_len_2 = 0;
    const bool ok = EVP_DecryptInit_ex(ctx,
                                       EVP_aes_256_ecb(),
                                       nullptr,
                                       reinterpret_cast<const unsigned char*>(key.data()),
                                       nullptr) == 1 &&
                    EVP_CIPHER_CTX_set_padding(ctx, 1) == 1 &&
                    EVP_DecryptUpdate(ctx,
                                      out.data(),
                                      &out_len_1,
                                      reinterpret_cast<const unsigned char*>(cipher_text.data()),
                                      static_cast<int>(cipher_text.size())) == 1 &&
                    EVP_DecryptFinal_ex(ctx, out.data() + out_len_1, &out_len_2) == 1;
    EVP_CIPHER_CTX_free(ctx);
    if (!ok)
        throw std::runtime_error("AES decrypt failed");
    return std::string(reinterpret_cast<char*>(out.data()), static_cast<std::size_t>(out_len_1 + out_len_2));
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
        try
        {
            return std::stoll(value.asString());
        }
        catch (...)
        {
            return fallback;
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

std::string ImageProxyUrl(int uid, const std::string& token, const std::string& source_id, const std::string& image_url)
{
    if (image_url.empty())
        return "";
    return "/api/r18/image?uid=" + std::to_string(uid) + "&token=" + UrlEncode(token) +
           "&source_id=" + UrlEncode(source_id) + "&image_url=" + UrlEncode(image_url);
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

bool DecodeBase64(const std::string& input, std::string& out)
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
            bits -= 8;
        }
    }
    return true;
}

} // namespace memochat::r18
