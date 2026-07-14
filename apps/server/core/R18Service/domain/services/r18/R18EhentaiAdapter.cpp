#include "r18/R18EhentaiAdapter.hpp"
#include "r18/R18AdapterUtils.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace memochat::r18
{

const char* const kEhentaiSourceId = "ehentai.official";

namespace
{

using namespace detail;
using json::JsonValue;

void SetError(std::string* error, std::string message)
{
    if (error != nullptr)
        *error = std::move(message);
}

std::string HtmlUnescape(std::string value)
{
    auto replace_all = [&](const std::string& from, const std::string& to)
    {
        std::size_t pos = 0;
        while ((pos = value.find(from, pos)) != std::string::npos)
        {
            value.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replace_all("&amp;", "&");
    replace_all("&lt;", "<");
    replace_all("&gt;", ">");
    replace_all("&quot;", "\"");
    replace_all("&#039;", "'");
    replace_all("&apos;", "'");
    return value;
}

std::string StripTags(std::string value)
{
    static const std::regex tag_re("<[^>]*>");
    return HtmlUnescape(std::regex_replace(value, tag_re, ""));
}

bool LooksLikeGidToken(const std::string& comic_id, std::string* gid, std::string* token)
{
    const auto sep = comic_id.find('/');
    if (sep == std::string::npos || sep == 0 || sep + 1 >= comic_id.size())
        return false;
    *gid = comic_id.substr(0, sep);
    *token = comic_id.substr(sep + 1);
    return !gid->empty() && !token->empty();
}

std::vector<std::pair<std::string, std::string>> BaseHeaders(const std::string& session_cookie)
{
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Accept", "text/html,application/xhtml+xml,application/json;q=0.9,*/*;q=0.8"},
        {"User-Agent",
         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 "
         "Safari/537.36"},
        {"Referer", "https://e-hentai.org/"},
    };
    if (!session_cookie.empty())
        headers.emplace_back("Cookie", session_cookie);
    return headers;
}

bool FetchHtml(const std::string& url, const std::string& session_cookie, std::string* body, std::string* error)
{
    HttpResult response;
    if (!HttpGet(url, BaseHeaders(session_cookie), &response, error, 15))
        return false;
    if (response.status < 200 || response.status >= 300)
    {
        SetError(error, "e-hentai HTTP " + std::to_string(response.status));
        return false;
    }
    *body = std::move(response.body);
    return true;
}

JsonValue
MakeItem(const std::string& comic_id, const std::string& title, const std::string& cover, const std::string& category)
{
    JsonValue item;
    item["source_id"] = kEhentaiSourceId;
    item["comic_id"] = comic_id;
    item["title"] = title;
    item["subtitle"] = category;
    item["description"] = category;
    item["cover"] = ImageProxyUrl(kEhentaiSourceId, cover);
    item["author"] = "";
    item["tags"] = MakeTags(category.empty() ? std::vector<std::string>{} : std::vector<std::string>{category});
    return item;
}

std::string NormalizeEhentaiCats(const std::string& value)
{
    if (value.empty())
        return "";
    // Numeric bitmask passes through.
    bool all_digits = true;
    for (char ch : value)
    {
        if (ch < '0' || ch > '9')
        {
            all_digits = false;
            break;
        }
    }
    if (all_digits)
        return value;
    // Named categories map to e-hentai f_cats "disabled" mask (bit set = hidden).
    // Base full mask with all categories visible is 0.
    // Bits: 0 Doujinshi, 1 Manga, 2 Artist CG, 3 Game CG, 4 Western, 5 Non-H, 6 Image Set, 7 Cosplay, 8 Asian Porn, 9
    // Misc We invert: requested category keeps its bit off; others on.
    auto bit_for = [](const std::string& name) -> int
    {
        if (name == "doujinshi" || name == "Doujinshi")
            return 0;
        if (name == "manga" || name == "Manga")
            return 1;
        if (name == "artist_cg" || name == "Artist CG" || name == "artist-cg")
            return 2;
        if (name == "game_cg" || name == "Game CG" || name == "game-cg")
            return 3;
        if (name == "western" || name == "Western")
            return 4;
        if (name == "non-h" || name == "Non-H" || name == "non_h")
            return 5;
        if (name == "image_set" || name == "Image Set" || name == "imageset")
            return 6;
        if (name == "cosplay" || name == "Cosplay")
            return 7;
        if (name == "asian_porn" || name == "Asian Porn" || name == "asian-porn")
            return 8;
        if (name == "misc" || name == "Misc")
            return 9;
        return -1;
    };
    const int keep = bit_for(value);
    if (keep < 0)
        return "";
    int mask = 0;
    for (int bit = 0; bit < 10; ++bit)
    {
        if (bit != keep)
            mask |= (1 << bit);
    }
    return std::to_string(mask);
}

} // namespace

bool EhentaiSearch(const std::string& keyword,
                   int page,
                   const std::string& sort,
                   const std::string& tag,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "e-hentai search output is null");
        return false;
    }
    const int normalized_page = page < 1 ? 1 : page;
    // sort chips map to f_cats category masks; tag remains free-text / namespace:tag search.
    const std::string cats = NormalizeEhentaiCats(sort);
    std::string search = keyword;
    if (!tag.empty())
    {
        // Always append explicit tag filter; category chips do not consume the tag field.
        if (!search.empty())
            search += " ";
        search += tag;
    }
    std::ostringstream url;
    url << "https://e-hentai.org/?page=" << (normalized_page - 1);
    if (!search.empty())
        url << "&f_search=" << UrlEncode(search);
    if (!cats.empty())
        url << "&f_cats=" << cats;

    std::string html;
    if (!FetchHtml(url.str(), session_cookie, &html, error))
        return false;

    JsonValue result;
    result["source_id"] = kEhentaiSourceId;
    result["keyword"] = keyword;
    result["sort"] = sort;
    result["tag"] = tag;
    result["page"] = normalized_page;
    result["max_page"] = normalized_page + 1; // next page available until empty
    result["items"] = JsonValue{json::array_t{}};

    // Compact list markup uses <tr> rows with gl1c/glname; covers are now on ehgt.org
    // (often /w/...), not the legacy /t/ thumbnail path.
    static const std::regex row_re(R"re(<tr[^>]*>[\s\S]*?gl1c[\s\S]*?</tr>)re", std::regex::icase);
    static const std::regex link_re(R"re(href="https?://e-hentai\.org/g/(\d+)/([0-9a-f]+)/")re", std::regex::icase);
    static const std::regex title_re(R"re(class="glink"[^>]*>([\s\S]*?)</div>)re", std::regex::icase);
    // Prefer glthumb image; exclude site icons under ehgt.org/g/.
    static const std::regex cover_re(
        R"re(<div class="glthumb"[\s\S]*?<img[^>]+(?:data-src|src)="(https?://ehgt\.org/(?!g/)[^"]+)")re",
        std::regex::icase);
    static const std::regex cover_re_fallback(
        R"re((?:data-src|src)="(https?://ehgt\.org/(?!g/)[^"]+\.(?:webp|jpe?g|png|gif))")re",
        std::regex::icase);
    static const std::regex cover_re_legacy(R"re((?:data-src|src)="(https?://[^"]+/t/[^"]+)")re", std::regex::icase);
    static const std::regex cat_re(R"re(class="cn[^"]*"[^>]*>([^<]+)</div>)re", std::regex::icase);

    std::sregex_iterator it(html.begin(), html.end(), row_re);
    std::sregex_iterator end;
    int count = 0;
    for (; it != end; ++it)
    {
        const std::string row = (*it).str();
        std::smatch m_link;
        if (!std::regex_search(row, m_link, link_re))
            continue;
        const std::string comic_id = m_link[1].str() + "/" + m_link[2].str();
        std::string title = comic_id;
        std::smatch m_title;
        if (std::regex_search(row, m_title, title_re))
            title = StripTags(m_title[1].str());
        std::string cover;
        std::smatch m_cover;
        if (std::regex_search(row, m_cover, cover_re) || std::regex_search(row, m_cover, cover_re_fallback) ||
            std::regex_search(row, m_cover, cover_re_legacy))
            cover = m_cover[1].str();
        std::string category;
        std::smatch m_cat;
        if (std::regex_search(row, m_cat, cat_re))
            category = StripTags(m_cat[1].str());
        json::glaze_append(result["items"], MakeItem(comic_id, title, cover, category));
        ++count;
        if (count >= 40)
            break;
    }

    if (count == 0)
        result["max_page"] = normalized_page;
    *out = std::move(result);
    return true;
}

bool EhentaiDetail(const std::string& comic_id,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "e-hentai detail output is null");
        return false;
    }
    std::string gid;
    std::string token;
    if (!LooksLikeGidToken(comic_id, &gid, &token))
    {
        SetError(error, "invalid e-hentai comic id");
        return false;
    }
    const std::string url = "https://e-hentai.org/g/" + gid + "/" + token + "/";
    std::string html;
    if (!FetchHtml(url, session_cookie, &html, error))
        return false;

    static const std::regex title_re(R"re(<h1 id="gn">([\s\S]*?)</h1>)re", std::regex::icase);
    static const std::regex cover_re(R"re(id="gd1"[\s\S]*?(?:data-src|src)="(https?://[^"]+)")re", std::regex::icase);
    static const std::regex desc_re(R"re(<div id="gdd">([\s\S]*?)</div>\s*<div id="gdr">)re", std::regex::icase);

    std::string title = comic_id;
    std::smatch m_title;
    if (std::regex_search(html, m_title, title_re))
        title = StripTags(m_title[1].str());
    std::string cover;
    std::smatch m_cover;
    if (std::regex_search(html, m_cover, cover_re))
        cover = m_cover[1].str();
    std::string description;
    std::smatch m_desc;
    if (std::regex_search(html, m_desc, desc_re))
        description = StripTags(m_desc[1].str());

    JsonValue result;
    result["source_id"] = kEhentaiSourceId;
    result["comic_id"] = comic_id;
    result["title"] = title;
    result["description"] = description;
    result["cover"] = ImageProxyUrl(kEhentaiSourceId, cover);
    result["chapters"] = JsonValue{json::array_t{}};

    JsonValue chapter;
    chapter["source_id"] = kEhentaiSourceId;
    chapter["comic_id"] = comic_id;
    chapter["chapter_id"] = comic_id;
    chapter["title"] = "Gallery";
    chapter["order"] = 1;
    json::glaze_append(result["chapters"], chapter);
    *out = std::move(result);
    return true;
}

bool EhentaiPages(const std::string& chapter_id,
                  const std::string& session_cookie,
                  json::JsonValue* out,
                  std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, "e-hentai pages output is null");
        return false;
    }
    std::string gid;
    std::string token;
    if (!LooksLikeGidToken(chapter_id, &gid, &token))
    {
        SetError(error, "invalid e-hentai chapter id");
        return false;
    }

    JsonValue result;
    result["source_id"] = kEhentaiSourceId;
    result["chapter_id"] = chapter_id;
    result["pages"] = JsonValue{json::array_t{}};

    int index = 1;
    for (int page = 0; page < 20; ++page)
    {
        const std::string url = "https://e-hentai.org/g/" + gid + "/" + token + "/?p=" + std::to_string(page);
        std::string html;
        if (!FetchHtml(url, session_cookie, &html, error))
            return false;

        static const std::regex page_re(R"re(href="(https?://e-hentai\.org/s/[0-9a-f]+/\d+-\d+)")re",
                                        std::regex::icase);
        std::sregex_iterator it(html.begin(), html.end(), page_re);
        std::sregex_iterator end;
        int found_on_page = 0;
        for (; it != end; ++it)
        {
            const std::string page_url = (*it)[1].str();
            std::string page_html;
            if (!FetchHtml(page_url, session_cookie, &page_html, error))
                continue;
            static const std::regex img_re(R"re(id="img"[^>]+src="(https?://[^"]+)")re", std::regex::icase);
            std::smatch m_img;
            if (!std::regex_search(page_html, m_img, img_re))
                continue;
            JsonValue page_value;
            page_value["index"] = index;
            page_value["image_id"] = chapter_id + "-p" + std::to_string(index);
            page_value["url"] = ImageProxyUrl(kEhentaiSourceId, m_img[1].str());
            json::glaze_append(result["pages"], page_value);
            ++index;
            ++found_on_page;
            if (index > 200)
                break;
        }
        if (found_on_page == 0 || index > 200)
            break;
    }

    *out = std::move(result);
    return true;
}

R18ImagePayload EhentaiFetchImage(const std::filesystem::path& cache_root,
                                  const std::string& image_url,
                                  const std::string& session_cookie)
{
    std::string error;
    ParsedUrl parsed;
    if (!ParseUrl(image_url, &parsed, &error))
        return PlaceholderImage("e-hentai image error", error);
    const bool host_ok =
        parsed.host.find("ehgt.org") != std::string::npos || parsed.host.find("e-hentai.org") != std::string::npos ||
        parsed.host.find("exhentai.org") != std::string::npos || parsed.host.find("hath.network") != std::string::npos;
    if (parsed.scheme != "https" || !host_ok)
        return PlaceholderImage("e-hentai image error", "image host not allowed");

    std::string cache_key;
    if (!Md5Hex(image_url, &cache_key, &error))
        return PlaceholderImage("e-hentai image error", error);
    R18ImagePayload cached;
    if (ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    HttpResult response;
    if (!HttpGet(image_url, BaseHeaders(session_cookie), &response, &error, 15))
        return PlaceholderImage("e-hentai image error", error);
    if (response.status < 200 || response.status >= 300 || response.body.empty())
        return PlaceholderImage("e-hentai image unavailable", "HTTP " + std::to_string(response.status));

    R18ImagePayload payload;
    payload.content_type = response.content_type.empty() ? "image/jpeg" : response.content_type;
    payload.body = std::move(response.body);
    WriteCachedImage(cache_root, cache_key, payload);
    return payload;
}

} // namespace memochat::r18
