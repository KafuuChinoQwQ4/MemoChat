#include "r18/R18Hanime1Adapter.hpp"
#include "r18/R18AdapterUtils.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace memochat::r18
{

const char* const kHanime1SourceId = "hanime1.official";

namespace
{

using namespace detail;
using json::JsonValue;

void SetError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

constexpr const char* kBase = "https://hanime1.me";

std::vector<std::pair<std::string, std::string>> Hanime1Headers(const std::string& session_cookie = {})
{
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
        {"User-Agent",
         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
         "Chrome/124.0.0.0 Safari/537.36"},
        {"Accept-Language", "zh-TW,zh;q=0.9,en;q=0.8"},
        {"Referer", "https://hanime1.me/"},
    };
    if (!session_cookie.empty())
        headers.push_back({"Cookie", session_cookie});
    return headers;
}

// ── Exception-free string helpers ────────────────────────────────────────────

// Find substring case-insensitively (ASCII only).
static std::string::size_type
FindCI(const std::string& haystack, const std::string& needle, std::string::size_type from = 0)
{
    if (needle.empty())
        return std::string::npos;
    for (std::string::size_type i = from; i + needle.size() <= haystack.size(); ++i)
    {
        bool match = true;
        for (std::string::size_type j = 0; j < needle.size(); ++j)
        {
            if (std::tolower(static_cast<unsigned char>(haystack[i + j])) !=
                std::tolower(static_cast<unsigned char>(needle[j])))
            {
                match = false;
                break;
            }
        }
        if (match)
            return i;
    }
    return std::string::npos;
}

// Extract value of an HTML attribute: attr="VALUE".
static std::string ExtractAttr(const std::string& html, std::string::size_type search_from, const std::string& attr)
{
    const std::string needle_dq = attr + "=\"";
    const std::string needle_sq = attr + "='";
    auto pos = FindCI(html, needle_dq, search_from);
    char close = '"';
    if (pos == std::string::npos)
    {
        pos = FindCI(html, needle_sq, search_from);
        close = '\'';
    }
    if (pos == std::string::npos)
        return {};
    const std::string::size_type start = pos + attr.size() + 2;
    const std::string::size_type end = html.find(close, start);
    if (end == std::string::npos)
        return {};
    return html.substr(start, end - start);
}

// Decode basic HTML entities.
static std::string HtmlDecode(std::string s)
{
    struct
    {
        const char* ent;
        const char* ch;
    } kEnt[] = {
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
        {"&quot;", "\""},
        {"&#39;", "'"},
        {"&nbsp;", " "},
    };
    for (const auto& e : kEnt)
    {
        std::string::size_type pos = 0;
        while ((pos = s.find(e.ent, pos)) != std::string::npos)
        {
            s.replace(pos, std::strlen(e.ent), e.ch);
            pos += std::strlen(e.ch);
        }
    }
    return s;
}

// Trim ASCII whitespace.
static std::string Trim(std::string s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\n' || s.front() == '\r'))
        s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\n' || s.back() == '\r'))
        s.pop_back();
    return s;
}

// ── Video card parser ─────────────────────────────────────────────────────────

struct Hanime1VideoCard
{
    std::string id;
    std::string title;
    std::string thumbnail;
};

// Extract the numeric ID from "watch?v=12345" fragment.
static std::string ExtractWatchId(const std::string& href)
{
    const std::string marker = "watch?v=";
    const auto pos = href.find(marker);
    if (pos == std::string::npos)
        return {};
    const std::string::size_type start = pos + marker.size();
    std::string::size_type end = start;
    while (end < href.size() && std::isdigit(static_cast<unsigned char>(href[end])))
        ++end;
    return end > start ? href.substr(start, end - start) : std::string{};
}

// Extract numeric ID from thumbnail URL ".../thumbnail/12345l.jpg".
static std::string ExtractThumbId(const std::string& src)
{
    const std::string marker = "thumbnail/";
    const auto pos = src.find(marker);
    if (pos == std::string::npos)
        return {};
    const std::string::size_type start = pos + marker.size();
    std::string::size_type end = start;
    while (end < src.size() && std::isdigit(static_cast<unsigned char>(src[end])))
        ++end;
    return end > start ? src.substr(start, end - start) : std::string{};
}

// Walk the HTML and collect video cards.
// Anchors the parse on 'class="video-item-container"'.
std::vector<Hanime1VideoCard> ParseVideoCards(const std::string& html)
{
    std::vector<Hanime1VideoCard> cards;
    const std::string anchor = "video-item-container";
    std::string::size_type pos = 0;
    while (pos < html.size())
    {
        // Find the opening <div ... class="video-item-container"
        const auto div_start = html.find(anchor, pos);
        if (div_start == std::string::npos)
            break;

        // The closing </div> pair: we use a fixed lookahead window (~2 KB).
        const std::string::size_type window_end = std::min(html.size(), div_start + 2000);
        const std::string window = html.substr(div_start, window_end - div_start);

        // Title: sits in the opening <div title="TITLE" class="video-item-container">
        // Walk backwards to find the '<' that opens this div tag.
        std::string::size_type tag_open = div_start;
        while (tag_open > 0 && html[tag_open] != '<')
            --tag_open;
        const std::string tag_src = html.substr(tag_open, div_start + anchor.size() + 2 - tag_open);
        const std::string title_raw = ExtractAttr(tag_src, 0, "title");

        // Watch ID: first href containing watch?v= in the window.
        std::string watch_id;
        {
            const std::string href_marker = "href=\"";
            std::string::size_type p = 0;
            while (p < window.size())
            {
                const auto hpos = window.find(href_marker, p);
                if (hpos == std::string::npos)
                    break;
                const std::string::size_type vs = hpos + href_marker.size();
                const std::string::size_type ve = window.find('"', vs);
                if (ve == std::string::npos)
                    break;
                const std::string href = window.substr(vs, ve - vs);
                watch_id = ExtractWatchId(href);
                if (!watch_id.empty())
                    break;
                p = ve + 1;
            }
        }

        // Thumbnail: first src pointing to vdownload.hembed.com thumbnail — keep FULL URL
        // (including ?secure=token,ts which the CDN requires).
        std::string thumb_url;
        {
            const std::string src_marker = "src=\"";
            std::string::size_type p = 0;
            while (p < window.size())
            {
                const auto spos = window.find(src_marker, p);
                if (spos == std::string::npos)
                    break;
                const std::string::size_type vs = spos + src_marker.size();
                const std::string::size_type ve = window.find('"', vs);
                if (ve == std::string::npos)
                    break;
                const std::string src = window.substr(vs, ve - vs);
                if (src.find("hembed.com/image/thumbnail/") != std::string::npos)
                {
                    thumb_url = src;
                    break;
                }
                p = ve + 1;
            }
        }

        if (!watch_id.empty() && !thumb_url.empty())
        {
            Hanime1VideoCard c;
            c.id = watch_id;
            c.title = title_raw.empty() ? watch_id : HtmlDecode(Trim(title_raw));
            c.thumbnail = thumb_url; // full URL with ?secure= preserved
            cards.push_back(std::move(c));
        }

        pos = div_start + anchor.size();
    }
    return cards;
}

// Parse highest page number from "?page=N" links in pagination section.
int ParseLastPage(const std::string& html)
{
    int last = 1;
    const std::string marker = "?page=";
    std::string::size_type pos = 0;
    while (pos < html.size())
    {
        const auto p = html.find(marker, pos);
        if (p == std::string::npos)
            break;
        const std::string::size_type ns = p + marker.size();
        std::string::size_type ne = ns;
        while (ne < html.size() && std::isdigit(static_cast<unsigned char>(html[ne])))
            ++ne;
        if (ne > ns)
        {
            int n = 0;
            const auto [ptr, ec] = std::from_chars(html.data() + ns, html.data() + ne, n);
            if (ec == std::errc{} && ptr == html.data() + ne && n > last)
                last = n;
        }
        pos = ne;
    }
    return last;
}

// Map our sort id to hanime1's sort= query parameter.
std::string SortParam(const std::string& sort)
{
    if (sort == "最多觀看" || sort == "views")
        return "最多觀看";
    if (sort == "最高評分" || sort == "popular")
        return "最高評分";
    return "最新上傳";
}

bool Hanime1Get(const std::string& url, const std::string& cookie, std::string* body_out, std::string* error)
{
    HttpResult r;
    if (!HttpGet(url, Hanime1Headers(cookie), &r, error, 20))
        return false;
    if (r.status < 200 || r.status >= 300)
    {
        SetError(error, "hanime1 HTTP " + std::to_string(r.status));
        return false;
    }
    if (body_out)
        *body_out = std::move(r.body);
    return true;
}

JsonValue CardToItem(const Hanime1VideoCard& c)
{
    JsonValue item;
    item["source_id"] = kHanime1SourceId;
    item["comic_id"] = c.id;
    item["title"] = c.title;
    item["subtitle"] = "";
    item["cover"] = ImageProxyUrl(kHanime1SourceId, c.thumbnail);
    item["author"] = "";
    item["tags"] = JsonValue{json::array_t{}};
    return item;
}

// Extract first value from a simple tag: <TAG>VALUE</TAG>
// or attribute: attr="VALUE"
std::string
ExtractMeta(const std::string& html, const std::string& name_attr, const std::string& content_attr = "content")
{
    // <meta name="..." content="...">
    const std::string marker = "name=\"" + name_attr + "\"";
    const auto pos = html.find(marker);
    if (pos == std::string::npos)
        return {};
    // Find the containing <meta> tag (walk backwards to '<')
    std::string::size_type tag_start = pos;
    while (tag_start > 0 && html[tag_start] != '<')
        --tag_start;
    const std::string::size_type tag_end = html.find('>', pos);
    if (tag_end == std::string::npos)
        return {};
    const std::string tag = html.substr(tag_start, tag_end - tag_start);
    return HtmlDecode(ExtractAttr(tag, 0, content_attr));
}

bool ParseSignedExpiry(const std::string& query, int64_t now_ms, int64_t* expires_at_ms)
{
    bool found_secure = false;
    int64_t parsed_expiry_ms = 0;
    std::string::size_type begin = 0;
    while (begin <= query.size())
    {
        const auto end = query.find('&', begin);
        const std::string_view part(query.data() + begin, (end == std::string::npos ? query.size() : end) - begin);
        const auto equals = part.find('=');
        if (equals != std::string_view::npos && part.substr(0, equals) == "secure")
        {
            if (found_secure)
                return false;
            found_secure = true;

            const std::string_view value = part.substr(equals + 1);
            const auto comma = value.find(',');
            if (comma == std::string_view::npos || comma == 0 || comma + 1 >= value.size() ||
                value.find(',', comma + 1) != std::string_view::npos)
                return false;

            const std::string_view expiry_text = value.substr(comma + 1);
            int64_t expiry_seconds = 0;
            const auto [ptr, ec] =
                std::from_chars(expiry_text.data(), expiry_text.data() + expiry_text.size(), expiry_seconds);
            if (ec != std::errc{} || ptr != expiry_text.data() + expiry_text.size() || expiry_seconds <= 0 ||
                expiry_seconds > std::numeric_limits<int64_t>::max() / 1000)
                return false;

            parsed_expiry_ms = expiry_seconds * 1000;
            if (parsed_expiry_ms <= now_ms)
                return false;
        }

        if (end == std::string::npos)
            break;
        begin = end + 1;
    }

    if (!found_secure)
        return false;
    if (expires_at_ms != nullptr)
        *expires_at_ms = parsed_expiry_ms;
    return true;
}

bool ParseAllowedVideoSource(const std::string& url,
                             const std::string& video_id,
                             int64_t now_ms,
                             Hanime1VideoSource* out)
{
    if (out == nullptr || video_id.empty())
        return false;

    ParsedUrl parsed;
    std::string parse_error;
    if (!ParseUrl(url, &parsed, &parse_error) || parsed.scheme != "https" || parsed.host != "vdownload.hembed.com" ||
        parsed.port != "443" || parsed.has_userinfo || parsed.has_fragment)
        return false;

    const auto query_pos = parsed.target.find('?');
    if (query_pos == std::string::npos || query_pos + 1 >= parsed.target.size())
        return false;
    const std::string path = parsed.target.substr(0, query_pos);
    const std::string prefix = "/" + video_id + "-";
    constexpr std::string_view suffix = "p.mp4";
    if (!path.starts_with(prefix) || path.size() <= prefix.size() + suffix.size() || !path.ends_with(suffix))
        return false;

    const std::string_view quality_text(path.data() + prefix.size(), path.size() - prefix.size() - suffix.size());
    int quality = 0;
    const auto [quality_ptr, quality_ec] =
        std::from_chars(quality_text.data(), quality_text.data() + quality_text.size(), quality);
    if (quality_ec != std::errc{} || quality_ptr != quality_text.data() + quality_text.size() ||
        (quality != 480 && quality != 720 && quality != 1080) ||
        path != prefix + std::to_string(quality) + std::string(suffix))
        return false;

    int64_t expires_at_ms = 0;
    if (!ParseSignedExpiry(parsed.target.substr(query_pos + 1), now_ms, &expires_at_ms))
        return false;

    out->url = url;
    out->mime_type = "video/mp4";
    out->quality = quality;
    out->expires_at_ms = expires_at_ms;
    return true;
}

bool IsAllowedPosterUrl(const std::string& url)
{
    ParsedUrl parsed;
    std::string parse_error;
    if (!ParseUrl(url, &parsed, &parse_error) || parsed.scheme != "https" || parsed.host != "vdownload.hembed.com" ||
        parsed.port != "443" || parsed.has_userinfo || parsed.has_fragment)
        return false;
    const auto query_pos = parsed.target.find('?');
    const std::string path = parsed.target.substr(0, query_pos);
    return path.starts_with("/image/thumbnail/");
}

std::string ExtractPosterUrl(const std::string& html)
{
    std::string::size_type pos = 0;
    while ((pos = FindCI(html, "<video", pos)) != std::string::npos)
    {
        const auto tag_end = html.find('>', pos);
        if (tag_end == std::string::npos)
            break;
        const std::string poster = HtmlDecode(ExtractAttr(html.substr(pos, tag_end - pos + 1), 0, "poster"));
        if (IsAllowedPosterUrl(poster))
            return poster;
        pos = tag_end + 1;
    }

    pos = 0;
    while ((pos = FindCI(html, "src=", pos)) != std::string::npos)
    {
        const std::string candidate = HtmlDecode(ExtractAttr(html, pos, "src"));
        if (IsAllowedPosterUrl(candidate))
            return candidate;
        pos += 4;
    }
    return {};
}

} // namespace

