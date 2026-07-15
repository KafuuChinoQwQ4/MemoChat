#include "r18/R18EhentaiAdapter.hpp"
#include "r18/R18AdapterUtils.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace memochat::r18
{

const char* const kEhentaiSourceId = "ehentai.official";
const char* const kExhentaiSourceId = "exhentai.official";

namespace
{

using namespace detail;
using json::JsonValue;

struct SiteConfig
{
    const char* source_id;
    const char* origin; // https://e-hentai.org  / https://exhentai.org
    const char* label;  // human label for errors
};

const SiteConfig kEhentaiSite{kEhentaiSourceId, "https://e-hentai.org", "e-hentai"};
const SiteConfig kExhentaiSite{kExhentaiSourceId, "https://exhentai.org", "exhentai"};

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

std::vector<std::pair<std::string, std::string>> BaseHeaders(const SiteConfig& site, const std::string& session_cookie)
{
    std::vector<std::pair<std::string, std::string>> headers = {
        {"Accept", "text/html,application/xhtml+xml,application/json;q=0.9,*/*;q=0.8"},
        {"User-Agent",
         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 "
         "Safari/537.36"},
        {"Referer", std::string(site.origin) + "/"},
    };
    if (!session_cookie.empty())
        headers.emplace_back("Cookie", session_cookie);
    return headers;
}

bool FetchHtml(const SiteConfig& site,
               const std::string& url,
               const std::string& session_cookie,
               std::string* body,
               std::string* error)
{
    HttpResult response;
    if (!HttpGet(url, BaseHeaders(site, session_cookie), &response, error, 15))
        return false;
    if (response.status < 200 || response.status >= 300)
    {
        SetError(error, std::string(site.label) + " HTTP " + std::to_string(response.status));
        return false;
    }
    *body = std::move(response.body);
    return true;
}

bool LooksLikeSadPanda(const std::string& html)
{
    if (html.empty())
        return true;
    // Classic "sad panda" gate / empty access denial page.
    if (html.find("This gallery has been removed") != std::string::npos)
        return false; // still authenticated HTML
    if (html.find("sadpanda") != std::string::npos || html.find("Sad Panda") != std::string::npos)
        return true;
    if (html.find("glink") == std::string::npos && html.find("id=\"nb\"") == std::string::npos &&
        html.find("e-hentai.org") == std::string::npos && html.find("exhentai.org") == std::string::npos)
    {
        // Very small non-HTML (often the panda gif bytes served as the whole body).
        if (html.size() < 512)
            return true;
        if (html.find("<html") == std::string::npos && html.find("<HTML") == std::string::npos)
            return true;
    }
    return false;
}

JsonValue MakeItem(const SiteConfig& site,
                   const std::string& comic_id,
                   const std::string& title,
                   const std::string& cover,
                   const std::string& category)
{
    JsonValue item;
    item["source_id"] = site.source_id;
    item["comic_id"] = comic_id;
    item["title"] = title;
    item["subtitle"] = category;
    item["description"] = category;
    item["cover"] = ImageProxyUrl(site.source_id, cover);
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

std::string CookiePairNameValue(const std::string& set_cookie_header)
{
    // "name=value; Path=/; ..." → "name=value"
    const auto semi = set_cookie_header.find(';');
    const std::string pair = semi == std::string::npos ? set_cookie_header : set_cookie_header.substr(0, semi);
    const auto eq = pair.find('=');
    if (eq == std::string::npos || eq == 0)
        return {};
    return pair;
}

void MergeSetCookies(std::unordered_map<std::string, std::string>* jar, const std::vector<std::string>& set_cookies)
{
    if (jar == nullptr)
        return;
    for (const auto& header : set_cookies)
    {
        const std::string pair = CookiePairNameValue(header);
        if (pair.empty())
            continue;
        const auto eq = pair.find('=');
        if (eq == std::string::npos)
            continue;
        (*jar)[pair.substr(0, eq)] = pair.substr(eq + 1);
    }
}

std::string JoinCookieJar(const std::unordered_map<std::string, std::string>& jar)
{
    std::ostringstream oss;
    bool first = true;
    for (const auto& [name, value] : jar)
    {
        if (name.empty())
            continue;
        if (!first)
            oss << "; ";
        first = false;
        oss << name << '=' << value;
    }
    return oss.str();
}

bool SearchImpl(const SiteConfig& site,
                const std::string& keyword,
                int page,
                const std::string& sort,
                const std::string& tag,
                const std::string& session_cookie,
                json::JsonValue* out,
                std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, std::string(site.label) + " search output is null");
        return false;
    }
    if (site.source_id == kExhentaiSourceId && session_cookie.empty())
    {
        SetError(error, "exhentai requires e-hentai login (cookie or account)");
        return false;
    }

    const int normalized_page = page < 1 ? 1 : page;
    // sort chips map to f_cats category masks; tag remains free-text / namespace:tag search.
    const std::string cats = NormalizeEhentaiCats(sort);
    std::string search = keyword;
    if (!tag.empty())
    {
        if (!search.empty())
            search += " ";
        search += tag;
    }
    std::ostringstream url;
    url << site.origin << "/?page=" << (normalized_page - 1);
    if (!search.empty())
        url << "&f_search=" << UrlEncode(search);
    if (!cats.empty())
        url << "&f_cats=" << cats;

    std::string html;
    if (!FetchHtml(site, url.str(), session_cookie, &html, error))
        return false;

    if (site.source_id == kExhentaiSourceId && LooksLikeSadPanda(html))
    {
        SetError(error, "exhentai access denied (sad panda) — login required or cookie missing igneous");
        return false;
    }

    JsonValue result;
    result["source_id"] = site.source_id;
    result["keyword"] = keyword;
    result["sort"] = sort;
    result["tag"] = tag;
    result["page"] = normalized_page;
    result["max_page"] = normalized_page + 1; // next page available until empty
    result["items"] = JsonValue{json::array_t{}};

    // Compact list markup uses <tr> rows with gl1c/glname; covers are on ehgt.org.
    static const std::regex row_re(R"re(<tr[^>]*>[\s\S]*?gl1c[\s\S]*?</tr>)re", std::regex::icase);
    // Match either e-hentai or exhentai gallery links (ex often links relative or absolute).
    static const std::regex link_re(R"re(href="(?:https?://(?:e-hentai|exhentai)\.org)?/g/(\d+)/([0-9a-f]+)/")re",
                                    std::regex::icase);
    static const std::regex title_re(R"re(class="glink"[^>]*>([\s\S]*?)</div>)re", std::regex::icase);
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
        json::glaze_append(result["items"], MakeItem(site, comic_id, title, cover, category));
        ++count;
        if (count >= 40)
            break;
    }

    if (count == 0)
        result["max_page"] = normalized_page;
    *out = std::move(result);
    return true;
}

bool DetailImpl(const SiteConfig& site,
                const std::string& comic_id,
                const std::string& session_cookie,
                json::JsonValue* out,
                std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, std::string(site.label) + " detail output is null");
        return false;
    }
    if (site.source_id == kExhentaiSourceId && session_cookie.empty())
    {
        SetError(error, "exhentai requires e-hentai login (cookie or account)");
        return false;
    }
    std::string gid;
    std::string token;
    if (!LooksLikeGidToken(comic_id, &gid, &token))
    {
        SetError(error, std::string("invalid ") + site.label + " comic id");
        return false;
    }
    const std::string url = std::string(site.origin) + "/g/" + gid + "/" + token + "/";
    std::string html;
    if (!FetchHtml(site, url, session_cookie, &html, error))
        return false;
    if (site.source_id == kExhentaiSourceId && LooksLikeSadPanda(html))
    {
        SetError(error, "exhentai access denied (sad panda)");
        return false;
    }

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
    result["source_id"] = site.source_id;
    result["comic_id"] = comic_id;
    result["title"] = title;
    result["description"] = description;
    result["cover"] = ImageProxyUrl(site.source_id, cover);
    result["chapters"] = JsonValue{json::array_t{}};

    JsonValue chapter;
    chapter["source_id"] = site.source_id;
    chapter["comic_id"] = comic_id;
    chapter["chapter_id"] = comic_id;
    chapter["title"] = "Gallery";
    chapter["order"] = 1;
    json::glaze_append(result["chapters"], chapter);
    *out = std::move(result);
    return true;
}

