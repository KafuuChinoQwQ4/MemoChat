#include "r18/R18JmAdapter.hpp"
#include "r18/R18AdapterUtils.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <turbojpeg.h>

import memochat.r18.jm_adapter_algorithms;

namespace memochat::r18
{

const char* const kJmSourceId = "jm.official";

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

std::mutex& JmImageFetchMutex()
{
    static std::mutex mutex;
    return mutex;
}

std::condition_variable& JmImageFetchCv()
{
    static std::condition_variable condition;
    return condition;
}

int& JmImageFetchCount()
{
    static int count = 0;
    return count;
}

class JmImageFetchSlot
{
public:
    JmImageFetchSlot()
    {
        std::unique_lock<std::mutex> lock(JmImageFetchMutex());
        JmImageFetchCv().wait(lock,
                              []
                              {
                                  return JmImageFetchCount() < jm_adapter::modules::MaxConcurrentImageFetches();
                              });
        ++JmImageFetchCount();
    }

    ~JmImageFetchSlot()
    {
        {
            std::lock_guard<std::mutex> lock(JmImageFetchMutex());
            --JmImageFetchCount();
        }
        JmImageFetchCv().notify_one();
    }
};

std::string JmCoverUrl(const std::string& id)
{
    return std::string(jm_adapter::modules::ImageBaseUrl()) + "/media/albums/" + id + "_3x4.jpg";
}

std::string JmImageUrl(const std::string& id, const std::string& image_name)
{
    return std::string(jm_adapter::modules::ImageBaseUrl()) + "/media/photos/" + id + "/" + image_name;
}

JsonValue JmComicToJson(const JsonValue& comic)
{
    const std::string id = FieldString(comic, "id");
    const JsonValue category = json::glaze_get(comic, "category");
    const JsonValue category_sub = json::glaze_get(comic, "category_sub");

    JsonValue item;
    item["source_id"] = jm_adapter::modules::SourceId();
    item["comic_id"] = id;
    item["title"] = FieldString(comic, "name", id);
    item["subtitle"] = FieldString(comic, "author");
    item["description"] = FieldString(comic, "description");
    item["cover"] = ImageProxyUrl(jm_adapter::modules::SourceId(), JmCoverUrl(id));
    item["author"] = FieldString(comic, "author");
    item["tags"] = MakeTags({FieldString(category, "title"), FieldString(category_sub, "title")});
    return item;
}

// uid is empty for unauthenticated requests; non-empty sets "Bearer <uid>".
bool JmApiHeaders(std::int64_t unix_time,
                  const std::string& uid,
                  std::vector<std::pair<std::string, std::string>>* out,
                  std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "JM headers output pointer is null");
        return false;
    }
    const std::string time = std::to_string(unix_time);
    std::string token;
    if (!Md5Hex(time + "18comicAPPContent", &token, error))
    {
        return false;
    }
    const std::string auth = uid.empty() ? std::string(jm_adapter::modules::AuthorizationBearer())
                                         : std::string(jm_adapter::modules::AuthorizationBearer()) + " " + uid;
    *out = {
        {"Accept", "*/*"},
        {"Accept-Language", "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7"},
        {"Authorization", auth},
        {"Connection", "keep-alive"},
        {"Origin", "https://localhost"},
        {"Referer", "https://localhost/"},
        {"Sec-Fetch-Dest", "empty"},
        {"Sec-Fetch-Mode", "cors"},
        {"Sec-Fetch-Site", "cross-site"},
        {"Sec-Fetch-Storage-Access", "active"},
        {"User-Agent", jm_adapter::modules::UserAgent()},
        {"X-Requested-With", jm_adapter::modules::PackageName()},
        {"token", token},
        {"tokenparam", time + "," + jm_adapter::modules::Version()},
    };
    return true;
}

bool TrimJsonPayload(const std::string& value, std::string* out, std::string* error);
bool JmApiGet(const std::string& target, const std::string& uid, JsonValue* out, std::string* error);

