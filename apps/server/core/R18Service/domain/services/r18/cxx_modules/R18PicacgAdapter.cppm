module;

#include <cstdlib>

export module memochat.r18.picacg_adapter_algorithms;

export namespace memochat::r18::picacg_adapter::modules
{
const char* SourceId()
{
    return "picacg.official";
}

const char* ApiHost()
{
    return "picaapi.picacomic.com";
}

const char* ApiKey()
{
    const char* value = std::getenv("MEMOCHAT_R18_PICACG_API_KEY");
    return value == nullptr ? "" : value;
}

const char* HmacKey()
{
    const char* value = std::getenv("MEMOCHAT_R18_PICACG_HMAC_KEY");
    return value == nullptr ? "" : value;
}

bool HasCredentials(bool api_key_empty, bool hmac_key_empty)
{
    return !api_key_empty && !hmac_key_empty;
}

bool HasValidSigningMaterial(unsigned long long api_key_size, unsigned long long hmac_key_size)
{
    return api_key_size == 29 && hmac_key_size == 63;
}

const char* MissingCredentialsMessage()
{
    return "Picacg signing configuration is missing or invalid";
}

const char* AccountLoginRequiredMessage()
{
    return "Picacg account login required";
}

const char* GetMethod()
{
    return "GET";
}

const char* PostMethod()
{
    return "POST";
}

int ApiTimeoutSeconds()
{
    return 8;
}

int NormalizeSearchPage(int page)
{
    return page < 1 ? 1 : page;
}

bool ShouldStripLeadingSlash(bool path_empty, bool starts_with_slash)
{
    return !path_empty && starts_with_slash;
}

int SuccessStatusCode()
{
    return 200;
}

bool IsSuccessStatus(int status)
{
    return status == SuccessStatusCode();
}

bool IsSuccessfulHttpRange(int status)
{
    return status >= 200 && status < 300;
}

const char* InvalidJsonResponseMessage()
{
    return "Picacg invalid JSON response";
}

const char* SearchInvalidJsonMessage()
{
    return "Picacg search invalid JSON";
}

const char* DefaultEpisodeTitle()
{
    return "Episode 1";
}

int DefaultEpisodeOrder()
{
    return 1;
}

bool ShouldUseFallbackEpisode(bool has_no_episodes)
{
    return has_no_episodes;
}

const char* ImageReferer()
{
    return "https://manhuabika.com/";
}

const char* AllowedImageHostsConfigSection()
{
    return "R18Picacg";
}

const char* AllowedImageHostsConfigKey()
{
    return "AllowedImageHosts";
}

const char* ImageTargetPrefix()
{
    return "/static/";
}

const char* RedirectImageHost()
{
    return "img.picacomic.com";
}

namespace detail
{
bool IsAsciiSpace(char value)
{
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

char LowerAscii(char value)
{
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
}

bool EqualsAsciiCaseInsensitive(const char* left,
                                unsigned long long left_size,
                                const char* right,
                                unsigned long long right_size)
{
    if (left == nullptr || right == nullptr || left_size != right_size)
        return false;
    for (unsigned long long index = 0; index < left_size; ++index)
        if (LowerAscii(left[index]) != LowerAscii(right[index]))
            return false;
    return true;
}

bool IsIpv4Prefix(unsigned int address, unsigned int prefix, unsigned int bits)
{
    const unsigned int mask = bits == 0 ? 0U : (~0U << (32U - bits));
    return (address & mask) == (prefix & mask);
}
} // namespace detail

bool IsRedirectImageTarget(const char* target, unsigned long long target_size)
{
    if (target == nullptr || target_size < 5 || target[0] != '/')
        return false;
    for (unsigned long long index = 0; index < target_size; ++index)
    {
        const unsigned char value = static_cast<unsigned char>(target[index]);
        if (value < 0x20U || target[index] == '?' || target[index] == '#' || target[index] == '\\')
            return false;
    }
    const auto has_extension = [&](const char* extension, unsigned long long extension_size)
    {
        return target_size > extension_size &&
               detail::EqualsAsciiCaseInsensitive(
                   target + target_size - extension_size, extension_size, extension, extension_size);
    };
    return has_extension(".jpg", 4) || has_extension(".jpeg", 5) || has_extension(".png", 4) ||
           has_extension(".webp", 5) || has_extension(".gif", 4) || has_extension(".avif", 5);
}

bool IsExactHostInPolicy(const char* host,
                         unsigned long long host_size,
                         const char* policy,
                         unsigned long long policy_size)
{
    if (host == nullptr || host_size == 0 || policy == nullptr || policy_size == 0)
        return false;

    unsigned long long cursor = 0;
    while (cursor < policy_size)
    {
        while (cursor < policy_size && (policy[cursor] == ',' || detail::IsAsciiSpace(policy[cursor])))
            ++cursor;
        const unsigned long long begin = cursor;
        while (cursor < policy_size && policy[cursor] != ',')
            ++cursor;
        unsigned long long end = cursor;
        while (end > begin && detail::IsAsciiSpace(policy[end - 1]))
            --end;
        if (end > begin && detail::EqualsAsciiCaseInsensitive(host, host_size, policy + begin, end - begin))
            return true;
    }
    return false;
}

bool IsCanonicalAllowedImageUrl(bool scheme_is_https,
                                bool port_is_443,
                                bool has_userinfo,
                                bool has_fragment,
                                bool host_allowed,
                                bool target_allowed)
{
    return scheme_is_https && port_is_443 && !has_userinfo && !has_fragment && host_allowed && target_allowed;
}

bool IsPublicIpv4Address(unsigned int address)
{
    return !detail::IsIpv4Prefix(address, 0x00000000U, 8) &&
           !detail::IsIpv4Prefix(address, 0x0A000000U, 8) &&
           !detail::IsIpv4Prefix(address, 0x64400000U, 10) &&
           !detail::IsIpv4Prefix(address, 0x7F000000U, 8) &&
           !detail::IsIpv4Prefix(address, 0xA9FE0000U, 16) &&
           !detail::IsIpv4Prefix(address, 0xAC100000U, 12) &&
           !detail::IsIpv4Prefix(address, 0xC0000000U, 24) &&
           !detail::IsIpv4Prefix(address, 0xC0000200U, 24) &&
           !detail::IsIpv4Prefix(address, 0xC0A80000U, 16) &&
           !detail::IsIpv4Prefix(address, 0xC6120000U, 15) &&
           !detail::IsIpv4Prefix(address, 0xC6336400U, 24) &&
           !detail::IsIpv4Prefix(address, 0xCB007100U, 24) &&
           !detail::IsIpv4Prefix(address, 0xE0000000U, 4) &&
           !detail::IsIpv4Prefix(address, 0xF0000000U, 4);
}

bool IsPublicIpv6Address(const unsigned char* bytes, unsigned long long size)
{
    if (bytes == nullptr || size != 16)
        return false;

    bool all_zero = true;
    for (unsigned long long index = 0; index < size; ++index)
        all_zero = all_zero && bytes[index] == 0;
    if (all_zero)
        return false;

    bool loopback = true;
    for (unsigned long long index = 0; index < 15; ++index)
        loopback = loopback && bytes[index] == 0;
    if (loopback && bytes[15] == 1)
        return false;

    const bool global_unicast = (bytes[0] & 0xE0U) == 0x20U;
    const bool documentation = bytes[0] == 0x20U && bytes[1] == 0x01U && bytes[2] == 0x0DU && bytes[3] == 0xB8U;
    return global_unicast && !documentation;
}

unsigned long long MaxImageBytes()
{
    return 16ULL * 1024ULL * 1024ULL;
}

bool IsAllowedImageContentType(bool jpeg, bool png, bool webp, bool gif, bool avif)
{
    return jpeg || png || webp || gif || avif;
}

bool IsImageRedirectStatus(int status)
{
    return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

unsigned long long MaxImageRedirects()
{
    return 2;
}

const char* ImageUnavailableTitle()
{
    return "Picacg image unavailable";
}

const char* ImageErrorTitle()
{
    return "Picacg image error";
}

bool ShouldUseImagePlaceholder(int status, bool body_empty)
{
    return !IsSuccessfulHttpRange(status) || body_empty;
}


const char* DefaultSearchSort()
{
    return "dd";
}

const char* NormalizeSearchSort(const char* sort)
{
    // Picacg advanced-search sort:
    // dd=最新, da=最旧, ld=最多爱心, vd=最多指名
    if (sort == nullptr || sort[0] == '\0')
        return DefaultSearchSort();
    auto eq = [](const char* a, const char* b) {
        if (a == nullptr || b == nullptr)
            return false;
        while (*a != '\0' && *b != '\0')
        {
            if (*a != *b)
                return false;
            ++a;
            ++b;
        }
        return *a == *b;
    };
    if (eq(sort, "dd") || eq(sort, "latest") || eq(sort, "new") || eq(sort, "date"))
        return "dd";
    if (eq(sort, "da") || eq(sort, "oldest") || eq(sort, "old"))
        return "da";
    if (eq(sort, "ld") || eq(sort, "likes") || eq(sort, "like") || eq(sort, "heart"))
        return "ld";
    if (eq(sort, "vd") || eq(sort, "views") || eq(sort, "hot") || eq(sort, "popular"))
        return "vd";
    if (eq(sort, "dd") || eq(sort, "da") || eq(sort, "ld") || eq(sort, "vd"))
        return sort;
    return DefaultSearchSort();
}

} // namespace memochat::r18::picacg_adapter::modules
