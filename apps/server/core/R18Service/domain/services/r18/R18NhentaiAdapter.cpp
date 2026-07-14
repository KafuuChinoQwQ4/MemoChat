#include "r18/R18NhentaiAdapter.hpp"
#include "r18/R18AdapterUtils.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace memochat::r18
{

const char* const kNhentaiSourceId = "nhentai.official";

namespace
{

using namespace detail;
using json::JsonValue;

void SetError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

std::string ToStringId(const JsonValue& gallery_or_id)
{
    if (gallery_or_id.isObject())
        return FieldString(gallery_or_id, "id");
    if (gallery_or_id.isString())
        return gallery_or_id.asString();
    if (gallery_or_id.isNumber())
        return std::to_string(gallery_or_id.asInt64());
    return {};
}

std::string MediaPathUrl(const std::string& server, const std::string& path)
{
    if (path.empty())
        return {};
    if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0)
        return path;
    if (!path.empty() && path[0] == '/')
        return server + path;
    return server + "/" + path;
}

// Upstream detail occasionally emits double extensions like "cover.webp.webp".
// Collapse consecutive identical extensions and prefer a single trailing type.
std::string NormalizeNhentaiMediaPath(std::string path)
{
    if (path.empty())
        return path;

    static const char* kExts[] = {".webp", ".jpg", ".jpeg", ".png", ".gif", ".avif"};
    auto ends_with_ci = [](const std::string& value, const std::string& suffix)
    {
        if (value.size() < suffix.size())
            return false;
        for (std::size_t i = 0; i < suffix.size(); ++i)
        {
            const unsigned char a = static_cast<unsigned char>(value[value.size() - suffix.size() + i]);
            const unsigned char b = static_cast<unsigned char>(suffix[i]);
            if (std::tolower(a) != std::tolower(b))
                return false;
        }
        return true;
    };

    bool collapsed = true;
    while (collapsed)
    {
        collapsed = false;
        for (const char* ext_c : kExts)
        {
            const std::string ext(ext_c);
            const std::string doubled = ext + ext;
            if (ends_with_ci(path, doubled))
            {
                path.resize(path.size() - ext.size());
                collapsed = true;
                break;
            }
        }
    }
    return path;
}

std::string PreferNhentaiCoverPath(const JsonValue& gallery)
{
    const JsonValue cover = json::glaze_get(gallery, "cover");
    const JsonValue thumbnail = json::glaze_get(gallery, "thumbnail");
    std::string cover_path = FieldString(cover, "path");
    if (cover_path.empty() && cover.isString())
        cover_path = cover.asString();
    std::string thumb_path = FieldString(thumbnail, "path");
    if (thumb_path.empty())
    {
        // list payload stores thumbnail as plain relative path string
        if (thumbnail.isString())
            thumb_path = thumbnail.asString();
        else
            thumb_path = FieldString(gallery, "thumbnail");
    }

    cover_path = NormalizeNhentaiMediaPath(std::move(cover_path));
    thumb_path = NormalizeNhentaiMediaPath(std::move(thumb_path));

    // Prefer normalized cover; fall back to thumb when cover is empty.
    if (!cover_path.empty())
        return cover_path;
    return thumb_path;
}

std::string ThumbServer()
{
    return "https://t1.nhentai.net";
}

std::string ImageServer()
{
    return "https://i1.nhentai.net";
}

std::string JoinTagNames(const JsonValue& tags)
{
    std::vector<std::string> names;
    if (const auto* values = json::glaze_get_array(tags))
    {
        for (const auto& tag : *values)
        {
            const std::string name = FieldString(JsonValue(tag), "name");
            if (!name.empty())
                names.push_back(name);
            if (names.size() >= 8)
                break;
        }
    }
    std::string joined;
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        if (!joined.empty())
            joined += ", ";
        joined += names[i];
    }
    return joined;
}

JsonValue TagArray(const JsonValue& tags)
{
    std::vector<std::string> names;
    if (const auto* values = json::glaze_get_array(tags))
    {
        for (const auto& tag : *values)
        {
            const std::string name = FieldString(JsonValue(tag), "name");
            if (!name.empty())
                names.push_back(name);
            if (names.size() >= 8)
                break;
        }
    }
    return MakeTags(names);
}

std::vector<std::pair<std::string, std::string>> NhentaiBrowserHeaders()
{
    return {
        {"Accept", "application/json,text/html;q=0.9,*/*;q=0.8"},
        {"User-Agent",
         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 "
         "Safari/537.36"},
        {"Referer", "https://nhentai.net/"},
    };
}

