#include "r18/R18HanimeoneAdapter.hpp"
#include "r18/R18AdapterUtils.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace memochat::r18
{

const char* const kHanimeoneSourceId = "hanimeone.official";

namespace
{

using namespace detail;
using json::JsonValue;

void SetError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

constexpr const char* kBase = "https://hanimeone.me";
constexpr int kPageChunkSize = 20;
constexpr int kMaxPageNumber = 1'000'000;
constexpr int kMaxTotalPages = 5'000;

bool ParseBoundedPositiveInt(std::string_view text, int maximum, int* value)
{
    if (value == nullptr || text.empty() || maximum <= 0)
        return false;
    int parsed = 0;
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (ec != std::errc{} || ptr != text.data() + text.size() || parsed <= 0 || parsed > maximum)
        return false;
    *value = parsed;
    return true;
}

bool ParsePageNumber(std::string_view text, int* value)
{
    return ParseBoundedPositiveInt(text, kMaxPageNumber, value);
}

bool ParseTotalPageCount(std::string_view text, int* value)
{
    return ParseBoundedPositiveInt(text, kMaxTotalPages, value);
}

bool IsAllowedImageBase(const std::string& image_base)
{
    std::string error;
    ParsedUrl parsed;
    if (!ParseUrl(image_base, &parsed, &error))
        return false;
    const bool allowed_host = parsed.host == "qy0.ru" || parsed.host.ends_with(".qy0.ru");
    return parsed.scheme == "https" && allowed_host && parsed.target.rfind("/data/", 0) == 0;
}

std::vector<std::pair<std::string, std::string>> HanimeoneHeaders(const std::string& cookie = {})
{
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
        {"User-Agent",
         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
         "Chrome/124.0.0.0 Safari/537.36"},
        {"Accept-Language", "zh-TW,zh;q=0.9,en;q=0.8"},
        {"Referer", "https://hanimeone.me/"},
    };
    if (!cookie.empty())
        headers.push_back({"Cookie", cookie});
    return headers;
}

// ── String helpers ────────────────────────────────────────────────────────────

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

static std::string Trim(std::string s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\n' || s.front() == '\r'))
        s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\n' || s.back() == '\r'))
        s.pop_back();
    return s;
}

// Extract numeric comic ID from "/comic/12345" href.
static std::string ExtractComicId(const std::string& href)
{
    const std::string marker = "/comic/";
    const auto pos = href.find(marker);
    if (pos == std::string::npos)
        return {};
    const std::string::size_type start = pos + marker.size();
    std::string::size_type end = start;
    while (end < href.size() && std::isdigit(static_cast<unsigned char>(href[end])))
        ++end;
    return end > start ? href.substr(start, end - start) : std::string{};
}

// ── Comic card parser ─────────────────────────────────────────────────────────

struct HanimeoneCard
{
    std::string id;
    std::string title;
    std::string cover; // full URL (img*.qy0.ru — no secure token needed)
};

// Parses comic-rows-videos-div blocks:
//   <div class="comic-rows-videos-div" ...>
//     <a href="https://hanimeone.me/comic/ID">
//       <img data-srcset="COVER_URL">
//       <div class="comic-rows-videos-title">TITLE</div>
std::vector<HanimeoneCard> ParseComicCards(const std::string& html)
{
    std::vector<HanimeoneCard> cards;
    const std::string anchor = "comic-rows-videos-div";
    std::string::size_type pos = 0;
    while (pos < html.size())
    {
        const auto div_start = html.find(anchor, pos);
        if (div_start == std::string::npos)
            break;
        const std::string::size_type window_end = std::min(html.size(), div_start + 1200);
        const std::string window = html.substr(div_start, window_end - div_start);

        // href
        std::string comic_id;
        {
            const std::string hm = "href=\"";
            std::string::size_type p = 0;
            while (p < window.size())
            {
                const auto hp = window.find(hm, p);
                if (hp == std::string::npos)
                    break;
                const std::string::size_type vs = hp + hm.size();
                const std::string::size_type ve = window.find('"', vs);
                if (ve == std::string::npos)
                    break;
                comic_id = ExtractComicId(window.substr(vs, ve - vs));
                if (!comic_id.empty())
                    break;
                p = ve + 1;
            }
        }

        // cover from data-srcset (img*.qy0.ru URL)
        std::string cover;
        {
            const std::string dm = "data-srcset=\"";
            const auto dp = window.find(dm);
            if (dp != std::string::npos)
            {
                const std::string::size_type vs = dp + dm.size();
                const std::string::size_type ve = window.find('"', vs);
                if (ve != std::string::npos)
                    cover = window.substr(vs, ve - vs);
            }
        }

        // title from class="comic-rows-videos-title"
        std::string title;
        {
            const std::string tm = "comic-rows-videos-title\">";
            const auto tp = window.find(tm);
            if (tp != std::string::npos)
            {
                const std::string::size_type ts = tp + tm.size();
                const std::string::size_type te = window.find('<', ts);
                if (te != std::string::npos)
                    title = HtmlDecode(Trim(window.substr(ts, te - ts)));
            }
        }

        if (!comic_id.empty())
        {
            HanimeoneCard c;
            c.id = comic_id;
            c.title = title.empty() ? comic_id : title;
            c.cover = cover;
            cards.push_back(std::move(c));
        }
        pos = div_start + anchor.size();
    }
    return cards;
}

// Parse highest page number from "?page=N" links.
static int ParseLastPage(const std::string& html)
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
            if (ParseBoundedPositiveInt(std::string_view(html).substr(ns, ne - ns), 9'999, &n) && n > last)
                last = n; // guard against comic IDs leaking in
        }
        pos = ne;
    }
    return last;
}

