#include "r18/R18PicacgAdapter.h"
#include "r18/R18AdapterUtils.h"

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

constexpr const char* kPicacgApiHost = "picaapi.picacomic.com";
constexpr const char* kPicacgApiKey = "C69BAF41DA5ABD1FFEDC6D2FEA56B";
constexpr const char* kPicacgHmacKey = "~d}$Q7$eIni=V)9\\RK/P.RM4;9[7|@/CA}b~OW!3?EV`:<>M7pddUBL5n|0/*Cn";
constexpr int kPicacgApiTimeoutSeconds = 8;

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
    const auto unix_ms =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const std::string time_str = std::to_string(unix_ms);
    const std::string nonce = GenerateNonce();
    const std::string raw = path + time_str + nonce + method + kPicacgApiKey;
    std::string lower;
    lower.reserve(raw.size());
    for (unsigned char c : raw)
        lower.push_back(static_cast<char>(std::tolower(c)));
    const std::string sig = HmacSha256Hex(kPicacgHmacKey, lower);

    return {
        {"api-key", kPicacgApiKey},
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
        {"Host", kPicacgApiHost},
        {"signature", sig},
    };
}

JsonValue PicacgApiGet(const std::string& path, const std::string& token = "")
{
    const std::string path_no_slash = path.empty() ? "" : (path[0] == '/' ? path.substr(1) : path);
    const std::string url = std::string("https://") + kPicacgApiHost + path;
    const auto headers = PicacgHeaders("GET", path_no_slash, token);
    const HttpResult res = HttpGet(url, headers, kPicacgApiTimeoutSeconds);
    if (res.status != 200)
        throw std::runtime_error("Picacg HTTP " + std::to_string(res.status));
    JsonValue root;
    if (!json::glaze_parse(root, res.body))
        throw std::runtime_error("Picacg invalid JSON response");
    return json::glaze_get(root, "data");
}

JsonValue PicacgComicToJson(const JsonValue& c, int uid, const std::string& token)
{
    const std::string id = FieldString(c, "_id");
    const std::string file_server = FieldString(json::glaze_get(c, "thumb"), "fileServer");
    const std::string thumb_path = FieldString(json::glaze_get(c, "thumb"), "path");
    const std::string cover_url = file_server.empty() ? "" : file_server + "/static/" + thumb_path;

    JsonValue item;
    item["source_id"] = kPicacgSourceId;
    item["comic_id"] = id;
    item["title"] = FieldString(c, "title", id);
    item["subtitle"] = FieldString(c, "author");
    item["cover"] = ImageProxyUrl(uid, token, kPicacgSourceId, cover_url);
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

json::JsonValue PicacgSearch(const std::string& keyword, int page, int uid, const std::string& token)
{
    const int p = page < 1 ? 1 : page;
    const std::string path = "/comics/advanced-search?page=" + std::to_string(p);
    const std::string body_str = R"({"keyword":")" + keyword + R"(","sort":"dd"})";
    const std::string path_no_slash = "comics/advanced-search?page=" + std::to_string(p);
    const auto headers = PicacgHeaders("POST", path_no_slash, token);
    const std::string url = std::string("https://") + kPicacgApiHost + path;

    const detail::ParsedUrl parsed = detail::ParseUrl(url);
    net::io_context ioc;
    ssl::context ctx(ssl::context::tls_client);
    ctx.set_verify_mode(ssl::verify_none);
    beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
    SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str());
    tcp::resolver resolver(ioc);
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(kPicacgApiTimeoutSeconds));
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
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(kPicacgApiTimeoutSeconds));
    http::read(stream, buffer, res);
    beast::error_code ec;
    stream.shutdown(ec);

    if (res.result_int() != 200)
        throw std::runtime_error("Picacg search HTTP " + std::to_string(res.result_int()));

    json::JsonValue root;
    if (!json::glaze_parse(root, res.body()))
        throw std::runtime_error("Picacg search invalid JSON");
    const json::JsonValue data = json::glaze_get(root, "data");
    const json::JsonValue comics = json::glaze_get(json::glaze_get(data, "comics"), "docs");
    const int max_page = static_cast<int>(detail::FieldInt(json::glaze_get(data, "comics"), "pages", 1));

    json::JsonValue out;
    out["source_id"] = kPicacgSourceId;
    out["keyword"] = keyword;
    out["page"] = p;
    out["max_page"] = max_page;
    out["items"] = json::JsonValue{json::array_t{}};
    if (const auto* arr = json::glaze_get_array(comics))
        for (const auto& c : *arr)
            json::glaze_append(out["items"], PicacgComicToJson(json::JsonValue(c), uid, token));
    return out;
}

