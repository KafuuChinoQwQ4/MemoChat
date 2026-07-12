#include "r18/R18JmAdapter.hpp"
#include "r18/R18AdapterUtils.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <utility>

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

bool JmApiHeaders(std::int64_t unix_time, std::vector<std::pair<std::string, std::string>>* out, std::string* error)
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
    *out = {
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
        {"User-Agent", jm_adapter::modules::UserAgent()},
        {"X-Requested-With", jm_adapter::modules::PackageName()},
        {"token", token},
        {"tokenparam", time + "," + jm_adapter::modules::Version()},
    };
    return true;
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

bool JmApiGet(const std::string& target, JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "JM API output pointer is null");
        return false;
    }
    const auto now = std::chrono::system_clock::now();
    const auto unix_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::vector<std::pair<std::string, std::string>> headers;
    if (!JmApiHeaders(unix_time, &headers, error))
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

bool JmSearch(const std::string& keyword, int page, json::JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "JM search output pointer is null");
        return false;
    }
    const int normalized_page = jm_adapter::modules::NormalizeSearchPage(page);
    const std::string normalized_keyword = keyword.empty() ? "" : detail::UrlEncode(keyword);
    std::string target = "/search?search_query=" + normalized_keyword + "&o=mr";
    if (jm_adapter::modules::ShouldAppendSearchPage(normalized_page))
        target += "&page=" + std::to_string(normalized_page);
    json::JsonValue result;
    if (!JmApiGet(target, &result, error))
    {
        return false;
    }

    json::JsonValue data;
    data["source_id"] = jm_adapter::modules::SourceId();
    data["keyword"] = keyword;
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
    if (out == nullptr)
    {
        SetError(error, "JM detail output pointer is null");
        return false;
    }
    std::string id = comic_id;
    if (jm_adapter::modules::ShouldStripJmPrefix(id.rfind("jm", 0) == 0))
        id = id.substr(2);
    json::JsonValue result;
    if (!JmApiGet("/album?id=" + detail::UrlEncode(id), &result, error))
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
    if (out == nullptr)
    {
        SetError(error, "JM pages output pointer is null");
        return false;
    }
    json::JsonValue result;
    if (!JmApiGet("/chapter?id=" + detail::UrlEncode(chapter_id), &result, error))
    {
        return false;
    }
    json::JsonValue data;
    data["source_id"] = jm_adapter::modules::SourceId();
    data["chapter_id"] = chapter_id;
    data["pages"] = json::JsonValue{json::array_t{}};

    const json::JsonValue images = json::glaze_get(result, "images");
    int index = 1;
    if (const auto* values = json::glaze_get_array(images))
    {
        for (const auto& entry : *values)
        {
            const std::string image_name = detail::StringValue(json::JsonValue(entry));
            json::JsonValue page;
            page["index"] = index;
            page["image_id"] = chapter_id + "-p" + std::to_string(index);
            page["url"] = detail::ImageProxyUrl(jm_adapter::modules::SourceId(), JmImageUrl(chapter_id, image_name));
            json::glaze_append(data["pages"], page);
            ++index;
        }
    }
    *out = std::move(data);
    return true;
}

R18ImagePayload JmFetchImage(const std::filesystem::path& cache_root, const std::string& image_url)
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
    R18ImagePayload cached;
    if (detail::ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    JmImageFetchSlot slot;
    if (detail::ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    const std::vector<std::pair<std::string, std::string>> headers = {
        {"Accept", "image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8"},
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
    detail::WriteCachedImage(cache_root, cache_key, payload);
    return payload;
}

} // namespace memochat::r18
