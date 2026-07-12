#include "r18/R18PicacgAdapter.hpp"
#include "r18/R18AdapterUtils.hpp"
#include "ConfigMgr.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/ssl.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

import memochat.r18.picacg_adapter_algorithms;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

namespace memochat::r18
{

const char* const kPicacgSourceId = "picacg.official";

namespace
{

using namespace detail;
using json::JsonValue;

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
        ctx.set_verify_mode(ssl::verify_peer, ec);
    if (!ec)
        stream.set_verify_mode(ssl::verify_peer, ec);
    if (!ec)
        stream.set_verify_callback(ssl::host_name_verification(host), ec);
    if (ec)
    {
        SetError(error, ec.message());
        return false;
    }
    return true;
}

std::string TrimCopy(std::string value)
{
    const auto is_space = [](unsigned char ch)
    {
        return std::isspace(ch) != 0;
    };
    value.erase(value.begin(),
                std::find_if(value.begin(),
                             value.end(),
                             [&](char ch)
                             {
                                 return !is_space(static_cast<unsigned char>(ch));
                             }));
    value.erase(std::find_if(value.rbegin(),
                             value.rend(),
                             [&](char ch)
                             {
                                 return !is_space(static_cast<unsigned char>(ch));
                             })
                    .base(),
                value.end());
    return value;
}

std::string LowerAsciiCopy(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool HasAllowedImageContentType(std::string content_type)
{
    if (const auto separator = content_type.find(';'); separator != std::string::npos)
        content_type.erase(separator);
    content_type = LowerAsciiCopy(TrimCopy(std::move(content_type)));
    return picacg_adapter::modules::IsAllowedImageContentType(content_type == "image/jpeg",
                                                              content_type == "image/png",
                                                              content_type == "image/webp",
                                                              content_type == "image/gif",
                                                              content_type == "image/avif");
}

bool ValidatePicacgImageUrl(const std::string& image_url, ParsedUrl* parsed, std::string* error)
{
    ParsedUrl candidate;
    if (!ParseUrl(image_url, &candidate, error))
        return false;

    const std::string allowed_hosts =
        ConfigMgr::Inst().GetValue(picacg_adapter::modules::AllowedImageHostsConfigSection(),
                                   picacg_adapter::modules::AllowedImageHostsConfigKey());
    const bool host_allowed = picacg_adapter::modules::IsExactHostInPolicy(candidate.host.data(),
                                                                           candidate.host.size(),
                                                                           allowed_hosts.data(),
                                                                           allowed_hosts.size());
    const bool target_allowed = candidate.target.rfind(picacg_adapter::modules::ImageTargetPrefix(), 0) == 0;
    if (!picacg_adapter::modules::IsCanonicalAllowedImageUrl(candidate.scheme == "https",
                                                             candidate.port == "443",
                                                             candidate.has_userinfo,
                                                             candidate.has_fragment,
                                                             host_allowed,
                                                             target_allowed))
    {
        SetError(error, "Picacg image URL is outside the configured media origin policy");
        return false;
    }
    if (parsed != nullptr)
        *parsed = std::move(candidate);
    return true;
}

bool IsPublicEndpoint(const tcp::endpoint& endpoint)
{
    const auto address = endpoint.address();
    if (address.is_v4())
    {
        const auto bytes = address.to_v4().to_bytes();
        const unsigned int value = (static_cast<unsigned int>(bytes[0]) << 24U) |
                                   (static_cast<unsigned int>(bytes[1]) << 16U) |
                                   (static_cast<unsigned int>(bytes[2]) << 8U) | static_cast<unsigned int>(bytes[3]);
        return picacg_adapter::modules::IsPublicIpv4Address(value);
    }
    if (address.is_v6())
    {
        const auto bytes = address.to_v6().to_bytes();
        return picacg_adapter::modules::IsPublicIpv6Address(bytes.data(), bytes.size());
    }
    return false;
}

bool ResolvePublicImageEndpoints(net::io_context& ioc,
                                 const ParsedUrl& parsed,
                                 std::vector<tcp::endpoint>* endpoints,
                                 std::string* error)
{
    if (endpoints == nullptr)
    {
        SetError(error, "Picacg endpoint output pointer is null");
        return false;
    }
    endpoints->clear();
    tcp::resolver resolver(ioc);
    beast::error_code ec;
    const auto resolved = resolver.resolve(parsed.host, parsed.port, ec);
    if (ec)
    {
        SetError(error, ec.message());
        return false;
    }
    for (const auto& result : resolved)
    {
        if (!IsPublicEndpoint(result.endpoint()))
        {
            endpoints->clear();
            SetError(error, "Picacg image host resolved to a non-public address");
            return false;
        }
        endpoints->push_back(result.endpoint());
    }
    if (endpoints->empty())
    {
        SetError(error, "Picacg image host resolved to no addresses");
        return false;
    }
    return true;
}

bool HttpGetPinned(net::io_context& ioc,
                   const ParsedUrl& parsed,
                   const std::vector<tcp::endpoint>& endpoints,
                   HttpResult* out,
                   std::string* error)
{
    if (out == nullptr || endpoints.empty())
    {
        SetError(error, "Picacg pinned HTTP request is missing output or endpoints");
        return false;
    }

    SSL_CTX* native_context = SSL_CTX_new(TLS_client_method());
    if (native_context == nullptr)
    {
        SetError(error, "failed to create TLS context");
        return false;
    }
    ssl::context context(native_context);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, context);
    if (!ConfigureTlsPeerVerification(context, stream, parsed.host, error))
        return false;
    if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str()))
    {
        SetError(error, "failed to set TLS SNI host");
        return false;
    }

    beast::error_code ec;
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(picacg_adapter::modules::ApiTimeoutSeconds()));
    beast::get_lowest_layer(stream).connect(endpoints, ec);
    if (!ec)
        stream.handshake(ssl::stream_base::client, ec);

    http::request<http::empty_body> request{http::verb::get, parsed.target, 11};
    request.set(http::field::host, parsed.host);
    request.set(http::field::user_agent, "MemoChat-R18/1.0");
    request.set(http::field::accept, "image/jpeg,image/png,image/webp,image/gif,image/avif");
    request.set(http::field::accept_encoding, "identity");
    request.set(http::field::referer, picacg_adapter::modules::ImageReferer());
    if (!ec)
        http::write(stream, request, ec);

    beast::flat_buffer buffer;
    http::response_parser<http::string_body> parser;
    parser.body_limit(picacg_adapter::modules::MaxImageBytes());
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(picacg_adapter::modules::ApiTimeoutSeconds()));
    if (!ec)
        http::read(stream, buffer, parser, ec);
    beast::error_code shutdown_error;
    stream.shutdown(shutdown_error);
    if (ec)
    {
        SetError(error, ec.message());
        return false;
    }

    auto response = parser.release();
    HttpResult result;
    result.status = response.result_int();
    result.body = std::move(response.body());
    if (const auto content_type = response.find(http::field::content_type); content_type != response.end())
        result.content_type.assign(content_type->value().data(), content_type->value().size());
    *out = std::move(result);
    return true;
}