// ── Public API ────────────────────────────────────────────────────────────────

int ParseHanime1LastPage(const std::string& html)
{
    return ParseLastPage(html);
}

std::vector<Hanime1VideoSource>
ParseHanime1VideoSources(const std::string& html, const std::string& video_id, int64_t now_ms)
{
    std::vector<Hanime1VideoSource> sources;
    if (video_id.empty())
        return sources;

    std::string::size_type pos = 0;
    while ((pos = FindCI(html, "<source", pos)) != std::string::npos)
    {
        const auto tag_end = html.find('>', pos);
        if (tag_end == std::string::npos)
            break;
        const std::string tag = html.substr(pos, tag_end - pos + 1);
        const std::string url = HtmlDecode(ExtractAttr(tag, 0, "src"));
        Hanime1VideoSource candidate;
        if (ParseAllowedVideoSource(url, video_id, now_ms, &candidate))
        {
            const auto duplicate = std::find_if(sources.begin(),
                                                sources.end(),
                                                [&](const Hanime1VideoSource& source)
                                                {
                                                    return source.quality == candidate.quality;
                                                });
            if (duplicate == sources.end())
                sources.push_back(std::move(candidate));
            else if (candidate.expires_at_ms > duplicate->expires_at_ms)
                *duplicate = std::move(candidate);
        }
        pos = tag_end + 1;
    }

    std::sort(sources.begin(),
              sources.end(),
              [](const Hanime1VideoSource& lhs, const Hanime1VideoSource& rhs)
              {
                  return lhs.quality > rhs.quality;
              });
    return sources;
}

