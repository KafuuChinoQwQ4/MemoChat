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

const char* NonHttpsImageRejectedMessage()
{
    return "non-https Picacg image URL rejected";
}

bool ShouldRejectImageScheme(bool scheme_is_https)
{
    return !scheme_is_https;
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

bool ShouldUseDefaultImageContentType(bool content_type_empty)
{
    return content_type_empty;
}

const char* DefaultImageContentType()
{
    return "image/jpeg";
}
} // namespace memochat::r18::picacg_adapter::modules
