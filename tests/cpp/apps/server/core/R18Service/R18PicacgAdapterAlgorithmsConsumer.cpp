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

const char* NonHttpsImageRejectedMessage()
{
    return memochat::r18::picacg_adapter::modules::NonHttpsImageRejectedMessage();
}

bool ShouldRejectImageScheme(bool scheme_is_https)
{
    return memochat::r18::picacg_adapter::modules::ShouldRejectImageScheme(scheme_is_https);
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

bool ShouldUseDefaultImageContentType(bool content_type_empty)
{
    return memochat::r18::picacg_adapter::modules::ShouldUseDefaultImageContentType(content_type_empty);
}

const char* DefaultImageContentType()
{
    return memochat::r18::picacg_adapter::modules::DefaultImageContentType();
}
} // namespace memochat::tests::r18::picacg_adapter
