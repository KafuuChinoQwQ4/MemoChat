import memochat.r18.picacg_adapter_algorithms;

namespace memochat::tests::r18::picacg_adapter
{
const char* SourceId()
{
    return memochat::r18::picacg_adapter::modules::SourceId();
}

const char* ApiHost()
{
    return memochat::r18::picacg_adapter::modules::ApiHost();
}

const char* ApiKey()
{
    return memochat::r18::picacg_adapter::modules::ApiKey();
}

const char* HmacKey()
{
    return memochat::r18::picacg_adapter::modules::HmacKey();
}

bool HasCredentials(bool api_key_empty, bool hmac_key_empty)
{
    return memochat::r18::picacg_adapter::modules::HasCredentials(api_key_empty, hmac_key_empty);
}

bool HasValidSigningMaterial(unsigned long long api_key_size, unsigned long long hmac_key_size)
{
    return memochat::r18::picacg_adapter::modules::HasValidSigningMaterial(api_key_size, hmac_key_size);
}

const char* MissingCredentialsMessage()
{
    return memochat::r18::picacg_adapter::modules::MissingCredentialsMessage();
}

const char* GetMethod()
{
    return memochat::r18::picacg_adapter::modules::GetMethod();
}

const char* PostMethod()
{
    return memochat::r18::picacg_adapter::modules::PostMethod();
}

int ApiTimeoutSeconds()
{
    return memochat::r18::picacg_adapter::modules::ApiTimeoutSeconds();
}

int NormalizeSearchPage(int page)
{
    return memochat::r18::picacg_adapter::modules::NormalizeSearchPage(page);
}

bool ShouldStripLeadingSlash(bool path_empty, bool starts_with_slash)
{
    return memochat::r18::picacg_adapter::modules::ShouldStripLeadingSlash(path_empty, starts_with_slash);
}

int SuccessStatusCode()
{
    return memochat::r18::picacg_adapter::modules::SuccessStatusCode();
}

bool IsSuccessStatus(int status)
{
    return memochat::r18::picacg_adapter::modules::IsSuccessStatus(status);
}

bool IsSuccessfulHttpRange(int status)
{
    return memochat::r18::picacg_adapter::modules::IsSuccessfulHttpRange(status);
}

const char* InvalidJsonResponseMessage()
{
    return memochat::r18::picacg_adapter::modules::InvalidJsonResponseMessage();
}

const char* SearchInvalidJsonMessage()
{
    return memochat::r18::picacg_adapter::modules::SearchInvalidJsonMessage();
}

const char* DefaultEpisodeTitle()
{
    return memochat::r18::picacg_adapter::modules::DefaultEpisodeTitle();
}

int DefaultEpisodeOrder()
{
    return memochat::r18::picacg_adapter::modules::DefaultEpisodeOrder();
}

bool ShouldUseFallbackEpisode(bool has_no_episodes)
{
    return memochat::r18::picacg_adapter::modules::ShouldUseFallbackEpisode(has_no_episodes);
}

const char* ImageReferer()
{
    return memochat::r18::picacg_adapter::modules::ImageReferer();
}

const char* AllowedImageHostsConfigSection()
{
    return memochat::r18::picacg_adapter::modules::AllowedImageHostsConfigSection();
}

const char* AllowedImageHostsConfigKey()
{
    return memochat::r18::picacg_adapter::modules::AllowedImageHostsConfigKey();
}

const char* ImageTargetPrefix()
{
    return memochat::r18::picacg_adapter::modules::ImageTargetPrefix();
}

const char* RedirectImageHost()
{
    return memochat::r18::picacg_adapter::modules::RedirectImageHost();
}

bool IsRedirectImageTarget(const char* target, unsigned long long target_size)
{
    return memochat::r18::picacg_adapter::modules::IsRedirectImageTarget(target, target_size);
}

bool IsExactHostInPolicy(const char* host,
                         unsigned long long host_size,
                         const char* policy,
                         unsigned long long policy_size)
{
    return memochat::r18::picacg_adapter::modules::IsExactHostInPolicy(host, host_size, policy, policy_size);
}

bool IsCanonicalAllowedImageUrl(bool scheme_is_https,
                                bool port_is_443,
                                bool has_userinfo,
                                bool has_fragment,
                                bool host_allowed,
                                bool target_allowed)
{
    return memochat::r18::picacg_adapter::modules::IsCanonicalAllowedImageUrl(scheme_is_https,
                                                                              port_is_443,
                                                                              has_userinfo,
                                                                              has_fragment,
                                                                              host_allowed,
                                                                              target_allowed);
}

bool IsPublicIpv4Address(unsigned int address)
{
    return memochat::r18::picacg_adapter::modules::IsPublicIpv4Address(address);
}

bool IsPublicIpv6Address(const unsigned char* bytes, unsigned long long size)
{
    return memochat::r18::picacg_adapter::modules::IsPublicIpv6Address(bytes, size);
}

unsigned long long MaxImageBytes()
{
    return memochat::r18::picacg_adapter::modules::MaxImageBytes();
}

bool IsAllowedImageContentType(bool jpeg, bool png, bool webp, bool gif, bool avif)
{
    return memochat::r18::picacg_adapter::modules::IsAllowedImageContentType(jpeg, png, webp, gif, avif);
}

bool IsImageRedirectStatus(int status)
{
    return memochat::r18::picacg_adapter::modules::IsImageRedirectStatus(status);
}

unsigned long long MaxImageRedirects()
{
    return memochat::r18::picacg_adapter::modules::MaxImageRedirects();
}

const char* ImageUnavailableTitle()
{
    return memochat::r18::picacg_adapter::modules::ImageUnavailableTitle();
}

const char* ImageErrorTitle()
{
    return memochat::r18::picacg_adapter::modules::ImageErrorTitle();
}

bool ShouldUseImagePlaceholder(int status, bool body_empty)
{
    return memochat::r18::picacg_adapter::modules::ShouldUseImagePlaceholder(status, body_empty);
}

} // namespace memochat::tests::r18::picacg_adapter