bool ReadBoundedCachedImage(const std::filesystem::path& cache_root,
                            const std::string& cache_key,
                            R18ImagePayload* payload)
{
    std::error_code ec;
    const auto size = std::filesystem::file_size(cache_root / (cache_key + ".bin"), ec);
    if (ec || size == 0 || size > picacg_adapter::modules::MaxImageBytes())
        return false;
    return ReadCachedImage(cache_root, cache_key, payload) && HasAllowedImageContentType(payload->content_type) &&
           payload->body.size() <= picacg_adapter::modules::MaxImageBytes();
}

bool HmacSha256Hex(const std::string& key, const std::string& data, std::string* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "HMAC output pointer is null");
        return false;
    }
    unsigned char digest[EVP_MAX_MD_SIZE]{};
    unsigned int length = 0;
    if (HMAC(EVP_sha256(),
             key.data(),
             static_cast<int>(key.size()),
             reinterpret_cast<const unsigned char*>(data.data()),
             data.size(),
             digest,
             &length) == nullptr)
    {
        SetError(error, "HMAC-SHA256 failed");
        return false;
    }
    std::ostringstream formatted;
    formatted << std::hex << std::setfill('0');
    for (unsigned int index = 0; index < length; ++index)
        formatted << std::setw(2) << static_cast<int>(digest[index]);
    *out = formatted.str();
    return true;
}

bool GenerateNonce(std::string* out, std::string* error)
{
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    return Md5Hex(std::to_string(now), out, error);
}

