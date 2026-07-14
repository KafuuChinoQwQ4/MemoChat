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


const char* DefaultSearchSort()
{
    return "mr";
}

const char* NormalizeSearchSort(const char* sort)
{
    // JMComic order codes:
    // mr=最新, mv=最多观看(总), mv_t=今日, mv_w=本周, mv_m=本月
    // tf=最多图片, mp=最多点赞, ld=最多爱心/喜欢
    if (sort == nullptr || sort[0] == '\0')
        return DefaultSearchSort();
    // Compare via string_view without allocating when possible is harder without include;
    // use simple strcmp-like loops through fixed table.
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
    if (eq(sort, "mr") || eq(sort, "latest") || eq(sort, "new") || eq(sort, "date"))
        return "mr";
    if (eq(sort, "mv") || eq(sort, "views") || eq(sort, "hot") || eq(sort, "popular"))
        return "mv";
    if (eq(sort, "mv_t") || eq(sort, "today") || eq(sort, "day"))
        return "mv_t";
    if (eq(sort, "mv_w") || eq(sort, "week"))
        return "mv_w";
    if (eq(sort, "mv_m") || eq(sort, "month"))
        return "mv_m";
    if (eq(sort, "tf") || eq(sort, "images") || eq(sort, "pages"))
        return "tf";
    if (eq(sort, "mp") || eq(sort, "likes") || eq(sort, "like"))
        return "mp";
    if (eq(sort, "ld") || eq(sort, "heart") || eq(sort, "love"))
        return "ld";
    // Pass through already-valid JM codes.
    if (eq(sort, "mr") || eq(sort, "mv") || eq(sort, "mv_t") || eq(sort, "mv_w") || eq(sort, "mv_m") ||
        eq(sort, "tf") || eq(sort, "mp") || eq(sort, "ld"))
        return sort;
    return DefaultSearchSort();
}


// JMComic photo pages are vertically scrambled. Covers under /media/albums/ are not.
// Constants and strip math match the public JMComic client algorithm.

int ScrambleIdThreshold()
{
    return 220980;
}

int ScrambleFixedTenThreshold()
{
    return 268850;
}

int ScrambleModEightThreshold()
{
    return 421926;
}

// filename is the image basename without extension (e.g. "00001").
// md5_last_byte is the last hex char of md5(aid + filename), as an ASCII byte.
int ScrambleStripCount(long long aid, long long scramble_id, unsigned char md5_last_byte)
{
    if (aid < scramble_id)
        return 0;
    if (aid < ScrambleFixedTenThreshold())
        return 10;
    const int modulus = aid < ScrambleModEightThreshold() ? 10 : 8;
    int num = static_cast<int>(md5_last_byte) % modulus;
    num = num * 2 + 2;
    return num;
}

// Source Y of the strip that should be pasted at destination index i.
// Returns strip height through *strip_height_out.
int ScrambleSourceY(int image_height, int strip_count, int destination_index, int* strip_height_out)
{
    if (strip_height_out == nullptr || strip_count <= 1 || image_height <= 0 || destination_index < 0 ||
        destination_index >= strip_count)
    {
        if (strip_height_out != nullptr)
            *strip_height_out = 0;
        return 0;
    }
    const int over = image_height % strip_count;
    const int base = image_height / strip_count;
    int move = base;
    const int y_src = image_height - (base * (destination_index + 1)) - over;
    if (destination_index == 0)
        move += over;
    *strip_height_out = move;
    return y_src;
}

int ScrambleDestinationY(int image_height, int strip_count, int destination_index)
{
    if (strip_count <= 1 || image_height <= 0 || destination_index < 0 || destination_index >= strip_count)
        return 0;
    const int over = image_height % strip_count;
    const int base = image_height / strip_count;
    int y_dst = base * destination_index;
    if (destination_index != 0)
        y_dst += over;
    return y_dst;
}

} // namespace memochat::r18::jm_adapter::modules
