#include <gtest/gtest.h>

#include <string_view>

namespace memochat::tests::r18::picacg_adapter
{
const char* SourceId();
const char* ApiHost();
const char* ApiKey();
const char* HmacKey();
bool HasCredentials(bool api_key_empty, bool hmac_key_empty);
const char* MissingCredentialsMessage();
const char* GetMethod();
const char* PostMethod();
int ApiTimeoutSeconds();
int NormalizeSearchPage(int page);
bool ShouldStripLeadingSlash(bool path_empty, bool starts_with_slash);
int SuccessStatusCode();
bool IsSuccessStatus(int status);
bool IsSuccessfulHttpRange(int status);
const char* InvalidJsonResponseMessage();
const char* SearchInvalidJsonMessage();
const char* DefaultEpisodeTitle();
int DefaultEpisodeOrder();
bool ShouldUseFallbackEpisode(bool has_no_episodes);
const char* ImageReferer();
const char* NonHttpsImageRejectedMessage();
bool ShouldRejectImageScheme(bool scheme_is_https);
const char* ImageUnavailableTitle();
const char* ImageErrorTitle();
bool ShouldUseImagePlaceholder(int status, bool body_empty);
bool ShouldUseDefaultImageContentType(bool content_type_empty);
const char* DefaultImageContentType();
} // namespace memochat::tests::r18::picacg_adapter

TEST(R18PicacgAdapterAlgorithmsTest, ExposesStableIdentityAndApiDefaults)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_STREQ(SourceId(), "picacg.official");
    EXPECT_STREQ(ApiHost(), "picaapi.picacomic.com");
    EXPECT_STREQ(ApiKey(), "");
    EXPECT_STREQ(HmacKey(), "");
    EXPECT_FALSE(HasCredentials(true, false));
    EXPECT_FALSE(HasCredentials(false, true));
    EXPECT_TRUE(HasCredentials(false, false));
    EXPECT_STREQ(MissingCredentialsMessage(), "Picacg credentials missing");
    EXPECT_STREQ(GetMethod(), "GET");
    EXPECT_STREQ(PostMethod(), "POST");
    EXPECT_EQ(ApiTimeoutSeconds(), 8);
}

TEST(R18PicacgAdapterAlgorithmsTest, ExposesPathStatusAndJsonGuards)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_EQ(NormalizeSearchPage(-4), 1);
    EXPECT_EQ(NormalizeSearchPage(0), 1);
    EXPECT_EQ(NormalizeSearchPage(5), 5);
    EXPECT_FALSE(ShouldStripLeadingSlash(true, false));
    EXPECT_FALSE(ShouldStripLeadingSlash(false, false));
    EXPECT_TRUE(ShouldStripLeadingSlash(false, true));

    EXPECT_EQ(SuccessStatusCode(), 200);
    EXPECT_TRUE(IsSuccessStatus(200));
    EXPECT_FALSE(IsSuccessStatus(201));
    EXPECT_TRUE(IsSuccessfulHttpRange(200));
    EXPECT_TRUE(IsSuccessfulHttpRange(299));
    EXPECT_FALSE(IsSuccessfulHttpRange(199));
    EXPECT_FALSE(IsSuccessfulHttpRange(300));
    EXPECT_STREQ(InvalidJsonResponseMessage(), "Picacg invalid JSON response");
    EXPECT_STREQ(SearchInvalidJsonMessage(), "Picacg search invalid JSON");
}

TEST(R18PicacgAdapterAlgorithmsTest, ExposesEpisodeFallbackDefaults)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_STREQ(DefaultEpisodeTitle(), "Episode 1");
    EXPECT_EQ(DefaultEpisodeOrder(), 1);
    EXPECT_TRUE(ShouldUseFallbackEpisode(true));
    EXPECT_FALSE(ShouldUseFallbackEpisode(false));
}

TEST(R18PicacgAdapterAlgorithmsTest, ExposesImageFetchGuards)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_STREQ(ImageReferer(), "https://manhuabika.com/");
    EXPECT_STREQ(NonHttpsImageRejectedMessage(), "non-https Picacg image URL rejected");
    EXPECT_FALSE(ShouldRejectImageScheme(true));
    EXPECT_TRUE(ShouldRejectImageScheme(false));
    EXPECT_STREQ(ImageUnavailableTitle(), "Picacg image unavailable");
    EXPECT_STREQ(ImageErrorTitle(), "Picacg image error");
    EXPECT_FALSE(ShouldUseImagePlaceholder(200, false));
    EXPECT_TRUE(ShouldUseImagePlaceholder(199, false));
    EXPECT_TRUE(ShouldUseImagePlaceholder(300, false));
    EXPECT_TRUE(ShouldUseImagePlaceholder(200, true));
    EXPECT_TRUE(ShouldUseDefaultImageContentType(true));
    EXPECT_FALSE(ShouldUseDefaultImageContentType(false));
    EXPECT_STREQ(DefaultImageContentType(), "image/jpeg");
}