bool PicacgHeaders(const std::string& method,
                   const std::string& path,
                   const std::string& token,
                   std::vector<std::pair<std::string, std::string>>* out,
                   std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "Picacg headers output pointer is null");
        return false;
    }
    const std::string api_key = picacg_adapter::modules::ApiKey();
    const std::string hmac_key = picacg_adapter::modules::HmacKey();
    if (!picacg_adapter::modules::HasCredentials(api_key.empty(), hmac_key.empty()))
    {
        SetError(error, picacg_adapter::modules::MissingCredentialsMessage());
        return false;
    }

    const auto unix_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const std::string time_str = std::to_string(unix_seconds);
    std::string nonce;
    if (!GenerateNonce(&nonce, error))
    {
        return false;
    }
    const std::string raw = path + time_str + nonce + method + api_key;
    std::string lower;
    lower.reserve(raw.size());
    for (unsigned char c : raw)
        lower.push_back(static_cast<char>(std::tolower(c)));
    std::string signature;
    if (!HmacSha256Hex(hmac_key, lower, &signature, error))
    {
        return false;
    }

    *out = {
        {"api-key", api_key},
        {"accept", "application/vnd.picacomic.com.v1+json"},
        {"app-channel", "2"},
        {"authorization", token},
        {"time", time_str},
        {"nonce", nonce},
        {"app-version", "2.2.1.3.3.4"},
        {"app-uuid", "defaultUuid"},
        {"image-quality", "original"},
        {"app-platform", "android"},
        {"app-build-version", "45"},
        {"Content-Type", "application/json; charset=UTF-8"},
        {"user-agent", "okhttp/3.8.1"},
        {"version", "v1.4.1"},
        {"Host", picacg_adapter::modules::ApiHost()},
        {"signature", signature},
    };
    return true;
}

bool PicacgApiGet(const std::string& path, const std::string& token, JsonValue* out, std::string* error)
{
    const std::string path_no_slash =
        picacg_adapter::modules::ShouldStripLeadingSlash(path.empty(), !path.empty() && path[0] == '/') ? path.substr(1)
                                                                                                        : path;
    const std::string url = std::string("https://") + picacg_adapter::modules::ApiHost() + path;
    std::vector<std::pair<std::string, std::string>> headers;
    if (!PicacgHeaders(picacg_adapter::modules::GetMethod(), path_no_slash, token, &headers, error))
    {
        return false;
    }
    HttpResult response;
    if (!HttpGet(url, headers, &response, error, picacg_adapter::modules::ApiTimeoutSeconds()))
    {
        return false;
    }
    if (!picacg_adapter::modules::IsSuccessStatus(response.status))
    {
        SetError(error, "Picacg HTTP " + std::to_string(response.status));
        return false;
    }
    JsonValue root;
    if (!json::glaze_parse(root, response.body))
    {
        SetError(error, picacg_adapter::modules::InvalidJsonResponseMessage());
        return false;
    }
    *out = json::glaze_get(root, "data");
    return true;
}

JsonValue PicacgComicToJson(const JsonValue& comic)
{
    const std::string id = FieldString(comic, "_id");
    const std::string file_server = FieldString(json::glaze_get(comic, "thumb"), "fileServer");
    const std::string thumb_path = FieldString(json::glaze_get(comic, "thumb"), "path");
    const std::string cover_url = file_server.empty() ? "" : file_server + "/static/" + thumb_path;

    JsonValue item;
    item["source_id"] = picacg_adapter::modules::SourceId();
    item["comic_id"] = id;
    item["title"] = FieldString(comic, "title", id);
    item["subtitle"] = FieldString(comic, "author");
    item["cover"] = ImageProxyUrl(picacg_adapter::modules::SourceId(), cover_url);
    item["author"] = FieldString(comic, "author");
    JsonValue tags{json::array_t{}};
    const JsonValue categories = json::glaze_get(comic, "categories");
    if (const auto* values = json::glaze_get_array(categories))
        for (const auto& value : *values)
            json::glaze_append(tags, StringValue(JsonValue(value)));
    item["tags"] = tags;
    return item;
}

} // namespace