json::JsonValue PicacgDetail(const std::string& comic_id, int uid, const std::string& token)
{
    const json::JsonValue data = PicacgApiGet("/comics/" + comic_id, token);
    const json::JsonValue info = json::glaze_get(data, "comic");

    const std::string file_server = detail::FieldString(json::glaze_get(info, "thumb"), "fileServer");
    const std::string thumb_path = detail::FieldString(json::glaze_get(info, "thumb"), "path");
    const std::string cover_url = file_server.empty() ? "" : file_server + "/static/" + thumb_path;

    json::JsonValue out;
    out["source_id"] = kPicacgSourceId;
    out["comic_id"] = comic_id;
    out["title"] = detail::FieldString(info, "title", comic_id);
    out["description"] = detail::FieldString(info, "description");
    out["cover"] = detail::ImageProxyUrl(uid, token, kPicacgSourceId, cover_url);
    out["chapters"] = json::JsonValue{json::array_t{}};

    const json::JsonValue eps_data = PicacgApiGet("/comics/" + comic_id + "/eps?page=1", token);
    const json::JsonValue eps_docs = json::glaze_get(json::glaze_get(eps_data, "eps"), "docs");
    int order = 1;
    if (const auto* arr = json::glaze_get_array(eps_docs))
    {
        for (const auto& ep : *arr)
        {
            json::JsonValue ch;
            ch["source_id"] = kPicacgSourceId;
            ch["comic_id"] = comic_id;
            ch["chapter_id"] = detail::FieldString(json::JsonValue(ep), "_id", std::to_string(order));
            ch["title"] = detail::FieldString(json::JsonValue(ep), "title", "Episode " + std::to_string(order));
            ch["order"] = order++;
            json::glaze_append(out["chapters"], ch);
        }
    }
    if (order == 1)
    {
        json::JsonValue ch;
        ch["source_id"] = kPicacgSourceId;
        ch["comic_id"] = comic_id;
        ch["chapter_id"] = comic_id + "-1";
        ch["title"] = "Episode 1";
        ch["order"] = 1;
        json::glaze_append(out["chapters"], ch);
    }
    return out;
}

json::JsonValue
PicacgPages(const std::string& comic_id, const std::string& chapter_id, int uid, const std::string& token)
{
    const std::string path = "/comics/" + comic_id + "/order/1/pages?page=1";
    const json::JsonValue data = PicacgApiGet(path, token);
    const json::JsonValue pages = json::glaze_get(json::glaze_get(data, "pages"), "docs");

    json::JsonValue out;
    out["source_id"] = kPicacgSourceId;
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
            page["url"] = detail::ImageProxyUrl(uid, token, kPicacgSourceId, img);
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
        if (parsed.scheme != "https")
            throw std::runtime_error("non-https Picacg image URL rejected");
        const std::string cache_key = detail::Md5Hex(image_url);
        R18ImagePayload cached;
        if (detail::ReadCachedImage(cache_root, cache_key, &cached))
            return cached;
        detail::HttpResult result = detail::HttpGet(image_url, {{"Referer", "https://manhuabika.com/"}}, 8);
        if (result.status < 200 || result.status >= 300 || result.body.empty())
            return detail::PlaceholderImage("Picacg image unavailable", "HTTP " + std::to_string(result.status));
        R18ImagePayload payload;
        payload.content_type = result.content_type.empty() ? "image/jpeg" : result.content_type;
        payload.body = std::move(result.body);
        detail::WriteCachedImage(cache_root, cache_key, payload);
        return payload;
    }
    catch (const std::exception& exc)
    {
        return detail::PlaceholderImage("Picacg image error", exc.what());
    }
}

} // namespace memochat::r18