bool JmApiPost(const std::string& target,
               const std::string& uid,
               const std::string& body,
               JsonValue* out,
               std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "JM API output pointer is null");
        return false;
    }
    const auto now = std::chrono::system_clock::now();
    const auto unix_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::vector<std::pair<std::string, std::string>> headers;
    if (!JmApiHeaders(unix_time, uid, &headers, error))
    {
        return false;
    }

    std::string last_error;
    for (int index = 0; index < jm_adapter::modules::ApiHostCount(); ++index)
    {
        const std::string host = jm_adapter::modules::ApiHostAt(index);
        const std::string url = "https://" + host + target;
        HttpResult response;
        std::string request_error;
        if (!HttpPost(url, headers, body, &response, &request_error, jm_adapter::modules::LoginTimeoutSeconds()))
        {
            last_error = host + ": " + request_error;
            continue;
        }
        if (response.status != 200)
        {
            last_error = host + " HTTP " + std::to_string(response.status);
            continue;
        }
        JsonValue encrypted_root;
        if (!json::glaze_parse(encrypted_root, response.body))
        {
            last_error = host + " returned invalid JSON";
            continue;
        }
        const std::string encrypted_data = json::glaze_safe_get<std::string>(encrypted_root, "data", "");
        if (encrypted_data.empty())
        {
            last_error = host + " returned empty encrypted payload";
            continue;
        }
        std::string cipher_text;
        if (!DecodeBase64(encrypted_data, cipher_text))
        {
            last_error = host + " returned invalid base64 payload";
            continue;
        }
        std::string key;
        if (!Md5Hex(std::to_string(unix_time) + "185Hcomic3PAPP7R", &key, &request_error))
        {
            last_error = host + ": " + request_error;
            continue;
        }
        std::string decrypted;
        if (!Aes256EcbDecrypt(cipher_text, key, &decrypted, &request_error))
        {
            last_error = host + ": " + request_error;
            continue;
        }
        std::string clear;
        if (!TrimJsonPayload(decrypted, &clear, &request_error))
        {
            last_error = host + ": " + request_error;
            continue;
        }
        JsonValue result;
        if (!json::glaze_parse(result, clear))
        {
            last_error = host + " decrypted payload parse failed";
            continue;
        }
        *out = std::move(result);
        return true;
    }

    SetError(error, last_error.empty() ? jm_adapter::modules::OfficialApiFailureMessage() : last_error);
    return false;
}

bool TrimJsonPayload(const std::string& value, std::string* out, std::string* error)
{
    const auto start = value.find_first_of("[{");
    const auto end = value.find_last_of("]}");
    if (!jm_adapter::modules::ShouldTrimJsonPayload(start == std::string::npos, end == std::string::npos, end < start))
    {
        SetError(error, jm_adapter::modules::InvalidDecryptedPayloadMessage());
        return false;
    }
    *out = value.substr(start, end - start + 1);
    return true;
}

bool JmApiGet(const std::string& target, const std::string& uid, JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "JM API output pointer is null");
        return false;
    }
    const auto now = std::chrono::system_clock::now();
    const auto unix_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::vector<std::pair<std::string, std::string>> headers;
    if (!JmApiHeaders(unix_time, uid, &headers, error))
    {
        return false;
    }

    std::string last_error;
    for (int index = 0; index < jm_adapter::modules::ApiHostCount(); ++index)
    {
        const std::string host = jm_adapter::modules::ApiHostAt(index);
        const std::string url = "https://" + host + target;
        HttpResult response;
        std::string request_error;
        if (!HttpGet(url, headers, &response, &request_error, jm_adapter::modules::ApiTimeoutSeconds()))
        {
            last_error = host + ": " + request_error;
            continue;
        }
        if (response.status != 200)
        {
            last_error = host + " HTTP " + std::to_string(response.status);
            continue;
        }
        JsonValue encrypted_root;
        if (!json::glaze_parse(encrypted_root, response.body))
        {
            last_error = host + " returned invalid JSON";
            continue;
        }
        const std::string encrypted_data = json::glaze_safe_get<std::string>(encrypted_root, "data", "");
        if (encrypted_data.empty())
        {
            last_error = host + " returned empty encrypted payload";
            continue;
        }
        std::string cipher_text;
        if (!DecodeBase64(encrypted_data, cipher_text))
        {
            last_error = host + " returned invalid base64 payload";
            continue;
        }
        std::string key;
        if (!Md5Hex(std::to_string(unix_time) + "185Hcomic3PAPP7R", &key, &request_error))
        {
            last_error = host + ": " + request_error;
            continue;
        }
        std::string decrypted;
        if (!Aes256EcbDecrypt(cipher_text, key, &decrypted, &request_error))
        {
            last_error = host + ": " + request_error;
            continue;
        }
        std::string clear;
        if (!TrimJsonPayload(decrypted, &clear, &request_error))
        {
            last_error = host + ": " + request_error;
            continue;
        }
        JsonValue result;
        if (!json::glaze_parse(result, clear))
        {
            last_error = host + " decrypted payload parse failed";
            continue;
        }
        *out = std::move(result);
        return true;
    }

    SetError(error, last_error.empty() ? jm_adapter::modules::OfficialApiFailureMessage() : last_error);
    return false;
}

} // namespace