bool Hanime1Search(const std::string& keyword,
                   int page,
                   const std::string& sort,
                   const std::string& tag,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error)
{
    if (!out)
    {
        SetError(error, "output pointer is null");
        return false;
    }

    std::string url = std::string(kBase) + "/search?page=" + std::to_string(std::max(1, page));
    if (!keyword.empty())
        url += "&query=" + UrlEncode(keyword);
    if (!tag.empty())
        url += "&genre=" + UrlEncode(tag);
    url += "&sort=" + UrlEncode(SortParam(sort));

    std::string body;
    if (!Hanime1Get(url, session_cookie, &body, error))
        return false;

    const auto cards = ParseVideoCards(body);
    const int max_page = std::max(1, ParseHanime1LastPage(body));

    JsonValue data;
    data["source_id"] = kHanime1SourceId;
    data["keyword"] = keyword;
    data["sort"] = sort;
    data["tag"] = tag;
    data["page"] = page;
    data["max_page"] = max_page;

    JsonValue items{json::array_t{}};
    for (const auto& c : cards)
        json::glaze_append(items, CardToItem(c));
    data["items"] = items;
    *out = data;
    return true;
}

bool Hanime1Detail(const std::string& comic_id,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error)
{
    if (!out)
    {
        SetError(error, "output pointer is null");
        return false;
    }

    const std::string url = std::string(kBase) + "/watch?v=" + UrlEncode(comic_id);
    std::string body;
    if (!Hanime1Get(url, session_cookie, &body, error))
        return false;

    // Title from <title>...</title>
    std::string title;
    {
        const auto ts = body.find("<title>");
        const auto te = body.find("</title>", ts != std::string::npos ? ts : 0);
        if (ts != std::string::npos && te != std::string::npos)
            title = HtmlDecode(Trim(body.substr(ts + 7, te - ts - 7)));
        // Strip " | Hanime1.me" suffix if present
        const auto sep = title.rfind(" | ");
        if (sep != std::string::npos)
            title = title.substr(0, sep);
    }

    // Thumbnail — extract full src URL (with ?secure=...) from the watch page.
    std::string thumb_url;
    {
        const std::string src_marker = "src=\"";
        std::string::size_type p = 0;
        while (p < body.size())
        {
            const auto spos = body.find(src_marker, p);
            if (spos == std::string::npos)
                break;
            const std::string::size_type vs = spos + src_marker.size();
            const std::string::size_type ve = body.find('"', vs);
            if (ve == std::string::npos)
                break;
            const std::string src = body.substr(vs, ve - vs);
            if (src.find("hembed.com/image/thumbnail/") != std::string::npos)
            {
                thumb_url = src;
                break;
            }
            p = ve + 1;
        }
    }

    // Description
    const std::string description = ExtractMeta(body, "description");

    JsonValue data;
    data["source_id"] = kHanime1SourceId;
    data["comic_id"] = comic_id;
    data["title"] = title.empty() ? comic_id : title;
    data["description"] = description;
    data["cover"] = thumb_url.empty() ? "" : ImageProxyUrl(kHanime1SourceId, thumb_url);
    data["author"] = "";
    data["tags"] = JsonValue{json::array_t{}};

    JsonValue chapters{json::array_t{}};
    JsonValue ch;
    ch["chapter_id"] = comic_id;
    ch["title"] = "完整視頻";
    ch["index"] = static_cast<int64_t>(0);
    ch["order"] = static_cast<int64_t>(1);
    json::glaze_append(chapters, ch);
    data["chapters"] = chapters;
    *out = data;
    return true;
}