bool PicacgSearch(const std::string& keyword, int page, json::JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "Picacg search output pointer is null");
        return false;
    }
    const int normalized_page = picacg_adapter::modules::NormalizeSearchPage(page);
    const std::string path = "/comics/advanced-search?page=" + std::to_string(normalized_page);
    const std::string body = R"({"keyword":")" + keyword + R"(","sort":"dd"})";
    const std::string path_no_slash = "comics/advanced-search?page=" + std::to_string(normalized_page);
    std::vector<std::pair<std::string, std::string>> headers;
    if (!PicacgHeaders(picacg_adapter::modules::PostMethod(), path_no_slash, "", &headers, error))
    {
        return false;
    }
    const std::string url = std::string("https://") + picacg_adapter::modules::ApiHost() + path;

    detail::ParsedUrl parsed;
    if (!detail::ParseUrl(url, &parsed, error))
    {
        return false;
    }
    net::io_context ioc;
    SSL_CTX* native_context = SSL_CTX_new(TLS_client_method());
    if (native_context == nullptr)
    {
        SetError(error, "failed to create TLS context");
        return false;
    }
    ssl::context context(native_context);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, context);
    if (!ConfigureTlsPeerVerification(context, stream, parsed.host, error))
    {
        return false;
    }
    if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str()))
    {
        SetError(error, "failed to set TLS SNI host");
        return false;
    }
    tcp::resolver resolver(ioc);
    beast::error_code ec;
    const auto endpoints = resolver.resolve(parsed.host, parsed.port, ec);
    if (!ec)
    {
        beast::get_lowest_layer(stream).expires_after(
            std::chrono::seconds(picacg_adapter::modules::ApiTimeoutSeconds()));
        beast::get_lowest_layer(stream).connect(endpoints, ec);
    }
    if (!ec)
        stream.handshake(ssl::stream_base::client, ec);

    http::request<http::string_body> request{http::verb::post, parsed.target, 11};
    request.set(http::field::host, parsed.host);
    request.body() = body;
    request.prepare_payload();
    for (const auto& [key, value] : headers)
        request.set(key, value);
    if (!ec)
        http::write(stream, request, ec);

    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(picacg_adapter::modules::ApiTimeoutSeconds()));
    if (!ec)
        http::read(stream, buffer, response, ec);
    beast::error_code shutdown_error;
    stream.shutdown(shutdown_error);
    if (ec)
    {
        SetError(error, ec.message());
        return false;
    }
    if (!picacg_adapter::modules::IsSuccessStatus(response.result_int()))
    {
        SetError(error, "Picacg search HTTP " + std::to_string(response.result_int()));
        return false;
    }

    json::JsonValue root;
    if (!json::glaze_parse(root, response.body()))
    {
        SetError(error, picacg_adapter::modules::SearchInvalidJsonMessage());
        return false;
    }
    const json::JsonValue data = json::glaze_get(root, "data");
    const json::JsonValue comics = json::glaze_get(json::glaze_get(data, "comics"), "docs");
    const int max_page = static_cast<int>(detail::FieldInt(json::glaze_get(data, "comics"), "pages", 1));

    json::JsonValue result;
    result["source_id"] = picacg_adapter::modules::SourceId();
    result["keyword"] = keyword;
    result["page"] = normalized_page;
    result["max_page"] = max_page;
    result["items"] = json::JsonValue{json::array_t{}};
    if (const auto* values = json::glaze_get_array(comics))
        for (const auto& comic : *values)
            json::glaze_append(result["items"], PicacgComicToJson(json::JsonValue(comic)));
    *out = std::move(result);
    return true;
}

bool PicacgDetail(const std::string& comic_id, json::JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "Picacg detail output pointer is null");
        return false;
    }
    json::JsonValue data;
    if (!PicacgApiGet("/comics/" + comic_id, "", &data, error))
    {
        return false;
    }
    const json::JsonValue info = json::glaze_get(data, "comic");

    const std::string file_server = detail::FieldString(json::glaze_get(info, "thumb"), "fileServer");
    const std::string thumb_path = detail::FieldString(json::glaze_get(info, "thumb"), "path");
    const std::string cover_url = file_server.empty() ? "" : file_server + "/static/" + thumb_path;

    json::JsonValue result;
    result["source_id"] = picacg_adapter::modules::SourceId();
    result["comic_id"] = comic_id;
    result["title"] = detail::FieldString(info, "title", comic_id);
    result["description"] = detail::FieldString(info, "description");
    result["cover"] = detail::ImageProxyUrl(picacg_adapter::modules::SourceId(), cover_url);
    result["chapters"] = json::JsonValue{json::array_t{}};

    json::JsonValue episodes_data;
    if (!PicacgApiGet("/comics/" + comic_id + "/eps?page=1", "", &episodes_data, error))
    {
        return false;
    }
    const json::JsonValue episodes = json::glaze_get(json::glaze_get(episodes_data, "eps"), "docs");
    int order = 1;
    if (const auto* values = json::glaze_get_array(episodes))
    {
        for (const auto& episode : *values)
        {
            json::JsonValue chapter;
            chapter["source_id"] = picacg_adapter::modules::SourceId();
            chapter["comic_id"] = comic_id;
            chapter["chapter_id"] = detail::FieldString(json::JsonValue(episode), "_id", std::to_string(order));
            chapter["title"] =
                detail::FieldString(json::JsonValue(episode), "title", "Episode " + std::to_string(order));
            chapter["order"] = order++;
            json::glaze_append(result["chapters"], chapter);
        }
    }
    if (picacg_adapter::modules::ShouldUseFallbackEpisode(order == picacg_adapter::modules::DefaultEpisodeOrder()))
    {
        json::JsonValue chapter;
        chapter["source_id"] = picacg_adapter::modules::SourceId();
        chapter["comic_id"] = comic_id;
        chapter["chapter_id"] = comic_id + "-1";
        chapter["title"] = picacg_adapter::modules::DefaultEpisodeTitle();
        chapter["order"] = picacg_adapter::modules::DefaultEpisodeOrder();
        json::glaze_append(result["chapters"], chapter);
    }
    *out = std::move(result);
    return true;
}