// Use data-max/data-total attribute for accurate page count.
static int ParseTotalPagesFromDataAttr(const std::string& html)
{
    const std::string marker = "data-max=\"";
    const auto p = html.find(marker);
    if (p != std::string::npos)
    {
        const std::string::size_type ns = p + marker.size();
        std::string::size_type ne = ns;
        while (ne < html.size() && std::isdigit(static_cast<unsigned char>(html[ne])))
            ++ne;
        if (ne > ns)
        {
            int total = 0;
            if (ParseTotalPageCount(std::string_view(html).substr(ns, ne - ns), &total))
                return total;
        }
    }
    const std::string marker2 = "data-total=\"";
    const auto p2 = html.find(marker2);
    if (p2 != std::string::npos)
    {
        const std::string::size_type ns = p2 + marker2.size();
        std::string::size_type ne = ns;
        while (ne < html.size() && std::isdigit(static_cast<unsigned char>(html[ne])))
            ++ne;
        if (ne > ns)
        {
            int total = 0;
            if (ParseTotalPageCount(std::string_view(html).substr(ns, ne - ns), &total))
                return total;
        }
    }
    return 0; // fallback to link-based parsing
}

// Find max page number from /comic/{comic_id}/{page_num} links.
static int ParseTotalPages(const std::string& html, const std::string& comic_id)
{
    int last = 1;
    const std::string marker = "/comic/" + comic_id + "/";
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
            if (ParseTotalPageCount(std::string_view(html).substr(ns, ne - ns), &n) && n > last)
                last = n;
        }
        pos = ne;
    }
    return last;
}

// Extract cover URL from detail page.
// Pattern: <img ... src="https://img*.qy0.ru/data/.../000.jpg" ...>
static std::string ExtractCoverUrl(const std::string& html)
{
    const std::string cdn = "img";
    const std::string cdn_suffix = ".qy0.ru/data/";
    std::string::size_type pos = 0;
    while (pos < html.size())
    {
        const auto p = html.find(cdn_suffix, pos);
        if (p == std::string::npos)
            break;
        // Walk backwards to find the opening quote
        std::string::size_type q = p;
        while (q > 0 && html[q] != '"' && html[q] != '\'')
            --q;
        if (q > 0 && (html[q] == '"' || html[q] == '\''))
        {
            const std::string::size_type end = html.find(html[q], q + 1);
            if (end != std::string::npos)
            {
                const std::string url = html.substr(q + 1, end - q - 1);
                if (url.find("qy0.ru") != std::string::npos)
                    return url;
            }
        }
        pos = p + cdn_suffix.size();
    }
    return {};
}

// Derive image URL for page N (1-indexed) from the cover base URL.
// Cover: https://img4.qy0.ru/data/1515/62/000.jpg
// Page 3: https://img4.qy0.ru/data/1515/62/002.jpg  (0-indexed)
static std::string PageUrl(const std::string& cover_url, int page_1indexed)
{
    const auto slash = cover_url.rfind('/');
    if (slash == std::string::npos)
        return cover_url;
    const std::string base = cover_url.substr(0, slash + 1);
    const int idx = page_1indexed - 1; // 0-indexed
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%03d.jpg", idx);
    return base + buf;
}