bool Hanime1Pages(const std::string& chapter_id,
                  const std::string& session_cookie,
                  json::JsonValue* out,
                  std::string* error)
{
    if (!out)
    {
        SetError(error, "output pointer is null");
        return false;
    }

    const std::string url = std::string(kBase) + "/watch?v=" + UrlEncode(chapter_id);
    std::string body;
    if (!Hanime1Get(url, session_cookie, &body, error))
        return false;

    // Extract full thumbnail URL with ?secure= preserved.
    std::string cover_url;
    {
        const std::string src_marker = "src=\"";
        std::string::size_type p = 0;
        while (p < body.size())
        {
            const auto spos = body.find(src_marker, p);
            if (spos == std::string::npos)
                break;
            const std::string::size_type vs = spos + src_marker.size();
            const std::string::size_type ve = body.find('"', vs);
            if (ve == std::string::npos)
                break;
            const std::string src = body.substr(vs, ve - vs);
            if (src.find("hembed.com/image/thumbnail/") != std::string::npos)
            {
                cover_url = src;
                break;
            }
            p = ve + 1;
        }
    }

    JsonValue data;
    data["source_id"] = kHanime1SourceId;
    data["chapter_id"] = chapter_id;

    JsonValue pages{json::array_t{}};
    if (!cover_url.empty())
    {
        JsonValue pg;
        pg["index"] = static_cast<int64_t>(0);
        pg["image_id"] = chapter_id + "_cover";
        pg["url"] = ImageProxyUrl(kHanime1SourceId, cover_url);
        json::glaze_append(pages, pg);
    }
    data["pages"] = pages;
    *out = data;
    return true;
}

