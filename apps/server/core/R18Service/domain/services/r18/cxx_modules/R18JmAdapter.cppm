export module memochat.r18.jm_adapter_algorithms;

export namespace memochat::r18::jm_adapter::modules
{
const char* SourceId()
{
    return "jm.official";
}

const char* ImageBaseUrl()
{
    return "https://cdn-msp.jmapinodeudzn.net";
}

const char* ImageHost()
{
    return "cdn-msp.jmapinodeudzn.net";
}

const char* Version()
{
    return "2.0.16";
}

const char* PackageName()
{
    return "com.example.app";
}

const char* UserAgent()
{
    return "Mozilla/5.0 (Linux; Android 10; K; wv) AppleWebKit/537.36 "
           "(KHTML, like Gecko) Version/4.0 Chrome/130.0.0.0 Mobile Safari/537.36";
}

int ApiTimeoutSeconds()
{
    return 6;
}

int ImageTimeoutSeconds()
{
    return 5;
}

int MaxConcurrentImageFetches()
{
    return 8;
}

int SearchPageSize()
{
    return 40;
}

const char* ApiHostAt(int index)
{
    switch (index)
    {
        case 0:
            return "www.cdnsha.org";
        case 1:
            return "www.cdnntr.cc";
        case 2:
            return "www.cdntwice.org";
        case 3:
            return "www.cdnaspa.cc";
        default:
            return "";
    }
}

int ApiHostCount()
{
    return 4;
}

bool ShouldTrimJsonPayload(bool start_missing, bool end_missing, bool end_before_start)
{
    return !start_missing && !end_missing && !end_before_start;
}

const char* InvalidDecryptedPayloadMessage()
{
    return "decrypted payload is not JSON";
}

const char* OfficialApiFailureMessage()
{
    return "JM official API request failed";
}

int NormalizeSearchPage(int page)
{
    return page < 1 ? 1 : page;
}

bool ShouldAppendSearchPage(int normalized_page)
{
    return normalized_page > 1;
}

bool ShouldUseNextSearchPage(int returned_count, int page_size)
{
    return returned_count >= page_size;
}

bool ShouldStripJmPrefix(bool starts_with_jm_prefix)
{
    return starts_with_jm_prefix;
}

int DefaultChapterOrder()
{
    return 1;
}

bool ShouldUseFallbackChapter(bool chapters_empty)
{
    return chapters_empty;
}

const char* DefaultChapterTitle()
{
    return "第1話";
}

const char* ImageUrlRejectedMessage()
{
    return "image url is not an allowed JM media URL";
}

bool IsAllowedImageScheme(bool scheme_is_https)
{
    return scheme_is_https;
}

bool IsAllowedImageHost(bool host_matches)
{
    return host_matches;
}

bool IsAllowedImageTarget(bool target_has_media_prefix)
{
    return target_has_media_prefix;
}

bool IsAllowedImageUrl(bool scheme_is_https, bool host_matches, bool target_has_media_prefix)
{
    return IsAllowedImageScheme(scheme_is_https) && IsAllowedImageHost(host_matches) &&
           IsAllowedImageTarget(target_has_media_prefix);
}

const char* ImageTargetPrefix()
{
    return "/media/";
}

bool ShouldUseDefaultImageContentType(bool content_type_empty)
{
    return content_type_empty;
}

const char* DefaultImageContentType()
{
    return "image/jpeg";
}

// --- Login / auth ---

const char* LoginTarget()
{
    return "/login";
}

const char* LoginUsernameField()
{
    return "username";
}

const char* LoginPasswordField()
{
    return "password";
}

const char* LoginUidField()
{
    return "uid";
}

// Authorization header prefix; append uid after the space for authenticated calls.
const char* AuthorizationBearer()
{
    return "Bearer";
}

const char* LoginFailedMessage()
{
    return "JM login failed";
}

const char* LoginRequiredMessage()
{
    return "JM username/password required";
}

// --- Daily check-in ---

const char* CheckinTarget()
{
    return "/member/punch_clock";
}

const char* CheckinSuccessStatus()
{
    return "ok";
}

const char* CheckinAlreadyDoneStatus()
{
    return "already";
}

bool IsCheckinSuccess(bool success_code)
{
    return success_code;
}

const char* CheckinFailedMessage()
{
    return "JM check-in failed";
}

int LoginTimeoutSeconds()
{
    return 8;
}

} // namespace memochat::r18::jm_adapter::modules
