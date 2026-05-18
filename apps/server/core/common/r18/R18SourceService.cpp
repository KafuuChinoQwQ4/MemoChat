#include "r18/R18SourceService.h"

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
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

namespace memochat::r18 {
namespace {

using json::JsonValue;

constexpr const char* kMockSourceId = "mock";
constexpr const char* kJmSourceId = "jm.official";
constexpr const char* kJmImageBase = "https://cdn-msp.jmapinodeudzn.net";
constexpr const char* kJmImageHost = "cdn-msp.jmapinodeudzn.net";
constexpr const char* kJmVersion = "2.0.16";
constexpr const char* kJmPackageName = "com.example.app";
constexpr int kR18SearchPageSize = 40;
constexpr int kJmApiTimeoutSeconds = 6;
constexpr int kJmImageTimeoutSeconds = 5;
constexpr int kMaxConcurrentJmImageFetches = 2;
constexpr const char* kJmUserAgent = "Mozilla/5.0 (Linux; Android 10; K; wv) AppleWebKit/537.36 "
                                     "(KHTML, like Gecko) Version/4.0 Chrome/130.0.0.0 Mobile Safari/537.36";

const std::array<std::string, 4> kJmApiHosts = {
    "www.cdnsha.org",
    "www.cdnntr.cc",
    "www.cdntwice.org",
    "www.cdnaspa.cc",
};

struct ParsedUrl {
    std::string scheme;
    std::string host;
    std::string port;
    std::string target;
};

ParsedUrl ParseUrl(const std::string& url);

std::filesystem::path ResolveDataRoot()
{
    const auto cwd = std::filesystem::current_path();
    std::string leaf = cwd.filename().string();
    std::transform(leaf.begin(), leaf.end(), leaf.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    const auto base = leaf.rfind("gateserver", 0) == 0 ? cwd.parent_path() : cwd;
    return base / "data" / "r18" / "sources";
}

bool EndsWithCaseInsensitive(const std::string& value, const std::string& suffix)
{
    if (value.size() < suffix.size()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
}

std::string Stem(const std::string& file_name)
{
    std::filesystem::path p(file_name);
    auto stem = p.stem().string();
    return stem.empty() ? "imported-source" : stem;
}

std::string NormalizeId(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        if (std::isalnum(c)) return static_cast<char>(std::tolower(c));
        return '-';
    });
    value.erase(std::unique(value.begin(), value.end(), [](char a, char b) {
        return a == '-' && b == '-';
    }), value.end());
    while (!value.empty() && value.front() == '-') value.erase(value.begin());
    while (!value.empty() && value.back() == '-') value.pop_back();
    return value.empty() ? "imported-source" : value;
}

std::string NormalizePathSegment(std::string value, const std::string& fallback)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        if (std::isalnum(c) || c == '.' || c == '_' || c == '-') {
            return static_cast<char>(c);
        }
        return '-';
    });
    value.erase(std::unique(value.begin(), value.end(), [](char a, char b) {
        return a == '-' && b == '-';
    }), value.end());
    while (!value.empty() && (value.front() == '.' || value.front() == '-' || value.front() == '_')) {
        value.erase(value.begin());
    }
    while (!value.empty() && (value.back() == '.' || value.back() == '-' || value.back() == '_')) {
        value.pop_back();
    }
    return value.empty() ? fallback : value;
}

bool LooksLikeJavaScript(const std::string& file_name,
                         const std::string& manifest_json,
                         const std::string& binary)
{
    if (EndsWithCaseInsensitive(file_name, ".js")) {
        return true;
    }
    if (!manifest_json.empty()) {
        JsonValue manifest;
        if (json::glaze_parse(manifest, manifest_json)) {
            const std::string format = json::glaze_safe_get<std::string>(manifest, "format", "");
            if (format == "source-js" || format == "javascript") {
                return true;
            }
        }
    }
    const auto probe = binary.substr(0, std::min<std::size_t>(binary.size(), 4096));
    return probe.find("class ") != std::string::npos &&
           (probe.find("ComicSource") != std::string::npos || probe.find("search") != std::string::npos);
}

bool IsBuiltinSourceId(const std::string& id)
{
    return id == kMockSourceId || id == kJmSourceId;
}

JsonValue ToJson(const R18SourceRecord& source)
{
    JsonValue item;
    item["id"] = source.id;
    item["name"] = source.name;
    item["version"] = source.version;
    item["path"] = source.path;
    item["format"] = source.format;
    item["source_url"] = source.source_url;
    item["catalog_url"] = source.catalog_url;
    item["enabled"] = source.enabled;
    item["builtin"] = source.builtin;
    item["status"] = source.status;
    item["message"] = source.message;
    return item;
}