bool IsAllowedJmImageUrl(const std::string& image_url)
{
    ParsedUrl parsed;
    if (!detail::ParseUrl(image_url, &parsed, nullptr))
    {
        return false;
    }
    std::transform(parsed.scheme.begin(),
                   parsed.scheme.end(),
                   parsed.scheme.begin(),
                   [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });
    std::transform(parsed.host.begin(),
                   parsed.host.end(),
                   parsed.host.begin(),
                   [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });
    return jm_adapter::modules::IsAllowedImageUrl(parsed.scheme == "https",
                                                  parsed.host == jm_adapter::modules::ImageHost(),
                                                  parsed.target.rfind(jm_adapter::modules::ImageTargetPrefix(), 0) ==
                                                      0);
}

bool JmSearch(const std::string& keyword,
              int page,
              const std::string& sort,
              const std::string& tag,
              json::JsonValue* out,
              std::string* error)
{
    return JmSearchWithToken(keyword, page, sort, tag, "", out, error);
}

bool JmSearchWithToken(const std::string& keyword,
                       int page,
                       const std::string& sort,
                       const std::string& tag,
                       const std::string& uid,
                       json::JsonValue* out,
                       std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "JM search output pointer is null");
        return false;
    }
    const int normalized_page = jm_adapter::modules::NormalizeSearchPage(page);
    const std::string resolved_sort = jm_adapter::modules::NormalizeSearchSort(sort.c_str());
    // Combine free-text keyword with category/tag filter when both are present.
    std::string query = keyword;
    if (!tag.empty())
    {
        if (query.empty())
            query = tag;
        else if (query.find(tag) == std::string::npos)
            query = query + " " + tag;
    }
    const std::string normalized_query = query.empty() ? "" : detail::UrlEncode(query);
    std::string target = "/search?search_query=" + normalized_query + "&o=" + resolved_sort;
    if (jm_adapter::modules::ShouldAppendSearchPage(normalized_page))
        target += "&page=" + std::to_string(normalized_page);
    json::JsonValue result;
    if (!JmApiGet(target, uid, &result, error))
    {
        return false;
    }

    json::JsonValue data;
    data["source_id"] = jm_adapter::modules::SourceId();
    data["keyword"] = keyword;
    data["sort"] = resolved_sort;
    data["tag"] = tag;
    data["page"] = normalized_page;
    data["items"] = json::JsonValue{json::array_t{}};

    const json::JsonValue content = json::glaze_get(result, "content");
    int returned_count = 0;
    if (const auto* items = json::glaze_get_array(content))
    {
        for (const auto& entry : *items)
        {
            if (returned_count >= jm_adapter::modules::SearchPageSize())
                break;
            json::glaze_append(data["items"], JmComicToJson(json::JsonValue(entry)));
            ++returned_count;
        }
    }
    data["max_page"] = static_cast<int64_t>(
        jm_adapter::modules::ShouldUseNextSearchPage(returned_count, jm_adapter::modules::SearchPageSize())
            ? normalized_page + 1
            : normalized_page);
    *out = std::move(data);
    return true;
}

bool JmDetail(const std::string& comic_id, json::JsonValue* out, std::string* error)
{
    return JmDetailWithToken(comic_id, "", out, error);
}

