#include "r18/R18PicacgAdapter.hpp"
#include "r18/R18AdapterUtils.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/ssl.h>

#include <cctype>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

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

void ConfigureTlsPeerVerification(ssl::context& ctx,
                                  beast::ssl_stream<beast::tcp_stream>& stream,
                                  const std::string& host)
{
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);
    stream.set_verify_mode(ssl::verify_peer);
    stream.set_verify_callback(ssl::host_name_verification(host));
}

std::string HmacSha256Hex(const std::string& key, const std::string& data)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(),
         key.data(),
         static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()),
         data.size(),
         digest,
         &len);
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < len; ++i)
        out << std::setw(2) << static_cast<int>(digest[i]);
    return out.str();
}

std::string GenerateNonce()
{
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    return Md5Hex(std::to_string(now));
}

std::vector<std::pair<std::string, std::string>>
PicacgHeaders(const std::string& method, const std::string& path, const std::string& token = "")
{
    const std::string api_key = picacg_adapter::modules::ApiKey();
    const std::string hmac_key = picacg_adapter::modules::HmacKey();
    if (!picacg_adapter::modules::HasCredentials(api_key.empty(), hmac_key.empty()))
    {
        throw std::runtime_error(picacg_adapter::modules::MissingCredentialsMessage());
    }

    const auto unix_ms =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const std::string time_str = std::to_string(unix_ms);
    const std::string nonce = GenerateNonce();
    const std::string raw = path + time_str + nonce + method + api_key;
    std::string lower;
    lower.reserve(raw.size());
    for (unsigned char c : raw)
        lower.push_back(static_cast<char>(std::tolower(c)));
    const std::string sig = HmacSha256Hex(hmac_key, lower);

    return {
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
        {"signature", sig},
    };
}

JsonValue PicacgApiGet(const std::string& path, const std::string& token = "")
{
    const std::string path_no_slash =
        picacg_adapter::modules::ShouldStripLeadingSlash(path.empty(), !path.empty() && path[0] == '/') ? path.substr(1)
                                                                                                        : path;
    const std::string url = std::string("https://") + picacg_adapter::modules::ApiHost() + path;
    const auto headers = PicacgHeaders(picacg_adapter::modules::GetMethod(), path_no_slash, token);
    const HttpResult res = HttpGet(url, headers, picacg_adapter::modules::ApiTimeoutSeconds());
    if (!picacg_adapter::modules::IsSuccessStatus(res.status))
        throw std::runtime_error("Picacg HTTP " + std::to_string(res.status));
    JsonValue root;
    if (!json::glaze_parse(root, res.body))
        throw std::runtime_error(picacg_adapter::modules::InvalidJsonResponseMessage());
    return json::glaze_get(root, "data");
}

JsonValue PicacgComicToJson(const JsonValue& c)
{
    const std::string id = FieldString(c, "_id");
    const std::string file_server = FieldString(json::glaze_get(c, "thumb"), "fileServer");
    const std::string thumb_path = FieldString(json::glaze_get(c, "thumb"), "path");
    const std::string cover_url = file_server.empty() ? "" : file_server + "/static/" + thumb_path;

    JsonValue item;
    item["source_id"] = picacg_adapter::modules::SourceId();
    item["comic_id"] = id;
    item["title"] = FieldString(c, "title", id);
    item["subtitle"] = FieldString(c, "author");
    item["cover"] = ImageProxyUrl(picacg_adapter::modules::SourceId(), cover_url);
    item["author"] = FieldString(c, "author");
    JsonValue tags{json::array_t{}};
    const JsonValue cats = json::glaze_get(c, "categories");
    if (const auto* arr = json::glaze_get_array(cats))
        for (const auto& e : *arr)
            json::glaze_append(tags, StringValue(JsonValue(e)));
    item["tags"] = tags;
    return item;
}

} // namespace