bool PagesImpl(const SiteConfig& site,
               const std::string& chapter_id,
               const std::string& session_cookie,
               json::JsonValue* out,
               std::string* error)
{
    if (out == nullptr)
    {
        SetError(error, std::string(site.label) + " pages output is null");
        return false;
    }
    if (site.source_id == kExhentaiSourceId && session_cookie.empty())
    {
        SetError(error, "exhentai requires e-hentai login (cookie or account)");
        return false;
    }
    std::string gid;
    std::string token;
    if (!LooksLikeGidToken(chapter_id, &gid, &token))
    {
        SetError(error, std::string("invalid ") + site.label + " chapter id");
        return false;
    }

    JsonValue result;
    result["source_id"] = site.source_id;
    result["chapter_id"] = chapter_id;
    result["pages"] = JsonValue{json::array_t{}};

    // Page image links: absolute or host-relative on either domain.
    static const std::regex page_re(R"re(href="((?:https?://(?:e-hentai|exhentai)\.org)?/s/[0-9a-f]+/\d+-\d+)")re",
                                    std::regex::icase);
    static const std::regex img_re(R"re(id="img"[^>]+src="(https?://[^"]+)")re", std::regex::icase);

    int index = 1;
    for (int page = 0; page < 20; ++page)
    {
        const std::string url = std::string(site.origin) + "/g/" + gid + "/" + token + "/?p=" + std::to_string(page);
        std::string html;
        if (!FetchHtml(site, url, session_cookie, &html, error))
            return false;
        if (site.source_id == kExhentaiSourceId && LooksLikeSadPanda(html))
        {
            SetError(error, "exhentai access denied (sad panda)");
            return false;
        }

        std::sregex_iterator it(html.begin(), html.end(), page_re);
        std::sregex_iterator end;
        int found_on_page = 0;
        for (; it != end; ++it)
        {
            std::string page_url = (*it)[1].str();
            if (page_url.rfind("http", 0) != 0)
                page_url = std::string(site.origin) + page_url;
            std::string page_html;
            if (!FetchHtml(site, page_url, session_cookie, &page_html, error))
                continue;
            std::smatch m_img;
            if (!std::regex_search(page_html, m_img, img_re))
                continue;
            JsonValue page_value;
            page_value["index"] = index;
            page_value["image_id"] = chapter_id + "-p" + std::to_string(index);
            page_value["url"] = ImageProxyUrl(site.source_id, m_img[1].str());
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

R18ImagePayload FetchImageImpl(const SiteConfig& site,
                               const std::filesystem::path& cache_root,
                               const std::string& image_url,
                               const std::string& session_cookie)
{
    std::string error;
    ParsedUrl parsed;
    if (!ParseUrl(image_url, &parsed, &error))
        return PlaceholderImage(std::string(site.label) + " image error", error);
    const bool host_ok =
        parsed.host.find("ehgt.org") != std::string::npos || parsed.host.find("e-hentai.org") != std::string::npos ||
        parsed.host.find("exhentai.org") != std::string::npos || parsed.host.find("hath.network") != std::string::npos;
    if (parsed.scheme != "https" || !host_ok)
        return PlaceholderImage(std::string(site.label) + " image error", "image host not allowed");

    std::string cache_key;
    if (!Md5Hex(image_url, &cache_key, &error))
        return PlaceholderImage(std::string(site.label) + " image error", error);
    R18ImagePayload cached;
    if (ReadCachedImage(cache_root, cache_key, &cached))
        return cached;

    HttpResult response;
    if (!HttpGet(image_url, BaseHeaders(site, session_cookie), &response, &error, 15))
        return PlaceholderImage(std::string(site.label) + " image error", error);
    if (response.status < 200 || response.status >= 300 || response.body.empty())
        return PlaceholderImage(std::string(site.label) + " image unavailable",
                                "HTTP " + std::to_string(response.status));

    R18ImagePayload payload;
    payload.content_type = response.content_type.empty() ? "image/jpeg" : response.content_type;
    payload.body = std::move(response.body);
    WriteCachedImage(cache_root, cache_key, payload);
    return payload;
}

} // namespace

bool LooksLikeEhentaiCookie(const std::string& value)
{
    if (value.find('=') == std::string::npos)
        return false;
    return value.find("ipb_member_id=") != std::string::npos || value.find("ipb_pass_hash=") != std::string::npos ||
           value.find("igneous=") != std::string::npos || value.find("sk=") != std::string::npos;
}

bool ExhentaiValidateSession(const std::string& session_cookie, std::string* error)
{
    if (session_cookie.empty())
    {
        SetError(error, "exhentai cookie is empty");
        return false;
    }
    std::string html;
    if (!FetchHtml(kExhentaiSite, "https://exhentai.org/", session_cookie, &html, error))
        return false;
    if (LooksLikeSadPanda(html))
    {
        SetError(error, "exhentai access denied (sad panda) — need valid e-hentai cookie with igneous");
        return false;
    }
    return true;
}

bool EhentaiForumLogin(const std::string& username,
                       const std::string& password,
                       bool for_exhentai,
                       std::string* session_cookie,
                       std::string* error)
{
    if (session_cookie == nullptr)
    {
        SetError(error, "session cookie output is null");
        return false;
    }
    if (username.empty() || password.empty())
    {
        SetError(error, "e-hentai username and password are required");
        return false;
    }

    // 1) Login on forums.e-hentai.org (shared auth for e-hentai / exhentai).
    const std::string login_url = "https://forums.e-hentai.org/index.php?act=Login&CODE=01";
    std::ostringstream body;
    body << "UserName=" << UrlEncode(username) << "&PassWord=" << UrlEncode(password) << "&CookieDate=1&b=d&bt=";

    std::vector<std::pair<std::string, std::string>> headers = {
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Accept", "text/html,application/xhtml+xml,*/*;q=0.8"},
        {"User-Agent",
         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 "
         "Safari/537.36"},
        {"Referer", "https://forums.e-hentai.org/index.php?act=Login"},
        {"Origin", "https://forums.e-hentai.org"},
    };

    HttpResult response;
    if (!HttpPost(login_url, headers, body.str(), &response, error, 20))
        return false;

    std::unordered_map<std::string, std::string> jar;
    MergeSetCookies(&jar, response.set_cookies);

    // Some deployments also drop cookies on the landing page of e-hentai itself.
    if (jar.find("ipb_member_id") == jar.end() || jar.find("ipb_pass_hash") == jar.end())
    {
        // Attempt cookie harvest from e-hentai homepage with any partial cookies.
        const std::string partial = JoinCookieJar(jar);
        HttpResult home;
        std::vector<std::pair<std::string, std::string>> home_headers = BaseHeaders(kEhentaiSite, partial);
        if (HttpGet("https://e-hentai.org/", home_headers, &home, error, 15))
            MergeSetCookies(&jar, home.set_cookies);
    }

    if (jar.find("ipb_member_id") == jar.end() || jar.find("ipb_pass_hash") == jar.end())
    {
        // Forums often return 200 with an error page instead of cookies.
        if (response.body.find("You must already have registered") != std::string::npos ||
            response.body.find("Username or password incorrect") != std::string::npos ||
            response.body.find("The following errors") != std::string::npos)
        {
            SetError(error, "e-hentai login failed: invalid username or password");
            return false;
        }
        SetError(error, "e-hentai login failed: no session cookies returned");
        return false;
    }

    std::string cookie = JoinCookieJar(jar);

    // 2) For exhentai, visit the site so the server issues the igneous cookie.
    if (for_exhentai)
    {
        HttpResult ex;
        if (!HttpGet("https://exhentai.org/", BaseHeaders(kExhentaiSite, cookie), &ex, error, 15))
            return false;
        MergeSetCookies(&jar, ex.set_cookies);
        cookie = JoinCookieJar(jar);

        if (LooksLikeSadPanda(ex.body))
        {
            SetError(error,
                     "e-hentai login ok but exhentai access denied — account may lack exhentai privileges "
                     "(need igneous cookie)");
            return false;
        }
        if (jar.find("igneous") == jar.end())
        {
            // Some accounts still work without a freshly set igneous in Set-Cookie if already present.
            // Re-validate by content.
            if (!ExhentaiValidateSession(cookie, error))
                return false;
        }
    }

    *session_cookie = std::move(cookie);
    return true;
}

bool EhentaiSearch(const std::string& keyword,
                   int page,
                   const std::string& sort,
                   const std::string& tag,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error)
{
    return SearchImpl(kEhentaiSite, keyword, page, sort, tag, session_cookie, out, error);
}

bool ExhentaiSearch(const std::string& keyword,
                    int page,
                    const std::string& sort,
                    const std::string& tag,
                    const std::string& session_cookie,
                    json::JsonValue* out,
                    std::string* error)
{
    return SearchImpl(kExhentaiSite, keyword, page, sort, tag, session_cookie, out, error);
}

bool EhentaiDetail(const std::string& comic_id,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error)
{
    return DetailImpl(kEhentaiSite, comic_id, session_cookie, out, error);
}

bool ExhentaiDetail(const std::string& comic_id,
                    const std::string& session_cookie,
                    json::JsonValue* out,
                    std::string* error)
{
    return DetailImpl(kExhentaiSite, comic_id, session_cookie, out, error);
}

bool EhentaiPages(const std::string& chapter_id,
                  const std::string& session_cookie,
                  json::JsonValue* out,
                  std::string* error)
{
    return PagesImpl(kEhentaiSite, chapter_id, session_cookie, out, error);
}

bool ExhentaiPages(const std::string& chapter_id,
                   const std::string& session_cookie,
                   json::JsonValue* out,
                   std::string* error)
{
    return PagesImpl(kExhentaiSite, chapter_id, session_cookie, out, error);
}

R18ImagePayload EhentaiFetchImage(const std::filesystem::path& cache_root,
                                  const std::string& image_url,
                                  const std::string& session_cookie)
{
    return FetchImageImpl(kEhentaiSite, cache_root, image_url, session_cookie);
}

R18ImagePayload ExhentaiFetchImage(const std::filesystem::path& cache_root,
                                   const std::string& image_url,
                                   const std::string& session_cookie)
{
    return FetchImageImpl(kExhentaiSite, cache_root, image_url, session_cookie);
}

} // namespace memochat::r18