bool HanimeoneGet(const std::string& url, const std::string& cookie, std::string* body_out, std::string* error)
{
    HttpResult r;
    if (!HttpGet(url, HanimeoneHeaders(cookie), &r, error, 20))
        return false;
    if (r.status < 200 || r.status >= 300)
    {
        SetError(error, "hanimeone HTTP " + std::to_string(r.status));
        return false;
    }
    if (body_out)
        *body_out = std::move(r.body);
    return true;
}

} // namespace

// ── Public API ────────────────────────────────────────────────────────────────

bool HanimeoneSearch(const std::string& keyword,
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

    std::string url = std::string(kBase) + "/comics?page=" + std::to_string(std::max(1, page));
    if (!keyword.empty())
        url += "&query=" + UrlEncode(keyword);
    if (!tag.empty())
        url += "&tag=" + UrlEncode(tag);
    if (!sort.empty())
        url += "&sort=" + UrlEncode(sort);

    std::string body;
    if (!HanimeoneGet(url, session_cookie, &body, error))
        return false;

    const auto cards = ParseComicCards(body);
    const int max_page = std::max(1, ParseLastPage(body));

    JsonValue data;
    data["source_id"] = kHanimeoneSourceId;
    data["keyword"] = keyword;
    data["sort"] = sort;
    data["tag"] = tag;
    data["page"] = page;
    data["max_page"] = max_page;

    JsonValue items{json::array_t{}};
    for (const auto& c : cards)
    {
        JsonValue item;
        item["source_id"] = kHanimeoneSourceId;
        item["comic_id"] = c.id;
        item["title"] = c.title;
        item["subtitle"] = "";
        item["cover"] = c.cover.empty() ? "" : ImageProxyUrl(kHanimeoneSourceId, c.cover);
        item["author"] = "";
        item["tags"] = JsonValue{json::array_t{}};
        json::glaze_append(items, item);
    }
    data["items"] = items;
    *out = data;
    return true;
}

bool HanimeoneDetail(const std::string& comic_id,
                     const std::string& session_cookie,
                     json::JsonValue* out,
                     std::string* error)
{
    if (!out)
    {
        SetError(error, "output pointer is null");
        return false;
    }

    const std::string url = std::string(kBase) + "/comic/" + UrlEncode(comic_id);
    std::string body;
    if (!HanimeoneGet(url, session_cookie, &body, error))
        return false;

    // Title
    std::string title;
    {
        const auto ts = body.find("<title>");
        const auto te = body.find("</title>", ts != std::string::npos ? ts : 0);
        if (ts != std::string::npos && te != std::string::npos)
            title = HtmlDecode(Trim(body.substr(ts + 7, te - ts - 7)));
        const auto sep = title.rfind(" - ");
        if (sep != std::string::npos)
            title = title.substr(0, sep);
    }

    const std::string cover_url = ExtractCoverUrl(body);
    int total_pages = ParseTotalPagesFromDataAttr(body);
    if (total_pages <= 0)
        total_pages = std::max(1, ParseTotalPages(body, comic_id));
    total_pages = std::clamp(total_pages, 1, kMaxTotalPages);

    // Extract the image base directory from cover URL:
    //   https://img4.qy0.ru/data/1515/62/000.jpg  →  https://img4.qy0.ru/data/1515/62
    std::string img_base;
    {
        const auto slash = cover_url.rfind('/');
        if (slash != std::string::npos)
            img_base = cover_url.substr(0, slash);
    }

    JsonValue data;
    data["source_id"] = kHanimeoneSourceId;
    data["comic_id"] = comic_id;
    data["title"] = title.empty() ? comic_id : title;
    data["description"] = "";
    data["cover"] = cover_url.empty() ? "" : ImageProxyUrl(kHanimeoneSourceId, cover_url);
    data["author"] = "";
    data["tags"] = JsonValue{json::array_t{}};

    // Split into chapters of 20 pages each.
    // chapter_id format: "{comic_id}|{img_base}|{start}|{end}"  (1-indexed, inclusive)
    JsonValue chapters{json::array_t{}};
    int chapter_idx = 0;
    for (int start = 1; start <= total_pages; start += kPageChunkSize)
    {
        const int end = std::min(start + kPageChunkSize - 1, total_pages);
        JsonValue ch;
        ch["chapter_id"] = comic_id + "|" + img_base + "|" + std::to_string(start) + "|" + std::to_string(end);
        ch["title"] = "第 " + std::to_string(start) + "–" + std::to_string(end) + " 頁";
        ch["index"] = static_cast<int64_t>(chapter_idx);
        ch["order"] = static_cast<int64_t>(chapter_idx + 1);
        json::glaze_append(chapters, ch);
        ++chapter_idx;
    }
    data["chapters"] = chapters;
    *out = data;
    return true;
}