bool NhentaiGetJson(const std::string& path, JsonValue* out, std::string* error)
{
    const std::string url = "https://nhentai.net" + path;
    HttpResult response;
    if (!HttpGet(url, NhentaiBrowserHeaders(), &response, error, 12))
        return false;
    if (response.status < 200 || response.status >= 300)
    {
        SetError(error, "nHentai HTTP " + std::to_string(response.status));
        return false;
    }
    if (!json::glaze_parse(*out, response.body))
    {
        SetError(error, "nHentai invalid JSON");
        return false;
    }
    return true;
}

JsonValue ComicItemFromList(const JsonValue& gallery)
{
    const std::string id = ToStringId(json::glaze_get(gallery, "id"));
    const std::string media_id = FieldString(gallery, "media_id");
    const std::string english = FieldString(gallery, "english_title", id);
    const std::string japanese = FieldString(gallery, "japanese_title");
    const std::string thumb_path = PreferNhentaiCoverPath(gallery);

    JsonValue item;
    item["source_id"] = kNhentaiSourceId;
    item["comic_id"] = id;
    item["title"] = english.empty() ? id : english;
    item["subtitle"] = japanese;
    item["description"] = "";
    item["cover"] = ImageProxyUrl(kNhentaiSourceId, MediaPathUrl(ThumbServer(), thumb_path));
    item["author"] = "";
    item["tags"] = MakeTags({});
    return item;
}

JsonValue ComicItemFromDetail(const JsonValue& gallery)
{
    const std::string id = ToStringId(json::glaze_get(gallery, "id"));
    const JsonValue title = json::glaze_get(gallery, "title");
    const std::string cover_path = PreferNhentaiCoverPath(gallery);
    const JsonValue tags = json::glaze_get(gallery, "tags");

    JsonValue item;
    item["source_id"] = kNhentaiSourceId;
    item["comic_id"] = id;
    item["title"] = FieldString(title, "pretty", FieldString(title, "english", id));
    item["subtitle"] = FieldString(title, "japanese");
    item["description"] = JoinTagNames(tags);
    item["cover"] = ImageProxyUrl(kNhentaiSourceId, MediaPathUrl(ThumbServer(), cover_path));
    item["author"] = "";
    item["tags"] = TagArray(tags);
    return item;
}

bool HostAllowed(const std::string& host)
{
    static const char* kAllowed[] = {
        "i.nhentai.net",
        "t.nhentai.net",
        "nhentai.net",
        "i1.nhentai.net",
        "i2.nhentai.net",
        "i3.nhentai.net",
        "i4.nhentai.net",
        "t1.nhentai.net",
        "t2.nhentai.net",
        "t3.nhentai.net",
        "t4.nhentai.net",
    };
    for (const char* allowed : kAllowed)
    {
        if (host == allowed)
            return true;
    }
    return false;
}

std::string NormalizeNhentaiSort(const std::string& sort)
{
    if (sort.empty() || sort == "date" || sort == "recent")
        return "";
    if (sort == "popular" || sort == "popular-all" || sort == "all")
        return "popular";
    if (sort == "popular-today" || sort == "today" || sort == "day")
        return "popular-today";
    if (sort == "popular-week" || sort == "week")
        return "popular-week";
    if (sort == "popular-month" || sort == "month")
        return "popular-month";
    return "";
}

} // namespace

bool NhentaiSearch(const std::string& keyword,
                   int page,
                   const std::string& sort,
                   const std::string& tag,
                   json::JsonValue* out,
                   std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "nHentai search output is null");
        return false;
    }
    const int normalized_page = page < 1 ? 1 : page;
    const std::string resolved_sort = NormalizeNhentaiSort(sort);
    // Tag filter becomes language:"x" / tag:"name"; free-text keyword is ANDed when present.
    auto FormatNhentaiTag = [](const std::string& raw) -> std::string
    {
        if (raw.empty())
            return {};
        if (raw.find(':') != std::string::npos)
            return raw;
        // Common language chips map better to language:"..." than generic tag:"...".
        if (raw == "chinese" || raw == "english" || raw == "japanese" || raw == "translated")
            return std::string("language:\"") + raw + "\"";
        return std::string("tag:\"") + raw + "\"";
    };
    std::string query = keyword;
    const std::string formatted_tag = FormatNhentaiTag(tag);
    if (!formatted_tag.empty())
    {
        if (query.empty())
            query = formatted_tag;
        else
            query = query + " " + formatted_tag;
    }
    // Empty query + no sort: browse latest via /api/v2/galleries.
    // Any sort or keyword/tag uses /api/v2/search.
    JsonValue root;
    if (query.empty() && resolved_sort.empty())
    {
        const std::string path = "/api/v2/galleries?page=" + std::to_string(normalized_page);
        if (!NhentaiGetJson(path, &root, error))
            return false;
    }
    else
    {
        std::string path = "/api/v2/search?query=" + UrlEncode(query.empty() ? "*" : query) +
                           "&page=" + std::to_string(normalized_page);
        if (!resolved_sort.empty())
            path += "&sort=" + UrlEncode(resolved_sort);
        if (!NhentaiGetJson(path, &root, error))
            return false;
    }

    JsonValue result;
    result["source_id"] = kNhentaiSourceId;
    result["keyword"] = keyword;
    result["sort"] = resolved_sort;
    result["tag"] = tag;
    result["page"] = normalized_page;
    result["max_page"] = FieldInt(root, "num_pages", 1);
    result["items"] = JsonValue{json::array_t{}};
    const JsonValue docs = json::glaze_get(root, "result");
    if (const auto* values = json::glaze_get_array(docs))
    {
        for (const auto& gallery : *values)
            json::glaze_append(result["items"], ComicItemFromList(JsonValue(gallery)));
    }
    *out = std::move(result);
    return true;
}