bool JmDetailWithToken(const std::string& comic_id, const std::string& uid, json::JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "JM detail output pointer is null");
        return false;
    }
    std::string id = comic_id;
    if (jm_adapter::modules::ShouldStripJmPrefix(id.rfind("jm", 0) == 0))
        id = id.substr(2);
    json::JsonValue result;
    if (!JmApiGet("/album?id=" + detail::UrlEncode(id), uid, &result, error))
    {
        return false;
    }

    json::JsonValue data;
    data["source_id"] = jm_adapter::modules::SourceId();
    data["comic_id"] = id;
    data["title"] = detail::FieldString(result, "name", id);
    data["description"] = detail::FieldString(result, "description");
    data["cover"] = detail::ImageProxyUrl(jm_adapter::modules::SourceId(), JmCoverUrl(id));
    data["likes"] = detail::FieldInt(result, "likes", 0);
    data["views"] = detail::FieldString(result, "total_views");
    data["favorite"] = json::glaze_safe_get<bool>(result, "is_favorite", false);
    data["chapters"] = json::JsonValue{json::array_t{}};

    std::vector<json::JsonValue> chapters;
    const json::JsonValue series = json::glaze_get(result, "series");
    if (const auto* values = json::glaze_get_array(series))
    {
        for (const auto& entry : *values)
            chapters.emplace_back(entry);
        std::sort(chapters.begin(),
                  chapters.end(),
                  [](const json::JsonValue& left, const json::JsonValue& right)
                  {
                      return detail::FieldInt(left, "sort", 0) < detail::FieldInt(right, "sort", 0);
                  });
    }
    if (jm_adapter::modules::ShouldUseFallbackChapter(chapters.empty()))
    {
        json::JsonValue chapter;
        chapter["source_id"] = jm_adapter::modules::SourceId();
        chapter["comic_id"] = id;
        chapter["chapter_id"] = id;
        chapter["title"] = jm_adapter::modules::DefaultChapterTitle();
        chapter["order"] = jm_adapter::modules::DefaultChapterOrder();
        json::glaze_append(data["chapters"], chapter);
    }
    else
    {
        int order = 1;
        for (const auto& entry : chapters)
        {
            json::JsonValue chapter;
            const std::string chapter_id = detail::FieldString(entry, "id", id);
            std::string title = detail::FieldString(entry, "name");
            if (title.empty())
                title = "第" + std::to_string(detail::FieldInt(entry, "sort", order)) + "話";
            chapter["source_id"] = jm_adapter::modules::SourceId();
            chapter["comic_id"] = id;
            chapter["chapter_id"] = chapter_id;
            chapter["title"] = title;
            chapter["order"] = order++;
            json::glaze_append(data["chapters"], chapter);
        }
    }
    *out = std::move(data);
    return true;
}

bool JmPages(const std::string& chapter_id, json::JsonValue* out, std::string* error)
{
    return JmPagesWithToken(chapter_id, "", out, error);
}

bool JmPagesWithToken(const std::string& chapter_id, const std::string& uid, json::JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "JM pages output pointer is null");
        return false;
    }
    json::JsonValue result;
    if (!JmApiGet("/chapter?id=" + detail::UrlEncode(chapter_id), uid, &result, error))
    {
        return false;
    }
    json::JsonValue data;
    data["source_id"] = jm_adapter::modules::SourceId();
    data["chapter_id"] = chapter_id;
    data["pages"] = json::JsonValue{json::array_t{}};

    const json::JsonValue images = json::glaze_get(result, "images");
    const long long scramble_id = detail::FieldInt(result, "scramble_id", jm_adapter::modules::ScrambleIdThreshold());
    int index = 1;
    if (const auto* values = json::glaze_get_array(images))
    {
        for (const auto& entry : *values)
        {
            const std::string image_name = detail::StringValue(json::JsonValue(entry));
            json::JsonValue page;
            page["index"] = index;
            page["image_id"] = chapter_id + "-p" + std::to_string(index);
            page["url"] = detail::ImageProxyUrl(jm_adapter::modules::SourceId(), JmImageUrl(chapter_id, image_name)) +
                          "&scramble_id=" + std::to_string(scramble_id);
            json::glaze_append(data["pages"], page);
            ++index;
        }
    }
    *out = std::move(data);
    return true;
}