bool Hanime1ResolveVideo(const std::string& chapter_id,
                         const std::string& session_cookie,
                         json::JsonValue* out,
                         std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "output pointer is null");
        return false;
    }
    if (chapter_id.empty())
    {
        SetError(error, "chapter_id is required");
        return false;
    }

    const std::string url = std::string(kBase) + "/watch?v=" + UrlEncode(chapter_id);
    std::string body;
    if (!Hanime1Get(url, session_cookie, &body, error))
        return false;

    const auto now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    const auto sources = ParseHanime1VideoSources(body, chapter_id, now_ms);
    if (sources.empty())
    {
        SetError(error, "hanime1 playback sources are unavailable");
        return false;
    }

    int64_t earliest_expiry_ms = std::numeric_limits<int64_t>::max();
    JsonValue source_values{json::array_t{}};
    for (const auto& source : sources)
    {
        earliest_expiry_ms = std::min(earliest_expiry_ms, source.expires_at_ms);
        JsonValue value;
        value["url"] = source.url;
        value["mime_type"] = source.mime_type;
        value["quality"] = source.quality;
        json::glaze_append(source_values, value);
    }

    const std::string poster_url = ExtractPosterUrl(body);
    JsonValue data;
    data["source_id"] = kHanime1SourceId;
    data["chapter_id"] = chapter_id;
    data["poster"] = poster_url.empty() ? "" : ImageProxyUrl(kHanime1SourceId, poster_url);
    data["expires_at_ms"] = earliest_expiry_ms;
    data["sources"] = source_values;
    *out = std::move(data);
    return true;
}

