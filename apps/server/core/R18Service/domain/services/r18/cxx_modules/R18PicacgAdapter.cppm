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

const char* MissingCredentialsMessage()
{
    return "Picacg credentials missing";
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

} // namespace memochat::r18::picacg_adapter::modules