namespace
{

bool ParseJmPhotoParts(const std::string& image_url, long long* chapter_id_out, std::string* filename_stem_out)
{
    if (chapter_id_out == nullptr || filename_stem_out == nullptr)
        return false;
    // Expected: https://host/media/photos/<chapter_id>/<name>.jpg
    const std::string marker = "/media/photos/";
    const auto pos = image_url.find(marker);
    if (pos == std::string::npos)
        return false;
    std::string rest = image_url.substr(pos + marker.size());
    const auto slash = rest.find('/');
    if (slash == std::string::npos || slash == 0)
        return false;
    const std::string chapter = rest.substr(0, slash);
    std::string name = rest.substr(slash + 1);
    const auto q = name.find('?');
    if (q != std::string::npos)
        name = name.substr(0, q);
    if (chapter.empty() || name.empty())
        return false;
    char* end = nullptr;
    const long long chapter_id = std::strtoll(chapter.c_str(), &end, 10);
    if (end == chapter.c_str() || chapter_id <= 0)
        return false;
    // strip extension
    const auto dot = name.find_last_of('.');
    if (dot != std::string::npos)
        name = name.substr(0, dot);
    if (name.empty())
        return false;
    *chapter_id_out = chapter_id;
    *filename_stem_out = std::move(name);
    return true;
}

bool IsJmPhotoUrl(const std::string& image_url)
{
    return image_url.find("/media/photos/") != std::string::npos;
}

bool DecodeImageToRgb(const std::string& body,
                      int* width_out,
                      int* height_out,
                      std::vector<unsigned char>* rgb_out,
                      std::string* error)
{
    if (width_out == nullptr || height_out == nullptr || rgb_out == nullptr)
    {
        SetError(error, "JM image decode outputs are null");
        return false;
    }
    if (body.empty())
    {
        SetError(error, "JM image body is empty");
        return false;
    }

    tjhandle handle = tjInitDecompress();
    if (handle == nullptr)
    {
        SetError(error, "turbojpeg init failed");
        return false;
    }

    int width = 0;
    int height = 0;
    int subsamp = 0;
    int colorspace = 0;
    const auto* data = reinterpret_cast<const unsigned char*>(body.data());
    if (tjDecompressHeader3(handle,
                            data,
                            static_cast<unsigned long>(body.size()),
                            &width,
                            &height,
                            &subsamp,
                            &colorspace) != 0)
    {
        const char* msg = tjGetErrorStr2(handle);
        SetError(error, msg != nullptr ? msg : "jpeg header parse failed");
        tjDestroy(handle);
        return false;
    }
    if (width <= 0 || height <= 0)
    {
        SetError(error, "invalid jpeg dimensions");
        tjDestroy(handle);
        return false;
    }

    std::vector<unsigned char> rgb(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u);
    if (tjDecompress2(handle,
                      data,
                      static_cast<unsigned long>(body.size()),
                      rgb.data(),
                      width,
                      0,
                      height,
                      TJPF_RGB,
                      TJFLAG_FASTDCT) != 0)
    {
        const char* msg = tjGetErrorStr2(handle);
        SetError(error, msg != nullptr ? msg : "jpeg decompress failed");
        tjDestroy(handle);
        return false;
    }
    tjDestroy(handle);
    *width_out = width;
    *height_out = height;
    *rgb_out = std::move(rgb);
    return true;
}

bool EncodeRgbToJpeg(const std::vector<unsigned char>& rgb,
                     int width,
                     int height,
                     std::string* jpeg_out,
                     std::string* error)
{
    if (jpeg_out == nullptr)
    {
        SetError(error, "jpeg output is null");
        return false;
    }
    if (width <= 0 || height <= 0 ||
        rgb.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u)
    {
        SetError(error, "invalid rgb buffer");
        return false;
    }
    tjhandle handle = tjInitCompress();
    if (handle == nullptr)
    {
        SetError(error, "turbojpeg compress init failed");
        return false;
    }
    unsigned char* jpeg_buf = nullptr;
    unsigned long jpeg_size = 0;
    if (tjCompress2(handle,
                    rgb.data(),
                    width,
                    0,
                    height,
                    TJPF_RGB,
                    &jpeg_buf,
                    &jpeg_size,
                    TJSAMP_420,
                    90,
                    TJFLAG_FASTDCT) != 0)
    {
        const char* msg = tjGetErrorStr2(handle);
        SetError(error, msg != nullptr ? msg : "jpeg compress failed");
        tjDestroy(handle);
        return false;
    }
    jpeg_out->assign(reinterpret_cast<char*>(jpeg_buf), reinterpret_cast<char*>(jpeg_buf) + jpeg_size);
    tjFree(jpeg_buf);
    tjDestroy(handle);
    return true;
}

bool UnscrambleJmRgb(const std::vector<unsigned char>& src_rgb,
                     int width,
                     int height,
                     int strip_count,
                     std::vector<unsigned char>* dst_rgb,
                     std::string* error)
{
    if (dst_rgb == nullptr)
    {
        SetError(error, "unscramble output is null");
        return false;
    }
    if (strip_count <= 1)
    {
        *dst_rgb = src_rgb;
        return true;
    }
    if (width <= 0 || height <= 0 ||
        src_rgb.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u)
    {
        SetError(error, "invalid scramble source buffer");
        return false;
    }

    dst_rgb->assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u, 0);
    const std::size_t stride = static_cast<std::size_t>(width) * 3u;
    for (int i = 0; i < strip_count; ++i)
    {
        int strip_height = 0;
        const int y_src = jm_adapter::modules::ScrambleSourceY(height, strip_count, i, &strip_height);
        const int y_dst = jm_adapter::modules::ScrambleDestinationY(height, strip_count, i);
        if (strip_height <= 0 || y_src < 0 || y_dst < 0 || y_src + strip_height > height ||
            y_dst + strip_height > height)
        {
            SetError(error, "invalid scramble strip geometry");
            return false;
        }
        for (int row = 0; row < strip_height; ++row)
        {
            const unsigned char* src = src_rgb.data() + static_cast<std::size_t>(y_src + row) * stride;
            unsigned char* dst = dst_rgb->data() + static_cast<std::size_t>(y_dst + row) * stride;
            std::memcpy(dst, src, stride);
        }
    }
    return true;
}