R18ImagePayload Hanime1FetchImage(const std::filesystem::path& cache_root,
                                  const std::string& image_url,
                                  const std::string& session_cookie)
{
    std::string err;
    ParsedUrl parsed;
    if (!ParseUrl(image_url, &parsed, &err) || parsed.scheme != "https" || parsed.host != "vdownload.hembed.com" ||
        parsed.target.rfind("/image/thumbnail/", 0) != 0)
        return FailedImage("hanime1 image URL is not allowed");
    std::string cache_key;
    if (!Md5Hex(image_url, &cache_key, &err))
        return FailedImage("hanime1 image error: " + err);
    R18ImagePayload cached;
    if (ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    HttpResult r;
    if (!HttpGetBounded(image_url, Hanime1Headers(session_cookie), MaxImageBytes(), &r, &err, 15))
        return FailedImage("hanime1 image error: " + err);
    if (r.status < 200 || r.status >= 300 || r.body.empty())
        return FailedImage("hanime1 image unavailable: HTTP " + std::to_string(r.status));
    if (!r.content_type.starts_with("image/"))
        return FailedImage("hanime1 upstream did not return an image");

    R18ImagePayload payload;
    payload.content_type = r.content_type.empty() ? "image/jpeg" : r.content_type;
    payload.body = std::move(r.body);
    WriteCachedImage(cache_root, cache_key, payload);
    return payload;
}

bool Hanime1Login(const std::string& /*username*/,
                  const std::string& /*password*/,
                  std::string* /*session_cookie_out*/,
                  std::string* error)
{
    SetError(error,
             "hanime1.me 受 Cloudflare 保护，服务端无法直接登录。"
             "请切换到「Cookie 登录」选项卡：在浏览器中登录 hanime1.me，"
             "按 F12 → Application → Cookies → hanime1.me，"
             "复制 remember_token 的值粘贴进去。");
    return false;
}

} // namespace memochat::r18
