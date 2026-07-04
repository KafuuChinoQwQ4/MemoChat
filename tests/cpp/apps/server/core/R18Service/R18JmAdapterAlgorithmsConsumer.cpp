import memochat.r18.jm_adapter_algorithms;

namespace memochat::tests::r18::jm_adapter
{
const char* SourceId()
{
    return memochat::r18::jm_adapter::modules::SourceId();
}

const char* ImageBaseUrl()
{
    return memochat::r18::jm_adapter::modules::ImageBaseUrl();
}

const char* ImageHost()
{
    return memochat::r18::jm_adapter::modules::ImageHost();
}

const char* Version()
{
    return memochat::r18::jm_adapter::modules::Version();
}

const char* PackageName()
{
    return memochat::r18::jm_adapter::modules::PackageName();
}

const char* UserAgent()
{
    return memochat::r18::jm_adapter::modules::UserAgent();
}

int ApiTimeoutSeconds()
{
    return memochat::r18::jm_adapter::modules::ApiTimeoutSeconds();
}

int ImageTimeoutSeconds()
{
    return memochat::r18::jm_adapter::modules::ImageTimeoutSeconds();
}

int MaxConcurrentImageFetches()
{
    return memochat::r18::jm_adapter::modules::MaxConcurrentImageFetches();
}

int SearchPageSize()
{
    return memochat::r18::jm_adapter::modules::SearchPageSize();
}

const char* ApiHostAt(int index)
{
    return memochat::r18::jm_adapter::modules::ApiHostAt(index);
}

int ApiHostCount()
{
    return memochat::r18::jm_adapter::modules::ApiHostCount();
}

bool ShouldTrimJsonPayload(bool start_missing, bool end_missing, bool end_before_start)
{
    return memochat::r18::jm_adapter::modules::ShouldTrimJsonPayload(start_missing, end_missing, end_before_start);
}

const char* InvalidDecryptedPayloadMessage()
{
    return memochat::r18::jm_adapter::modules::InvalidDecryptedPayloadMessage();
}

const char* OfficialApiFailureMessage()
{
    return memochat::r18::jm_adapter::modules::OfficialApiFailureMessage();
}

int NormalizeSearchPage(int page)
{
    return memochat::r18::jm_adapter::modules::NormalizeSearchPage(page);
}

bool ShouldAppendSearchPage(int normalized_page)
{
    return memochat::r18::jm_adapter::modules::ShouldAppendSearchPage(normalized_page);
}

bool ShouldUseNextSearchPage(int returned_count, int page_size)
{
    return memochat::r18::jm_adapter::modules::ShouldUseNextSearchPage(returned_count, page_size);
}

bool ShouldStripJmPrefix(bool starts_with_jm_prefix)
{
    return memochat::r18::jm_adapter::modules::ShouldStripJmPrefix(starts_with_jm_prefix);
}

int DefaultChapterOrder()
{
    return memochat::r18::jm_adapter::modules::DefaultChapterOrder();
}

bool ShouldUseFallbackChapter(bool chapters_empty)
{
    return memochat::r18::jm_adapter::modules::ShouldUseFallbackChapter(chapters_empty);
}

const char* DefaultChapterTitle()
{
    return memochat::r18::jm_adapter::modules::DefaultChapterTitle();
}

const char* ImageUrlRejectedMessage()
{
    return memochat::r18::jm_adapter::modules::ImageUrlRejectedMessage();
}

bool IsAllowedImageUrl(bool scheme_is_https, bool host_matches, bool target_has_media_prefix)
{
    return memochat::r18::jm_adapter::modules::IsAllowedImageUrl(scheme_is_https,
                                                                 host_matches,
                                                                 target_has_media_prefix);
}

const char* ImageTargetPrefix()
{
    return memochat::r18::jm_adapter::modules::ImageTargetPrefix();
}

bool ShouldUseDefaultImageContentType(bool content_type_empty)
{
    return memochat::r18::jm_adapter::modules::ShouldUseDefaultImageContentType(content_type_empty);
}

const char* DefaultImageContentType()
{
    return memochat::r18::jm_adapter::modules::DefaultImageContentType();
}
} // namespace memochat::tests::r18::jm_adapter