R18SourceRecord FromJson(const JsonValue& item)
{
    R18SourceRecord rec;
    rec.id = json::glaze_safe_get<std::string>(item, "id", "");
    rec.name = json::glaze_safe_get<std::string>(item, "name", rec.id);
    rec.version = json::glaze_safe_get<std::string>(item, "version", "0.0.0");
    rec.path = json::glaze_safe_get<std::string>(item, "path", "");
    rec.format = json::glaze_safe_get<std::string>(item, "format", "native-zip");
    rec.source_url = json::glaze_safe_get<std::string>(item, "source_url", "");
    rec.catalog_url = json::glaze_safe_get<std::string>(item, "catalog_url", "");
    rec.enabled = json::glaze_safe_get<bool>(item, "enabled", false);
    rec.builtin = json::glaze_safe_get<bool>(item, "builtin", false);
    rec.status = json::glaze_safe_get<std::string>(item, "status", "staged");
    rec.message = json::glaze_safe_get<std::string>(item, "message", "");
    return rec;
}

std::string StringValue(const JsonValue& value)
{
    if (value.isString()) {
        return value.asString();
    }
    if (value.isNumber()) {
        return std::to_string(value.asInt64());
    }
    if (value.isBool()) {
        return value.asBool() ? "true" : "false";
    }
    return "";
}

std::string FieldString(const JsonValue& object, const std::string& key, const std::string& fallback = "")
{
    if (!object.isObject() || !json::glaze_has_key(object, key)) {
        return fallback;
    }
    const std::string value = StringValue(json::glaze_get(object, key));
    return value.empty() ? fallback : value;
}

int64_t FieldInt(const JsonValue& object, const std::string& key, int64_t fallback = 0)
{
    if (!object.isObject() || !json::glaze_has_key(object, key)) {
        return fallback;
    }
    const JsonValue value = json::glaze_get(object, key);
    if (value.isNumber()) {
        return value.asInt64();
    }
    if (value.isString()) {
        try {
            return std::stoll(value.asString());
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}

std::string UrlEncode(const std::string& input)
{
    std::ostringstream out;
    out << std::hex << std::uppercase;
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out << static_cast<char>(c);
        } else {
            out << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }
    return out.str();
}

std::string ImageProxyUrl(int uid,
                          const std::string& token,
                          const std::string& source_id,
                          const std::string& image_url)
{
    if (image_url.empty()) {
        return "";
    }
    return "/api/r18/image?uid=" + std::to_string(uid) +
           "&token=" + UrlEncode(token) +
           "&source_id=" + UrlEncode(source_id) +
           "&image_url=" + UrlEncode(image_url);
}

std::string JmCoverUrl(const std::string& id)
{
    return std::string(kJmImageBase) + "/media/albums/" + id + "_3x4.jpg";
}

std::string JmImageUrl(const std::string& id, const std::string& image_name)
{
    return std::string(kJmImageBase) + "/media/photos/" + id + "/" + image_name;
}

bool IsAllowedJmImageUrl(const std::string& image_url)
{
    try {
        ParsedUrl parsed = ParseUrl(image_url);
        std::transform(parsed.scheme.begin(), parsed.scheme.end(), parsed.scheme.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        std::transform(parsed.host.begin(), parsed.host.end(), parsed.host.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return parsed.scheme == "https" &&
               parsed.host == kJmImageHost &&
               parsed.target.rfind("/media/", 0) == 0;
    } catch (...) {
        return false;
    }
}

JsonValue MakeTags(const std::vector<std::string>& tags)
{
    JsonValue arr{json::array_t{}};
    for (const auto& tag : tags) {
        if (!tag.empty()) {
            json::glaze_append(arr, tag);
        }
    }
    return arr;
}

JsonValue JmComicToJson(const JsonValue& comic, int uid, const std::string& token)
{
    const std::string id = FieldString(comic, "id");
    const JsonValue category = json::glaze_get(comic, "category");
    const JsonValue category_sub = json::glaze_get(comic, "category_sub");

    JsonValue item;
    item["source_id"] = kJmSourceId;
    item["comic_id"] = id;
    item["title"] = FieldString(comic, "name", id);
    item["subtitle"] = FieldString(comic, "author");
    item["description"] = FieldString(comic, "description");
    item["cover"] = ImageProxyUrl(uid, token, kJmSourceId, JmCoverUrl(id));
    item["author"] = FieldString(comic, "author");
    item["tags"] = MakeTags({
        FieldString(category, "title"),
        FieldString(category_sub, "title"),
    });
    return item;
}

ParsedUrl ParseUrl(const std::string& url)
{
    const auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        throw std::runtime_error("URL missing scheme");
    }
    ParsedUrl parsed;
    parsed.scheme = url.substr(0, scheme_end);
    const auto authority_begin = scheme_end + 3;
    const auto path_begin = url.find('/', authority_begin);
    std::string authority = path_begin == std::string::npos
        ? url.substr(authority_begin)
        : url.substr(authority_begin, path_begin - authority_begin);
    parsed.target = path_begin == std::string::npos ? "/" : url.substr(path_begin);
    const auto port_pos = authority.rfind(':');
    if (port_pos != std::string::npos && authority.find(']') == std::string::npos) {
        parsed.host = authority.substr(0, port_pos);
        parsed.port = authority.substr(port_pos + 1);
    } else {
        parsed.host = authority;
        parsed.port = parsed.scheme == "https" ? "443" : "80";
    }
    if (parsed.host.empty()) {
        throw std::runtime_error("URL missing host");
    }
    return parsed;
}

struct HttpResult {
    int status = 0;
    std::string content_type;
    std::string body;
};

std::mutex& JmImageFetchMutex()
{
    static std::mutex mu;
    return mu;
}

int& JmImageFetchCount()
{
    static int count = 0;
    return count;
}

std::string EscapeXml(std::string value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped.push_back(ch); break;
        }
    }
    return escaped;
}