json::JsonValue PicacgSearch(const std::string& keyword, int page)
{
    const int p = picacg_adapter::modules::NormalizeSearchPage(page);
    const std::string path = "/comics/advanced-search?page=" + std::to_string(p);
    const std::string body_str = R"({"keyword":")" + keyword + R"(","sort":"dd"})";
    const std::string path_no_slash = "comics/advanced-search?page=" + std::to_string(p);
    const auto headers = PicacgHeaders(picacg_adapter::modules::PostMethod(), path_no_slash);
    const std::string url = std::string("https://") + picacg_adapter::modules::ApiHost() + path;

    const detail::ParsedUrl parsed = detail::ParseUrl(url);
    net::io_context ioc;
    ssl::context ctx(ssl::context::tls_client);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
    ConfigureTlsPeerVerification(ctx, stream, parsed.host);
    if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str()))
        throw std::runtime_error("failed to set TLS SNI host");
    tcp::resolver resolver(ioc);
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(picacg_adapter::modules::ApiTimeoutSeconds()));
    beast::get_lowest_layer(stream).connect(resolver.resolve(parsed.host, parsed.port));
    stream.handshake(ssl::stream_base::client);

    http::request<http::string_body> req{http::verb::post, parsed.target, 11};
    req.set(http::field::host, parsed.host);
    req.body() = body_str;
    req.prepare_payload();
    for (const auto& [k, v] : headers)
        req.set(k, v);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::write(stream, req);
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(picacg_adapter::modules::ApiTimeoutSeconds()));
    http::read(stream, buffer, res);
    beast::error_code ec;
    stream.shutdown(ec);

    if (!picacg_adapter::modules::IsSuccessStatus(res.result_int()))
        throw std::runtime_error("Picacg search HTTP " + std::to_string(res.result_int()));

    json::JsonValue root;
    if (!json::glaze_parse(root, res.body()))
        throw std::runtime_error(picacg_adapter::modules::SearchInvalidJsonMessage());
    const json::JsonValue data = json::glaze_get(root, "data");
    const json::JsonValue comics = json::glaze_get(json::glaze_get(data, "comics"), "docs");
    const int max_page = static_cast<int>(detail::FieldInt(json::glaze_get(data, "comics"), "pages", 1));

    json::JsonValue out;
    out["source_id"] = picacg_adapter::modules::SourceId();
    out["keyword"] = keyword;
    out["page"] = p;
    out["max_page"] = max_page;
    out["items"] = json::JsonValue{json::array_t{}};
    if (const auto* arr = json::glaze_get_array(comics))
        for (const auto& c : *arr)
            json::glaze_append(out["items"], PicacgComicToJson(json::JsonValue(c)));
    return out;
}

json::JsonValue PicacgDetail(const std::string& comic_id)
{
    const json::JsonValue data = PicacgApiGet("/comics/" + comic_id);
    const json::JsonValue info = json::glaze_get(data, "comic");

    const std::string file_server = detail::FieldString(json::glaze_get(info, "thumb"), "fileServer");
    const std::string thumb_path = detail::FieldString(json::glaze_get(info, "thumb"), "path");
    const std::string cover_url = file_server.empty() ? "" : file_server + "/static/" + thumb_path;

    json::JsonValue out;
    out["source_id"] = picacg_adapter::modules::SourceId();
    out["comic_id"] = comic_id;
    out["title"] = detail::FieldString(info, "title", comic_id);
    out["description"] = detail::FieldString(info, "description");
    out["cover"] = detail::ImageProxyUrl(picacg_adapter::modules::SourceId(), cover_url);
    out["chapters"] = json::JsonValue{json::array_t{}};

    const json::JsonValue eps_data = PicacgApiGet("/comics/" + comic_id + "/eps?page=1");
    const json::JsonValue eps_docs = json::glaze_get(json::glaze_get(eps_data, "eps"), "docs");
    int order = 1;
    if (const auto* arr = json::glaze_get_array(eps_docs))
    {
        for (const auto& ep : *arr)
        {
            json::JsonValue ch;
            ch["source_id"] = picacg_adapter::modules::SourceId();
            ch["comic_id"] = comic_id;
            ch["chapter_id"] = detail::FieldString(json::JsonValue(ep), "_id", std::to_string(order));
            ch["title"] = detail::FieldString(json::JsonValue(ep), "title", "Episode " + std::to_string(order));
            ch["order"] = order++;
            json::glaze_append(out["chapters"], ch);
        }
    }
    if (picacg_adapter::modules::ShouldUseFallbackEpisode(order == picacg_adapter::modules::DefaultEpisodeOrder()))
    {
        json::JsonValue ch;
        ch["source_id"] = picacg_adapter::modules::SourceId();
        ch["comic_id"] = comic_id;
        ch["chapter_id"] = comic_id + "-1";
        ch["title"] = picacg_adapter::modules::DefaultEpisodeTitle();
        ch["order"] = picacg_adapter::modules::DefaultEpisodeOrder();
        json::glaze_append(out["chapters"], ch);
    }
    return out;
}