bool MaybeUnscrambleJmImage(const std::string& image_url,
                            long long scramble_id,
                            R18ImagePayload* payload,
                            std::string* error)
{
    if (payload == nullptr)
        return false;
    if (!IsJmPhotoUrl(image_url))
        return true; // covers/albums stay as-is

    long long chapter_id = 0;
    std::string filename_stem;
    if (!ParseJmPhotoParts(image_url, &chapter_id, &filename_stem))
        return true; // not a parseable photo URL; leave untouched

    if (scramble_id <= 0)
        scramble_id = jm_adapter::modules::ScrambleIdThreshold();
    int strip_count = 0;
    if (chapter_id < scramble_id)
    {
        strip_count = 0;
    }
    else if (chapter_id < jm_adapter::modules::ScrambleFixedTenThreshold())
    {
        strip_count = 10;
    }
    else
    {
        std::string md5_hex;
        if (!detail::Md5Hex(std::to_string(chapter_id) + filename_stem, &md5_hex, error))
            return false;
        if (md5_hex.empty())
        {
            SetError(error, "md5 empty for scramble key");
            return false;
        }
        strip_count = jm_adapter::modules::ScrambleStripCount(chapter_id,
                                                              scramble_id,
                                                              static_cast<unsigned char>(md5_hex.back()));
    }
    if (strip_count <= 1)
        return true;

    // JM currently serves chapter photos as JPEG. Do not leak a still-scrambled
    // body if the upstream format changes unexpectedly.
    if (payload->body.size() < 3 || static_cast<unsigned char>(payload->body[0]) != 0xFF ||
        static_cast<unsigned char>(payload->body[1]) != 0xD8)
    {
        SetError(error, "JM scrambled photo is not JPEG");
        return false;
    }

    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgb;
    if (!DecodeImageToRgb(payload->body, &width, &height, &rgb, error))
        return false;

    std::vector<unsigned char> restored;
    if (!UnscrambleJmRgb(rgb, width, height, strip_count, &restored, error))
        return false;

    std::string jpeg;
    if (!EncodeRgbToJpeg(restored, width, height, &jpeg, error))
        return false;

    payload->body = std::move(jpeg);
    payload->content_type = jm_adapter::modules::DefaultImageContentType();
    return true;
}

} // namespace