class JmImageFetchSlot {
public:
    JmImageFetchSlot()
    {
        std::lock_guard<std::mutex> lock(JmImageFetchMutex());
        if (JmImageFetchCount() < kMaxConcurrentJmImageFetches) {
            ++JmImageFetchCount();
            acquired_ = true;
        }
    }

    ~JmImageFetchSlot()
    {
        if (!acquired_) {
            return;
        }
        std::lock_guard<std::mutex> lock(JmImageFetchMutex());
        --JmImageFetchCount();
    }

    bool acquired() const { return acquired_; }

private:
    bool acquired_ = false;
};

R18ImagePayload PlaceholderImage(const std::string& line1, const std::string& line2 = "")
{
    const std::string safe_line1 = EscapeXml(line1);
    const std::string safe_line2 = EscapeXml(line2);
    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"720\" height=\"1080\" viewBox=\"0 0 720 1080\">"
        << "<rect width=\"720\" height=\"1080\" fill=\"#201923\"/>"
        << "<rect x=\"48\" y=\"48\" width=\"624\" height=\"984\" rx=\"18\" fill=\"#2f2734\" stroke=\"#f2a7c5\" stroke-width=\"3\"/>"
        << "<text x=\"360\" y=\"500\" fill=\"#f8dce7\" font-size=\"36\" text-anchor=\"middle\" font-family=\"Arial\">"
        << safe_line1
        << "</text>";
    if (!safe_line2.empty()) {
        svg << "<text x=\"360\" y=\"558\" fill=\"#d6bac6\" font-size=\"22\" text-anchor=\"middle\" font-family=\"Arial\">"
            << safe_line2
            << "</text>";
    }
    svg << "</svg>";
    R18ImagePayload payload;
    payload.content_type = "image/svg+xml";
    payload.body = svg.str();
    return payload;
}

bool ReadCachedImage(const std::filesystem::path& cache_root,
                     const std::string& cache_key,
                     R18ImagePayload* payload)
{
    const auto body_path = cache_root / (cache_key + ".bin");
    const auto meta_path = cache_root / (cache_key + ".meta");
    std::ifstream body_in(body_path, std::ios::binary);
    if (!body_in.is_open()) {
        return false;
    }
    payload->body.assign(std::istreambuf_iterator<char>(body_in), std::istreambuf_iterator<char>());
    if (payload->body.empty()) {
        return false;
    }
    std::ifstream meta_in(meta_path, std::ios::binary);
    if (meta_in.is_open()) {
        std::getline(meta_in, payload->content_type);
    }
    if (payload->content_type.empty()) {
        payload->content_type = "image/jpeg";
    }
    return true;
}

void WriteCachedImage(const std::filesystem::path& cache_root,
                      const std::string& cache_key,
                      const R18ImagePayload& payload)
{
    std::error_code ec;
    std::filesystem::create_directories(cache_root, ec);
    if (ec) {
        return;
    }
    std::ofstream body_out(cache_root / (cache_key + ".bin"), std::ios::binary | std::ios::trunc);
    if (!body_out.is_open()) {
        return;
    }
    body_out.write(payload.body.data(), static_cast<std::streamsize>(payload.body.size()));
    std::ofstream meta_out(cache_root / (cache_key + ".meta"), std::ios::binary | std::ios::trunc);
    if (meta_out.is_open()) {
        meta_out << payload.content_type;
    }
}

