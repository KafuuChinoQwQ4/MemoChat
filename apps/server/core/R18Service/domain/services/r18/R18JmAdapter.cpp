#include "r18/R18JmAdapter.hpp"
#include "r18/R18AdapterUtils.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

import memochat.r18.jm_adapter_algorithms;

namespace memochat::r18
{

const char* const kJmSourceId = "jm.official";

namespace
{

using namespace detail;
using json::JsonValue;

std::mutex& JmImageFetchMutex()
{
    static std::mutex mu;
    return mu;
}

std::condition_variable& JmImageFetchCv()
{
    static std::condition_variable cv;
    return cv;
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
        {"User-Agent", jm_adapter::modules::UserAgent()},
        {"X-Requested-With", jm_adapter::modules::PackageName()},
        {"token", token},
        {"tokenparam", time + "," + jm_adapter::modules::Version()},
    };
}

std::string TrimJsonPayload(const std::string& value)
{
    const auto start = value.find_first_of("[{");
    const auto end = value.find_last_of("]}");
    if (!jm_adapter::modules::ShouldTrimJsonPayload(start == std::string::npos, end == std::string::npos, end < start))
        throw std::runtime_error(jm_adapter::modules::InvalidDecryptedPayloadMessage());
    return value.substr(start, end - start + 1);
}

JsonValue JmApiGet(const std::string& target)
{
    const auto now = std::chrono::system_clock::now();
    const auto unix_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::string last_error;
    for (int index = 0; index < jm_adapter::modules::ApiHostCount(); ++index)
    {
        const std::string host = jm_adapter::modules::ApiHostAt(index);
        try
        {
            const std::string url = "https://" + host + target;
            const HttpResult response = HttpGet(url, JmApiHeaders(unix_time), jm_adapter::modules::ApiTimeoutSeconds());
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
            const std::string key = Md5Hex(std::to_string(unix_time) + "185Hcomic3PAPP7R");
            const std::string clear = TrimJsonPayload(Aes256EcbDecrypt(cipher_text, key));
            JsonValue result;
            if (!json::glaze_parse(result, clear))
            {
                last_error = host + " decrypted payload parse failed";
                continue;
            }
            return result;
        }
        catch (const std::exception& exc)
        {
            last_error = host + ": " + exc.what();
        }
    }
    throw std::runtime_error(last_error.empty() ? jm_adapter::modules::OfficialApiFailureMessage() : last_error);
}

} // namespace

bool IsAllowedJmImageUrl(const std::string& image_url)
{
    try
    {
        ParsedUrl parsed = detail::ParseUrl(image_url);
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
        return jm_adapter::modules::IsAllowedImageUrl(
            parsed.scheme == "https",
            parsed.host == jm_adapter::modules::ImageHost(),
            parsed.target.rfind(jm_adapter::modules::ImageTargetPrefix(), 0) == 0);
    }
    catch (...)
    {
        return false;
    }
}

json::JsonValue JmSearch(const std::string& keyword, int page)
{
    const int normalized_page = jm_adapter::modules::NormalizeSearchPage(page);
    const std::string normalized_keyword = keyword.empty() ? "" : detail::UrlEncode(keyword);
    std::string target = "/search?search_query=" + normalized_keyword + "&o=mr";
    if (jm_adapter::modules::ShouldAppendSearchPage(normalized_page))
        target += "&page=" + std::to_string(normalized_page);
    const json::JsonValue result = JmApiGet(target);

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
    if (jm_adapter::modules::ShouldUseNextSearchPage(returned_count, jm_adapter::modules::SearchPageSize()))
    {
        data["max_page"] = static_cast<int64_t>(normalized_page + 1);
    }
    else
    {
        data["max_page"] = static_cast<int64_t>(normalized_page);
    }
    return data;
}

json::JsonValue JmDetail(const std::string& comic_id)
{
    std::string id = comic_id;
    if (jm_adapter::modules::ShouldStripJmPrefix(id.rfind("jm", 0) == 0))
        id = id.substr(2);
    const json::JsonValue result = JmApiGet("/album?id=" + detail::UrlEncode(id));

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
    if (const auto* arr = json::glaze_get_array(series))
    {
        for (const auto& entry : *arr)
            chapters.emplace_back(entry);
        std::sort(chapters.begin(),
                  chapters.end(),
                  [](const json::JsonValue& a, const json::JsonValue& b)
                  {
                      return detail::FieldInt(a, "sort", 0) < detail::FieldInt(b, "sort", 0);
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
    return data;
}

json::JsonValue JmPages(const std::string& chapter_id)
{
    const json::JsonValue result = JmApiGet("/chapter?id=" + detail::UrlEncode(chapter_id));
    json::JsonValue data;
    data["source_id"] = jm_adapter::modules::SourceId();
    data["chapter_id"] = chapter_id;
    data["pages"] = json::JsonValue{json::array_t{}};

    const json::JsonValue images = json::glaze_get(result, "images");
    int index = 1;
    if (const auto* arr = json::glaze_get_array(images))
    {
        for (const auto& entry : *arr)
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
    return data;
}

R18ImagePayload JmFetchImage(const std::filesystem::path& cache_root, const std::string& image_url)
{
    if (!IsAllowedJmImageUrl(image_url))
        throw std::runtime_error(jm_adapter::modules::ImageUrlRejectedMessage());

    const std::string cache_key = detail::Md5Hex(image_url);
    R18ImagePayload cached;
    if (detail::ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    JmImageFetchSlot slot;
    if (detail::ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    std::vector<std::pair<std::string, std::string>> headers = {
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
    try
    {
        detail::HttpResult result = detail::HttpGet(image_url, headers, jm_adapter::modules::ImageTimeoutSeconds());
        if (result.status < 200 || result.status >= 300 || result.body.empty())
            return detail::PlaceholderImage("JMComic image unavailable", "HTTP " + std::to_string(result.status));
        R18ImagePayload payload;
        payload.content_type = jm_adapter::modules::ShouldUseDefaultImageContentType(result.content_type.empty())
                                   ? jm_adapter::modules::DefaultImageContentType()
                                   : result.content_type;
        payload.body = std::move(result.body);
        detail::WriteCachedImage(cache_root, cache_key, payload);
        return payload;
    }
    catch (const std::exception& exc)
    {
        return detail::PlaceholderImage("JMComic image timeout", exc.what());
    }
}

} // namespace memochat::r18