bool PicacgPages(const std::string& comic_id, const std::string& chapter_id, json::JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "Picacg pages output pointer is null");
        return false;
    }
    const std::string path = "/comics/" + comic_id + "/order/1/pages?page=1";
    json::JsonValue data;
    if (!PicacgApiGet(path, "", &data, error))
    {
        return false;
    }
    const json::JsonValue pages = json::glaze_get(json::glaze_get(data, "pages"), "docs");

    json::JsonValue result;
    result["source_id"] = picacg_adapter::modules::SourceId();
    result["chapter_id"] = chapter_id;
    result["pages"] = json::JsonValue{json::array_t{}};

    int index = 1;
    if (const auto* values = json::glaze_get_array(pages))
    {
        for (const auto& value : *values)
        {
            const json::JsonValue media = json::glaze_get(json::JsonValue(value), "media");
            const std::string file_server = detail::FieldString(media, "fileServer");
            const std::string media_path = detail::FieldString(media, "path");
            const std::string image_url = file_server.empty() ? "" : file_server + "/static/" + media_path;
            json::JsonValue page_value;
            page_value["index"] = index;
            page_value["image_id"] = chapter_id + "-p" + std::to_string(index);
            page_value["url"] = detail::ImageProxyUrl(picacg_adapter::modules::SourceId(), image_url);
            json::glaze_append(result["pages"], page_value);
            ++index;
        }
    }
    *out = std::move(result);
    return true;
}

R18ImagePayload PicacgFetchImage(const std::filesystem::path& cache_root, const std::string& image_url)
{
    std::string error;
    detail::ParsedUrl parsed;
    if (!ValidatePicacgImageUrl(image_url, &parsed, &error))
    {
        return detail::PlaceholderImage(picacg_adapter::modules::ImageErrorTitle(), error);
    }

    net::io_context ioc;
    std::vector<tcp::endpoint> endpoints;
    if (!ResolvePublicImageEndpoints(ioc, parsed, &endpoints, &error))
    {
        return detail::PlaceholderImage(picacg_adapter::modules::ImageErrorTitle(), error);
    }

    std::string cache_key;
    if (!detail::Md5Hex(image_url, &cache_key, &error))
    {
        return detail::PlaceholderImage(picacg_adapter::modules::ImageErrorTitle(), error);
    }
    R18ImagePayload cached;
    if (ReadBoundedCachedImage(cache_root, cache_key, &cached))
    {
        return cached;
    }
    detail::HttpResult response;
    if (!HttpGetPinned(ioc, parsed, endpoints, &response, &error))
    {
        return detail::PlaceholderImage(picacg_adapter::modules::ImageErrorTitle(), error);
    }
    if (picacg_adapter::modules::ShouldUseImagePlaceholder(response.status, response.body.empty()) ||
        response.body.size() > picacg_adapter::modules::MaxImageBytes() ||
        !HasAllowedImageContentType(response.content_type))
    {
        return detail::PlaceholderImage(picacg_adapter::modules::ImageUnavailableTitle(),
                                        "remote image response was rejected");
    }
    R18ImagePayload payload;
    payload.content_type = response.content_type;
    payload.body = std::move(response.body);
    detail::WriteCachedImage(cache_root, cache_key, payload);
    return payload;
}

} // namespace memochat::r18