HttpResult HttpGet(const std::string& url,
                   const std::vector<std::pair<std::string, std::string>>& headers,
                   int timeout_seconds = 20)
{
    const ParsedUrl parsed = ParseUrl(url);
    net::io_context ioc;
    tcp::resolver resolver(ioc);

    http::request<http::empty_body> req{http::verb::get, parsed.target, 11};
    req.set(http::field::host, parsed.host);
    req.set(http::field::user_agent, kJmUserAgent);
    req.set(http::field::accept_encoding, "identity");
    for (const auto& [key, value] : headers) {
        req.set(key, value);
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    if (parsed.scheme == "https") {
        ssl::context ctx(ssl::context::tls_client);
        ctx.set_verify_mode(ssl::verify_none);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
        if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str())) {
            throw std::runtime_error("failed to set TLS SNI host");
        }
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds));
        beast::get_lowest_layer(stream).connect(resolver.resolve(parsed.host, parsed.port));
        stream.handshake(ssl::stream_base::client);
        http::write(stream, req);
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds));
        http::read(stream, buffer, res);
        beast::error_code ec;
        stream.shutdown(ec);
    } else if (parsed.scheme == "http") {
        beast::tcp_stream stream(ioc);
        stream.expires_after(std::chrono::seconds(timeout_seconds));
        stream.connect(resolver.resolve(parsed.host, parsed.port));
        http::write(stream, req);
        stream.expires_after(std::chrono::seconds(timeout_seconds));
        http::read(stream, buffer, res);
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    } else {
        throw std::runtime_error("unsupported URL scheme");
    }

    HttpResult result;
    result.status = res.result_int();
    result.body = std::move(res.body());
    auto ct = res.find(http::field::content_type);
    if (ct != res.end()) {
        result.content_type.assign(ct->value().data(), ct->value().size());
    }
    return result;
}

std::string Md5Hex(const std::string& input)
{
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }
    const bool ok = EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) == 1 &&
                    EVP_DigestUpdate(ctx, input.data(), input.size()) == 1 &&
                    EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) == 1;
    EVP_MD_CTX_free(ctx);
    if (!ok) {
        throw std::runtime_error("MD5 failed");
    }
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i) {
        out << std::setw(2) << static_cast<int>(digest[i]);
    }
    return out.str();
}

std::string Aes256EcbDecrypt(const std::string& cipher_text, const std::string& key)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    }
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
    if (!ok) {
        throw std::runtime_error("AES decrypt failed");
    }
    return std::string(reinterpret_cast<char*>(out.data()), static_cast<std::size_t>(out_len_1 + out_len_2));
}

std::string TrimJsonPayload(const std::string& value)
{
    const auto start = value.find_first_of("[{");
    const auto end = value.find_last_of("]}");
    if (start == std::string::npos || end == std::string::npos || end < start) {
        throw std::runtime_error("decrypted payload is not JSON");
    }
    return value.substr(start, end - start + 1);
}

std::vector<std::pair<std::string, std::string>> JmApiHeaders(std::int64_t unix_time)
{
    const std::string time = std::to_string(unix_time);
    const std::string token = Md5Hex(time + "18comicAPPContent");
    return {
        {"Accept", "*/*"},
        {"Accept-Language", "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7"},
        {"Authorization", "Bearer"},
        {"Connection", "keep-alive"},
        {"Origin", "https://localhost"},
        {"Referer", "https://localhost/"},
        {"Sec-Fetch-Dest", "empty"},
        {"Sec-Fetch-Mode", "cors"},
        {"Sec-Fetch-Site", "cross-site"},
        {"Sec-Fetch-Storage-Access", "active"},
        {"User-Agent", kJmUserAgent},
        {"X-Requested-With", kJmPackageName},
        {"token", token},
        {"tokenparam", time + "," + kJmVersion},
    };
}

JsonValue JmApiGet(const std::string& target)
{
    const auto now = std::chrono::system_clock::now();
    const auto unix_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::string last_error;
    for (const auto& host : kJmApiHosts) {
        try {
            const std::string url = "https://" + host + target;
            const HttpResult response = HttpGet(url, JmApiHeaders(unix_time), kJmApiTimeoutSeconds);
            if (response.status != 200) {
                last_error = host + " HTTP " + std::to_string(response.status);
                continue;
            }

            JsonValue encrypted_root;
            if (!json::glaze_parse(encrypted_root, response.body)) {
                last_error = host + " returned invalid JSON";
                continue;
            }
            const std::string encrypted_data = json::glaze_safe_get<std::string>(encrypted_root, "data", "");
            if (encrypted_data.empty()) {
                last_error = host + " returned empty encrypted payload";
                continue;
            }
            std::string cipher_text;
            if (!DecodeBase64(encrypted_data, cipher_text)) {
                last_error = host + " returned invalid base64 payload";
                continue;
            }
            const std::string key = Md5Hex(std::to_string(unix_time) + "185Hcomic3PAPP7R");
            const std::string clear = TrimJsonPayload(Aes256EcbDecrypt(cipher_text, key));
            JsonValue result;
            if (!json::glaze_parse(result, clear)) {
                last_error = host + " decrypted payload parse failed";
                continue;
            }
            return result;
        } catch (const std::exception& exc) {
            last_error = host + ": " + exc.what();
        }
    }
    throw std::runtime_error(last_error.empty() ? "JM official API request failed" : last_error);
}