bool HanimeonePages(const std::string& chapter_id,
                    const std::string& session_cookie,
                    json::JsonValue* out,
                    std::string* error)
{
    if (!out)
    {
        SetError(error, "output pointer is null");
        return false;
    }

    (void) session_cookie;
    // Current chapter_id format: "{comic_id}|{img_base}|{start}|{end}".
    const auto p1 = chapter_id.find('|');
    const auto p2 = p1 != std::string::npos ? chapter_id.find('|', p1 + 1) : std::string::npos;
    const auto p3 = p2 != std::string::npos ? chapter_id.find('|', p2 + 1) : std::string::npos;
    const auto p4 = p3 != std::string::npos ? chapter_id.find('|', p3 + 1) : std::string::npos;

    if (p1 == std::string::npos || p1 == 0 || p2 == std::string::npos || p2 == p1 + 1 || p3 == std::string::npos ||
        p3 == p2 + 1 || p3 + 1 >= chapter_id.size() || p4 != std::string::npos)
    {
        SetError(error, "invalid hanimeone chapter id");
        return false;
    }

    const std::string comic_id = chapter_id.substr(0, p1);
    const std::string img_base = chapter_id.substr(p1 + 1, p2 - p1 - 1);
    int start_page = 0;
    int end_page = 0;
    if (!ParsePageNumber(std::string_view(chapter_id).substr(p2 + 1, p3 - p2 - 1), &start_page) ||
        !ParsePageNumber(std::string_view(chapter_id).substr(p3 + 1), &end_page) || end_page < start_page ||
        end_page - start_page + 1 > kPageChunkSize)
    {
        SetError(error, "invalid hanimeone chapter range");
        return false;
    }
    if (!IsAllowedImageBase(img_base))
    {
        SetError(error, "hanimeone image base is not allowed");
        return false;
    }

    JsonValue data;
    data["source_id"] = kHanimeoneSourceId;
    data["chapter_id"] = chapter_id;

    JsonValue pages{json::array_t{}};
    for (int i = start_page; i <= end_page; ++i)
    {
        const int idx = i - 1;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%03d.jpg", idx);
        const std::string img_url = img_base + "/" + buf;
        JsonValue p;
        p["index"] = static_cast<int64_t>(i - start_page);
        p["image_id"] = comic_id + "_p" + std::to_string(i);
        p["url"] = ImageProxyUrl(kHanimeoneSourceId, img_url);
        json::glaze_append(pages, p);
    }
    data["pages"] = pages;
    *out = data;
    return true;
}

R18ImagePayload HanimeoneFootImage(const std::filesystem::path& cache_root,
                                   const std::string& image_url,
                                   const std::string& session_cookie)
{
    std::string err;
    ParsedUrl parsed;
    if (!ParseUrl(image_url, &parsed, &err))
        return FailedImage("hanimeone image URL is invalid: " + err);
    const bool allowed_host = parsed.host == "qy0.ru" || parsed.host.ends_with(".qy0.ru");
    if (parsed.scheme != "https" || !allowed_host || parsed.target.rfind("/data/", 0) != 0)
        return FailedImage("hanimeone image URL is not allowed");
    std::string cache_key;
    if (!Md5Hex(image_url, &cache_key, &err))
        return FailedImage("hanimeone image error: " + err);
    R18ImagePayload cached;
    if (ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    // img.qy0.ru CDN does not require secure tokens or special Referer.
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Accept", "image/avif,image/webp,image/*,*/*;q=0.8"},
        {"User-Agent", "Mozilla/5.0 (compatible; MemoChatR18/1.0)"},
        {"Referer", "https://hanimeone.me/"},
    };
    if (!session_cookie.empty())
        headers.push_back({"Cookie", session_cookie});

    HttpResult r;
    if (!HttpGetBounded(image_url, headers, MaxImageBytes(), &r, &err, 15))
        return FailedImage("hanimeone image error: " + err);
    if (r.status < 200 || r.status >= 300 || r.body.empty())
        return FailedImage("hanimeone image unavailable: HTTP " + std::to_string(r.status));
    if (!r.content_type.starts_with("image/"))
        return FailedImage("hanimeone upstream did not return an image");

    R18ImagePayload payload;
    payload.content_type = r.content_type.empty() ? "image/jpeg" : r.content_type;
    payload.body = std::move(r.body);
    WriteCachedImage(cache_root, cache_key, payload);
    return payload;
}

} // namespace memochat::r18