R18ImagePayload
JmFetchImage(const std::filesystem::path& cache_root, const std::string& image_url, long long scramble_id)
{
    if (!IsAllowedJmImageUrl(image_url))
    {
        return detail::FailedImage(jm_adapter::modules::ImageUrlRejectedMessage());
    }

    std::string error;
    std::string cache_key;
    if (!detail::Md5Hex(image_url, &cache_key, &error))
    {
        return detail::FailedImage(error);
    }
    if (IsJmPhotoUrl(image_url))
    {
        if (scramble_id <= 0)
            scramble_id = jm_adapter::modules::ScrambleIdThreshold();
        cache_key += "-unscrambled-v1-" + std::to_string(scramble_id);
    }
    R18ImagePayload cached;
    if (detail::ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    JmImageFetchSlot slot;
    if (detail::ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    const std::vector<std::pair<std::string, std::string>> headers = {
        {"Accept", "image/jpeg,image/jpg,image/png;q=0.9,image/webp;q=0.5,*/*;q=0.1"},
        {"Accept-Language", "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7"},
        {"Referer", "https://localhost/"},
        {"Sec-Fetch-Dest", "image"},
        {"Sec-Fetch-Mode", "no-cors"},
        {"Sec-Fetch-Site", "cross-site"},
        {"Sec-Fetch-Storage-Access", "active"},
        {"User-Agent", jm_adapter::modules::UserAgent()},
        {"X-Requested-With", jm_adapter::modules::PackageName()},
    };
    detail::HttpResult result;
    if (!detail::HttpGet(image_url, headers, &result, &error, jm_adapter::modules::ImageTimeoutSeconds()))
    {
        return detail::PlaceholderImage("JMComic image timeout", error);
    }
    if (result.status < 200 || result.status >= 300 || result.body.empty())
    {
        return detail::PlaceholderImage("JMComic image unavailable", "HTTP " + std::to_string(result.status));
    }
    R18ImagePayload payload;
    payload.content_type = jm_adapter::modules::ShouldUseDefaultImageContentType(result.content_type.empty())
                               ? jm_adapter::modules::DefaultImageContentType()
                               : result.content_type;
    payload.body = std::move(result.body);

    // JM chapter photos are vertically scrambled; restore before caching/serving.
    std::string unscramble_error;
    if (!MaybeUnscrambleJmImage(image_url, scramble_id, &payload, &unscramble_error))
    {
        // Prefer a readable placeholder over serving scrambled strips.
        return detail::PlaceholderImage("JMComic image unscramble failed", unscramble_error);
    }

    detail::WriteCachedImage(cache_root, cache_key, payload);
    return payload;
}

bool JmLogin(const std::string& username, const std::string& password, std::string* uid_out, std::string* error)
{
    if (uid_out == nullptr)
    {
        SetError(error, "JM login uid output is null");
        return false;
    }
    if (username.empty() || password.empty())
    {
        SetError(error, jm_adapter::modules::LoginRequiredMessage());
        return false;
    }

    // Official JM login expects application/x-www-form-urlencoded fields, not JSON.
    // Wrong content-type yields: "用戶名和密碼字段不能留空！" even when values are present.
    const std::string body = std::string(jm_adapter::modules::LoginUsernameField()) + "=" +
                             detail::UrlEncode(username) + "&" + jm_adapter::modules::LoginPasswordField() + "=" +
                             detail::UrlEncode(password);

    const auto now = std::chrono::system_clock::now();
    const auto unix_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::vector<std::pair<std::string, std::string>> headers;
    if (!JmApiHeaders(unix_time, "", &headers, error))
        return false;
    headers.emplace_back("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");

    std::string last_error;
    for (int index = 0; index < jm_adapter::modules::ApiHostCount(); ++index)
    {
        const std::string host = jm_adapter::modules::ApiHostAt(index);
        if (host.empty())
            continue;
        const std::string url = "https://" + host + jm_adapter::modules::LoginTarget();
        HttpResult response;
        std::string request_error;
        if (!HttpPost(url, headers, body, &response, &request_error, jm_adapter::modules::LoginTimeoutSeconds()))
        {
            last_error = host + ": " + request_error;
            continue;
        }
        if (response.body.empty())
        {
            last_error = host + " HTTP " + std::to_string(response.status) + " empty body";
            continue;
        }

        // Login responses are plain JSON (not the encrypted `data` blob used by search/album).
        // Failed logins often return HTTP 401 with {"code":401,"errorMsg":"..."}.
        JsonValue root;
        if (!json::glaze_parse(root, response.body))
        {
            last_error = host + " returned non-JSON login response (HTTP " + std::to_string(response.status) + ")";
            continue;
        }

        const int64_t code = detail::FieldInt(root, "code", response.status);
        const std::string error_msg =
            detail::FieldString(root,
                                "errorMsg",
                                detail::FieldString(root, "message", detail::FieldString(root, "error", "")));

        // data may be object on success, empty array on failure, or rarely an encrypted string.
        JsonValue data = json::glaze_get(root, "data");
        std::string uid = detail::FieldString(data, jm_adapter::modules::LoginUidField());
        if (uid.empty())
            uid = detail::FieldString(root, jm_adapter::modules::LoginUidField());

        if (uid.empty() && data.isString())
        {
            // Fallback: some gateways still wrap login success in encrypted data.
            const std::string encrypted_data = detail::StringValue(data);
            std::string cipher_text;
            if (DecodeBase64(encrypted_data, cipher_text))
            {
                std::string key;
                std::string decrypt_error;
                if (Md5Hex(std::to_string(unix_time) + "185Hcomic3PAPP7R", &key, &decrypt_error))
                {
                    std::string decrypted;
                    if (Aes256EcbDecrypt(cipher_text, key, &decrypted, &decrypt_error))
                    {
                        std::string clear;
                        if (TrimJsonPayload(decrypted, &clear, &decrypt_error))
                        {
                            JsonValue decrypted_root;
                            if (json::glaze_parse(decrypted_root, clear))
                            {
                                uid = detail::FieldString(decrypted_root, jm_adapter::modules::LoginUidField());
                                if (uid.empty())
                                {
                                    const JsonValue nested = json::glaze_get(decrypted_root, "data");
                                    uid = detail::FieldString(nested, jm_adapter::modules::LoginUidField());
                                }
                            }
                        }
                    }
                }
            }
        }

        const bool ok_code = (code == 0 || code == 200);
        if (!uid.empty() && (ok_code || response.status == 200))
        {
            *uid_out = std::move(uid);
            return true;
        }

        if (!error_msg.empty())
        {
            // Prefer semantic upstream error (wrong password etc.) over host noise.
            last_error = jm_adapter::modules::LoginFailedMessage() + std::string(": ") + error_msg;
            // Auth failures are definitive — no need to try remaining hosts.
            if (code == 401 || response.status == 401)
            {
                SetError(error, last_error);
                return false;
            }
            continue;
        }

        last_error =
            host + " HTTP " + std::to_string(response.status) + " login rejected (code " + std::to_string(code) + ")";
    }

    SetError(error, last_error.empty() ? jm_adapter::modules::LoginFailedMessage() : last_error);
    return false;
}

bool JmCheckin(const std::string& uid, json::JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "JM check-in output pointer is null");
        return false;
    }
    if (uid.empty())
    {
        SetError(error, "JM check-in requires a logged-in uid");
        return false;
    }
    json::JsonValue result;
    if (!JmApiPost(jm_adapter::modules::CheckinTarget(), uid, "{}", &result, error))
        return false;

    const int64_t code = detail::FieldInt(result, "code", 200);
    const std::string msg = detail::FieldString(result, "message", "");
    json::JsonValue data;
    data["source_id"] = jm_adapter::modules::SourceId();
    data["uid"] = uid;
    data["message"] = msg;
    if (code == 200 || code == 0)
    {
        data["status"] = jm_adapter::modules::CheckinSuccessStatus();
        *out = std::move(data);
        return true;
    }
    // Code 401 or specific messages indicate already checked in or not logged in.
    const bool already = msg.find("已") != std::string::npos || msg.find("already") != std::string::npos ||
                         msg.find("签") != std::string::npos;
    if (already)
    {
        data["status"] = jm_adapter::modules::CheckinAlreadyDoneStatus();
        *out = std::move(data);
        return true;
    }
    SetError(error,
             msg.empty()
                 ? (jm_adapter::modules::CheckinFailedMessage() + std::string(" (code ") + std::to_string(code) + ")")
                 : (jm_adapter::modules::CheckinFailedMessage() + std::string(": ") + msg));
    return false;
}

} // namespace memochat::r18