JsonValue ErrorData(const std::string& source_id, const std::string& message)
{
    JsonValue data;
    data["source_id"] = source_id;
    data["items"] = JsonValue{json::array_t{}};
    data["error_message"] = message;

    JsonValue item;
    item["source_id"] = source_id;
    item["comic_id"] = "";
    item["title"] = "官方源请求失败";
    item["subtitle"] = message;
    item["description"] = message;
    item["cover"] = "";
    item["tags"] = JsonValue{json::array_t{}};
    json::glaze_append(data["items"], item);
    return data;
}

JsonValue JmSearch(const std::string& keyword, int page, int uid, const std::string& token)
{
    const int normalized_page = page < 1 ? 1 : page;
    const std::string normalized_keyword = keyword.empty() ? "" : UrlEncode(keyword);
    std::string target = "/search?search_query=" + normalized_keyword + "&o=mr";
    if (normalized_page > 1) {
        target += "&page=" + std::to_string(normalized_page);
    }
    const JsonValue result = JmApiGet(target);

    JsonValue data;
    data["source_id"] = kJmSourceId;
    data["keyword"] = keyword;
    data["page"] = normalized_page;
    data["items"] = JsonValue{json::array_t{}};
    data["max_page"] = static_cast<int64_t>((FieldInt(result, "total", 0) + kR18SearchPageSize - 1) / kR18SearchPageSize);

    const JsonValue content = json::glaze_get(result, "content");
    if (const auto* items = json::glaze_get_array(content)) {
        int count = 0;
        for (const auto& entry : *items) {
            if (count >= kR18SearchPageSize) {
                break;
            }
            json::glaze_append(data["items"], JmComicToJson(JsonValue(entry), uid, token));
            ++count;
        }
    }
    return data;
}

JsonValue JmDetail(const std::string& comic_id, int uid, const std::string& token)
{
    std::string id = comic_id;
    if (id.rfind("jm", 0) == 0) {
        id = id.substr(2);
    }
    const JsonValue result = JmApiGet("/album?id=" + UrlEncode(id));

    JsonValue data;
    data["source_id"] = kJmSourceId;
    data["comic_id"] = id;
    data["title"] = FieldString(result, "name", id);
    data["description"] = FieldString(result, "description");
    data["cover"] = ImageProxyUrl(uid, token, kJmSourceId, JmCoverUrl(id));
    data["likes"] = FieldInt(result, "likes", 0);
    data["views"] = FieldString(result, "total_views");
    data["favorite"] = json::glaze_safe_get<bool>(result, "is_favorite", false);
    data["chapters"] = JsonValue{json::array_t{}};

    std::vector<JsonValue> chapters;
    const JsonValue series = json::glaze_get(result, "series");
    if (const auto* arr = json::glaze_get_array(series)) {
        for (const auto& entry : *arr) {
            chapters.emplace_back(entry);
        }
        std::sort(chapters.begin(), chapters.end(), [](const JsonValue& a, const JsonValue& b) {
            return FieldInt(a, "sort", 0) < FieldInt(b, "sort", 0);
        });
    }
    if (chapters.empty()) {
        JsonValue chapter;
        chapter["source_id"] = kJmSourceId;
        chapter["comic_id"] = id;
        chapter["chapter_id"] = id;
        chapter["title"] = "第1話";
        chapter["order"] = 1;
        json::glaze_append(data["chapters"], chapter);
    } else {
        int order = 1;
        for (const auto& entry : chapters) {
            JsonValue chapter;
            const std::string chapter_id = FieldString(entry, "id", id);
            std::string title = FieldString(entry, "name");
            if (title.empty()) {
                title = "第" + std::to_string(FieldInt(entry, "sort", order)) + "話";
            }
            chapter["source_id"] = kJmSourceId;
            chapter["comic_id"] = id;
            chapter["chapter_id"] = chapter_id;
            chapter["title"] = title;
            chapter["order"] = order++;
            json::glaze_append(data["chapters"], chapter);
        }
    }
    return data;
}