bool NhentaiDetail(const std::string& comic_id, json::JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "nHentai detail output is null");
        return false;
    }
    JsonValue gallery;
    if (!NhentaiGetJson("/api/v2/galleries/" + UrlEncode(comic_id), &gallery, error))
        return false;

    JsonValue result = ComicItemFromDetail(gallery);
    result["chapters"] = JsonValue{json::array_t{}};

    JsonValue chapter;
    chapter["source_id"] = kNhentaiSourceId;
    chapter["comic_id"] = comic_id;
    chapter["chapter_id"] = comic_id;
    chapter["title"] = "Gallery";
    chapter["order"] = 1;
    json::glaze_append(result["chapters"], chapter);
    *out = std::move(result);
    return true;
}

bool NhentaiPages(const std::string& chapter_id, json::JsonValue* out, std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "nHentai pages output is null");
        return false;
    }
    JsonValue gallery;
    if (!NhentaiGetJson("/api/v2/galleries/" + UrlEncode(chapter_id), &gallery, error))
        return false;

    JsonValue result;
    result["source_id"] = kNhentaiSourceId;
    result["chapter_id"] = chapter_id;
    result["pages"] = JsonValue{json::array_t{}};

    const JsonValue pages = json::glaze_get(gallery, "pages");
    if (const auto* values = json::glaze_get_array(pages))
    {
        for (const auto& page : *values)
        {
            const JsonValue page_obj(page);
            const int index = FieldInt(page_obj, "number", 0);
            const std::string path = NormalizeNhentaiMediaPath(FieldString(page_obj, "path"));
            if (index <= 0 || path.empty())
                continue;
            JsonValue page_value;
            page_value["index"] = index;
            page_value["image_id"] = chapter_id + "-p" + std::to_string(index);
            page_value["url"] = ImageProxyUrl(kNhentaiSourceId, MediaPathUrl(ImageServer(), path));
            json::glaze_append(result["pages"], page_value);
        }
    }
    *out = std::move(result);
    return true;
}

R18ImagePayload NhentaiFetchImage(const std::filesystem::path& cache_root, const std::string& image_url)
{
    std::string error;
    ParsedUrl parsed;
    if (!ParseUrl(image_url, &parsed, &error))
        return PlaceholderImage("nHentai image error", error);
    if (parsed.scheme != "https" || !HostAllowed(parsed.host))
        return PlaceholderImage("nHentai image error", "image host not allowed");

    std::string cache_key;
    if (!Md5Hex(image_url, &cache_key, &error))
        return PlaceholderImage("nHentai image error", error);
    R18ImagePayload cached;
    if (ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    const std::vector<std::pair<std::string, std::string>> headers = {
        {"Accept", "image/avif,image/webp,image/*,*/*;q=0.8"},
        {"User-Agent", "Mozilla/5.0 (compatible; MemoChatR18/1.0)"},
        {"Referer", "https://nhentai.net/"},
    };
    HttpResult response;
    if (!HttpGet(image_url, headers, &response, &error, 15))
        return PlaceholderImage("nHentai image error", error);
    if (response.status < 200 || response.status >= 300 || response.body.empty())
        return PlaceholderImage("nHentai image unavailable", "HTTP " + std::to_string(response.status));

    R18ImagePayload payload;
    payload.content_type = response.content_type.empty() ? "image/webp" : response.content_type;
    payload.body = std::move(response.body);
    WriteCachedImage(cache_root, cache_key, payload);
    return payload;
}

} // namespace memochat::r18