json::JsonValue PicacgPages(const std::string& comic_id, const std::string& chapter_id)
{
    const std::string path = "/comics/" + comic_id + "/order/1/pages?page=1";
    const json::JsonValue data = PicacgApiGet(path);
    const json::JsonValue pages = json::glaze_get(json::glaze_get(data, "pages"), "docs");

    json::JsonValue out;
    out["source_id"] = picacg_adapter::modules::SourceId();
    out["chapter_id"] = chapter_id;
    out["pages"] = json::JsonValue{json::array_t{}};

    int index = 1;
    if (const auto* arr = json::glaze_get_array(pages))
    {
        for (const auto& p : *arr)
        {
            const json::JsonValue media = json::glaze_get(json::JsonValue(p), "media");
            const std::string fs = detail::FieldString(media, "fileServer");
            const std::string mp = detail::FieldString(media, "path");
            const std::string img = fs.empty() ? "" : fs + "/static/" + mp;
            json::JsonValue page;
            page["index"] = index;
            page["image_id"] = chapter_id + "-p" + std::to_string(index);
            page["url"] = detail::ImageProxyUrl(picacg_adapter::modules::SourceId(), img);
            json::glaze_append(out["pages"], page);
            ++index;
        }
    }
    return out;
}

R18ImagePayload PicacgFetchImage(const std::filesystem::path& cache_root, const std::string& image_url)
{
    try
    {
        const detail::ParsedUrl parsed = detail::ParseUrl(image_url);
        if (picacg_adapter::modules::ShouldRejectImageScheme(parsed.scheme == "https"))
            throw std::runtime_error(picacg_adapter::modules::NonHttpsImageRejectedMessage());
        const std::string cache_key = detail::Md5Hex(image_url);
        R18ImagePayload cached;
        if (detail::ReadCachedImage(cache_root, cache_key, &cached))
            return cached;
        detail::HttpResult result = detail::HttpGet(image_url,
                                                    {{"Referer", picacg_adapter::modules::ImageReferer()}},
                                                    picacg_adapter::modules::ApiTimeoutSeconds());
        if (picacg_adapter::modules::ShouldUseImagePlaceholder(result.status, result.body.empty()))
            return detail::PlaceholderImage(picacg_adapter::modules::ImageUnavailableTitle(),
                                            "HTTP " + std::to_string(result.status));
        R18ImagePayload payload;
        payload.content_type = picacg_adapter::modules::ShouldUseDefaultImageContentType(result.content_type.empty())
                                   ? picacg_adapter::modules::DefaultImageContentType()
                                   : result.content_type;
        payload.body = std::move(result.body);
        detail::WriteCachedImage(cache_root, cache_key, payload);
        return payload;
    }
    catch (const std::exception& exc)
    {
        return detail::PlaceholderImage(picacg_adapter::modules::ImageErrorTitle(), exc.what());
    }
}

} // namespace memochat::r18