JsonValue JmPages(const std::string& chapter_id, int uid, const std::string& token)
{
    const JsonValue result = JmApiGet("/chapter?id=" + UrlEncode(chapter_id));
    JsonValue data;
    data["source_id"] = kJmSourceId;
    data["chapter_id"] = chapter_id;
    data["pages"] = JsonValue{json::array_t{}};

    const JsonValue images = json::glaze_get(result, "images");
    int index = 1;
    if (const auto* arr = json::glaze_get_array(images)) {
        for (const auto& entry : *arr) {
            const std::string image_name = StringValue(JsonValue(entry));
            JsonValue page;
            page["index"] = index;
            page["image_id"] = chapter_id + "-p" + std::to_string(index);
            page["url"] = ImageProxyUrl(uid, token, kJmSourceId, JmImageUrl(chapter_id, image_name));
            json::glaze_append(data["pages"], page);
            ++index;
        }
    }
    return data;
}

} // namespace

R18SourceService& R18SourceService::Instance()
{
    static R18SourceService svc;
    return svc;
}

R18SourceService::R18SourceService()
{
    data_root_ = ResolveDataRoot();
    image_cache_root_ = data_root_.parent_path() / "image-cache";
    std::error_code ec;
    std::filesystem::create_directories(data_root_, ec);
    std::filesystem::create_directories(image_cache_root_, ec);
    InstallBuiltinSourcesLocked();
    LoadLocked();
}

void R18SourceService::InstallBuiltinSourcesLocked()
{
    // Intentionally empty: the source list now starts with no preloaded sources.
}

JsonValue R18SourceService::ListSources()
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    JsonValue arr{json::array_t{}};
    const auto append_source = [this, &arr](const std::string& id) {
        const auto it = sources_.find(id);
        if (it != sources_.end()) {
            json::glaze_append(arr, ToJson(it->second));
        }
    };
    append_source(kJmSourceId);
    append_source(kMockSourceId);
    for (const auto& [id, source] : sources_) {
        if (id != kJmSourceId && id != kMockSourceId) {
            json::glaze_append(arr, ToJson(source));
        }
    }
    return arr;
}

bool R18SourceService::EnableSource(const std::string& id, bool enabled, std::string* error)
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    auto it = sources_.find(id);
    if (it == sources_.end()) {
        if (error) *error = "source not found";
        return false;
    }
    it->second.enabled = enabled;
    SaveLocked();
    return true;
}

R18SourceRecord R18SourceService::ImportZip(const std::string& file_name,
                                            const std::string& manifest_json,
                                            const std::string& binary,
                                            std::string* error)
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    const bool zip_payload = binary.size() >= 4 && binary[0] == 'P' && binary[1] == 'K';
    const bool js_payload = LooksLikeJavaScript(file_name, manifest_json, binary);
    if (!zip_payload && !js_payload) {
        if (error) *error = "plugin package must be a zip file or JavaScript source";
        return {};
    }

    JsonValue manifest;
    if (!manifest_json.empty() && !json::glaze_parse(manifest, manifest_json)) {
        if (error) *error = "manifest_json is invalid";
        return {};
    }

    const std::string fallback_id = NormalizeId(Stem(file_name));
    R18SourceRecord rec;
    rec.id = manifest_json.empty() ? fallback_id : json::glaze_safe_get<std::string>(manifest, "id", fallback_id);
    rec.id = NormalizeId(rec.id);
    rec.name = manifest_json.empty() ? rec.id : json::glaze_safe_get<std::string>(manifest, "name", rec.id);
    rec.version = manifest_json.empty() ? "0.0.0" : json::glaze_safe_get<std::string>(manifest, "version", "0.0.0");
    rec.version = NormalizePathSegment(rec.version, "0.0.0");
    rec.format = js_payload ? "source-js" : json::glaze_safe_get<std::string>(manifest, "format", "native-zip");
    rec.source_url = manifest_json.empty() ? "" : json::glaze_safe_get<std::string>(manifest, "source_url", "");
    rec.catalog_url = manifest_json.empty() ? "" : json::glaze_safe_get<std::string>(manifest, "catalog_url", "");
    rec.enabled = false;
    rec.builtin = false;
    rec.status = js_payload ? "staged-js" : "staged";
    rec.message = js_payload
        ? "JavaScript source saved. Execution requires a MemoChat source runtime adapter."
        : "Package staged. Build/unpack validation is handled by the plugin host deployment step.";

    if (rec.id.empty()) {
        if (error) *error = "source id is empty";
        return {};
    }
    if (IsBuiltinSourceId(rec.id)) {
        if (error) *error = "source id is reserved for a built-in adapter";
        return {};
    }

    const auto dir = data_root_ / rec.id / rec.version;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        if (error) *error = "failed to create source directory";
        return {};
    }
    const auto source_path = dir / (js_payload ? "source.js" : "source.zip");
    std::ofstream out(source_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        if (error) *error = "failed to persist source package";
        return {};
    }
    out.write(binary.data(), static_cast<std::streamsize>(binary.size()));
    out.close();
    if (!manifest_json.empty()) {
        std::ofstream manifest_out(dir / "manifest.json", std::ios::binary | std::ios::trunc);
        manifest_out << manifest_json;
    }

    rec.path = source_path.string();
    sources_[rec.id] = rec;
    SaveLocked();
    return rec;
}

JsonValue R18SourceService::Search(const std::string& source_id,
                                   const std::string& keyword,
                                   int page,
                                   int uid,
                                   const std::string& token)
{
    if (source_id == kJmSourceId) {
        try {
            return JmSearch(keyword, page < 1 ? 1 : page, uid, token);
        } catch (const std::exception& exc) {
            return ErrorData(kJmSourceId, exc.what());
        }
    }

    JsonValue data;
    data["source_id"] = source_id;
    data["keyword"] = keyword;
    data["page"] = page;
    data["items"] = JsonValue{json::array_t{}};
    data["max_page"] = 1;

    JsonValue first;
    first["source_id"] = source_id;
    first["comic_id"] = "mock-" + std::to_string(page) + "-1";
    first["title"] = keyword.empty() ? "R18 Preview Comic" : ("R18 Preview: " + keyword);
    first["subtitle"] = SourceSnapshot(source_id).value_or(R18SourceRecord{}).message;
    first["cover"] = "/api/r18/image?uid=" + std::to_string(uid) + "&token=" + UrlEncode(token) +
                     "&source_id=" + UrlEncode(source_id) + "&image_id=cover";
    first["author"] = source_id;
    first["tags"] = MakeTags({"sample"});
    json::glaze_append(data["items"], first);
    return data;
}

JsonValue R18SourceService::Detail(const std::string& source_id,
                                   const std::string& comic_id,
                                   int uid,
                                   const std::string& token)
{
    if (source_id == kJmSourceId) {
        try {
            return JmDetail(comic_id, uid, token);
        } catch (const std::exception& exc) {
            JsonValue data;
            data["source_id"] = kJmSourceId;
            data["comic_id"] = comic_id;
            data["title"] = "官方源请求失败";
            data["description"] = exc.what();
            data["cover"] = "";
            data["chapters"] = JsonValue{json::array_t{}};
            return data;
        }
    }

    const auto source = SourceSnapshot(source_id);
    JsonValue data;
    data["source_id"] = source_id;
    data["comic_id"] = comic_id;
    data["title"] = (source && !source->name.empty() ? source->name : "Unknown Source") + " Preview Comic";
    data["description"] = source ? source->message : "The selected source was not found.";
    data["cover"] = "/api/r18/image?uid=" + std::to_string(uid) + "&token=" + UrlEncode(token) +
                    "&source_id=" + UrlEncode(source_id) + "&image_id=cover";
    data["chapters"] = JsonValue{json::array_t{}};
    for (int i = 1; i <= 3; ++i) {
        JsonValue ch;
        ch["source_id"] = source_id;
        ch["comic_id"] = comic_id;
        ch["chapter_id"] = comic_id + "-ch" + std::to_string(i);
        ch["title"] = "Chapter " + std::to_string(i);
        ch["order"] = i;
        json::glaze_append(data["chapters"], ch);
    }
    return data;
}

JsonValue R18SourceService::Pages(const std::string& source_id,
                                  const std::string& chapter_id,
                                  int uid,
                                  const std::string& token)
{
    if (source_id == kJmSourceId) {
        try {
            return JmPages(chapter_id, uid, token);
        } catch (const std::exception& exc) {
            JsonValue data;
            data["source_id"] = kJmSourceId;
            data["chapter_id"] = chapter_id;
            data["error_message"] = exc.what();
            data["pages"] = JsonValue{json::array_t{}};
            return data;
        }
    }

    JsonValue data;
    data["source_id"] = source_id;
    data["chapter_id"] = chapter_id;
    data["pages"] = JsonValue{json::array_t{}};
    for (int i = 1; i <= 5; ++i) {
        JsonValue page;
        page["index"] = i;
        page["image_id"] = chapter_id + "-p" + std::to_string(i);
        page["url"] = "/api/r18/image?uid=" + std::to_string(uid) + "&token=" + UrlEncode(token) +
                      "&source_id=" + UrlEncode(source_id) + "&image_id=" + UrlEncode(chapter_id + "-p" + std::to_string(i));
        json::glaze_append(data["pages"], page);
    }
    return data;
}

R18ImagePayload R18SourceService::FetchImage(const std::string& source_id, const std::string& image_url)
{
    if (source_id == kJmSourceId && !image_url.empty()) {
        if (!IsAllowedJmImageUrl(image_url)) {
            throw std::runtime_error("image url is not an allowed JM media URL");
        }
        const std::string cache_key = Md5Hex(image_url);
        R18ImagePayload cached;
        if (ReadCachedImage(image_cache_root_, cache_key, &cached)) {
            return cached;
        }
        JmImageFetchSlot slot;
        if (!slot.acquired()) {
            return PlaceholderImage("JMComic cover queued", "scroll or refresh after images cache");
        }
        std::vector<std::pair<std::string, std::string>> headers = {
            {"Accept", "image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8"},
            {"Accept-Language", "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7"},
            {"Referer", "https://localhost/"},
            {"Sec-Fetch-Dest", "image"},
            {"Sec-Fetch-Mode", "no-cors"},
            {"Sec-Fetch-Site", "cross-site"},
            {"Sec-Fetch-Storage-Access", "active"},
            {"User-Agent", kJmUserAgent},
            {"X-Requested-With", kJmPackageName},
        };
        try {
            HttpResult result = HttpGet(image_url, headers, kJmImageTimeoutSeconds);
            if (result.status < 200 || result.status >= 300 || result.body.empty()) {
                return PlaceholderImage("JMComic image unavailable", "HTTP " + std::to_string(result.status));
            }
            R18ImagePayload payload;
            payload.content_type = result.content_type.empty() ? "image/jpeg" : result.content_type;
            payload.body = std::move(result.body);
            WriteCachedImage(image_cache_root_, cache_key, payload);
            return payload;
        } catch (const std::exception& exc) {
            return PlaceholderImage("JMComic image timeout", exc.what());
        }
    }

    return PlaceholderImage("R18 Source Image", "preview");
}

std::optional<R18SourceRecord> R18SourceService::SourceSnapshot(const std::string& source_id)
{
    std::lock_guard<std::mutex> lock(mu_);
    InstallBuiltinSourcesLocked();
    LoadLocked();
    auto it = sources_.find(source_id);
    if (it == sources_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void R18SourceService::LoadLocked()
{
    const auto manifest_path = data_root_ / "sources.json";
    const bool has_shared_manifest = std::filesystem::exists(manifest_path);
    LoadManifestLocked(manifest_path);
    const auto legacy_root = std::filesystem::current_path() / "data" / "r18" / "sources";
    if (!has_shared_manifest && legacy_root != data_root_) {
        const auto before = sources_.size();
        LoadManifestLocked(legacy_root / "sources.json");
        if (sources_.size() != before) {
            SaveLocked();
        }
    }
}

void R18SourceService::LoadManifestLocked(const std::filesystem::path& manifest_path)
{
    std::ifstream in(manifest_path, std::ios::binary);
    if (!in.is_open()) {
        return;
    }
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    JsonValue root;
    if (!json::glaze_parse(root, content)) {
        return;
    }
    JsonValue sources = json::glaze_get(root, "sources");
    auto arr = json::glaze_get_array(sources);
    if (!arr) {
        return;
    }
    for (const auto& item_json : *arr) {
        const R18SourceRecord rec = FromJson(JsonValue(item_json));
        if (!rec.id.empty() && !rec.builtin && !IsBuiltinSourceId(rec.id)) {
            sources_[rec.id] = rec;
        }
    }
}

void R18SourceService::SaveLocked()
{
    JsonValue root;
    root["sources"] = JsonValue{json::array_t{}};
    for (const auto& [_, source] : sources_) {
        if (!source.builtin) {
            json::glaze_append(root["sources"], ToJson(source));
        }
    }
    std::ofstream out(data_root_ / "sources.json", std::ios::binary | std::ios::trunc);
    out << json::glaze_stringify(root);
}

bool DecodeBase64(const std::string& input, std::string& out)
{
    static constexpr unsigned char kInvalid = 255;
    unsigned char table[256];
    std::fill(std::begin(table), std::end(table), kInvalid);
    for (int i = 0; i < 26; ++i) {
        table[static_cast<unsigned char>('A' + i)] = static_cast<unsigned char>(i);
        table[static_cast<unsigned char>('a' + i)] = static_cast<unsigned char>(26 + i);
    }
    for (int i = 0; i < 10; ++i) table[static_cast<unsigned char>('0' + i)] = static_cast<unsigned char>(52 + i);
    table[static_cast<unsigned char>('+')] = 62;
    table[static_cast<unsigned char>('/')] = 63;

    out.clear();
    int val = 0;
    int bits = -8;
    for (unsigned char c : input) {
        if (std::isspace(c)) continue;
        if (c == '=') break;
        if (table[c] == kInvalid) return false;
        val = (val << 6) + table[c];
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<char>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return true;
}

} // namespace memochat::r18
