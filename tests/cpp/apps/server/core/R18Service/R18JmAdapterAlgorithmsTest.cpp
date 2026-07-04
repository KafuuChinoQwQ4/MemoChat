#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace memochat::tests::r18::jm_adapter
{
const char* SourceId();
const char* ImageBaseUrl();
const char* ImageHost();
const char* Version();
const char* PackageName();
const char* UserAgent();
int ApiTimeoutSeconds();
int ImageTimeoutSeconds();
int MaxConcurrentImageFetches();
int SearchPageSize();
const char* ApiHostAt(int index);
int ApiHostCount();
bool ShouldTrimJsonPayload(bool start_missing, bool end_missing, bool end_before_start);
const char* InvalidDecryptedPayloadMessage();
const char* OfficialApiFailureMessage();
int NormalizeSearchPage(int page);
bool ShouldAppendSearchPage(int normalized_page);
bool ShouldUseNextSearchPage(int returned_count, int page_size);
bool ShouldStripJmPrefix(bool starts_with_jm_prefix);
int DefaultChapterOrder();
bool ShouldUseFallbackChapter(bool chapters_empty);
const char* DefaultChapterTitle();
const char* ImageUrlRejectedMessage();
bool IsAllowedImageUrl(bool scheme_is_https, bool host_matches, bool target_has_media_prefix);
const char* ImageTargetPrefix();
bool ShouldUseDefaultImageContentType(bool content_type_empty);
const char* DefaultImageContentType();
} // namespace memochat::tests::r18::jm_adapter

TEST(R18JmAdapterAlgorithmsTest, ExposesStableIdentityAndRuntimeDefaults)
{
    using namespace memochat::tests::r18::jm_adapter;

    EXPECT_STREQ(SourceId(), "jm.official");
    EXPECT_STREQ(ImageBaseUrl(), "https://cdn-msp.jmapinodeudzn.net");
    EXPECT_STREQ(ImageHost(), "cdn-msp.jmapinodeudzn.net");
    EXPECT_STREQ(Version(), "2.0.16");
    EXPECT_STREQ(PackageName(), "com.example.app");
    EXPECT_NE(std::string_view(UserAgent()).find("Mobile Safari/537.36"), std::string_view::npos);
    EXPECT_EQ(ApiTimeoutSeconds(), 6);
    EXPECT_EQ(ImageTimeoutSeconds(), 5);
    EXPECT_EQ(MaxConcurrentImageFetches(), 8);
    EXPECT_EQ(SearchPageSize(), 40);
}

TEST(R18JmAdapterAlgorithmsTest, ExposesStableApiHostRotation)
{
    using namespace memochat::tests::r18::jm_adapter;

    const std::array<std::string_view, 4> expected = {
        "www.cdnsha.org",
        "www.cdnntr.cc",
        "www.cdntwice.org",
        "www.cdnaspa.cc",
    };
    EXPECT_EQ(ApiHostCount(), static_cast<int>(expected.size()));
    for (int index = 0; index < ApiHostCount(); ++index)
        EXPECT_EQ(ApiHostAt(index), expected[static_cast<std::size_t>(index)]);
    EXPECT_STREQ(ApiHostAt(-1), "");
    EXPECT_STREQ(ApiHostAt(ApiHostCount()), "");
}

TEST(R18JmAdapterAlgorithmsTest, ExposesPayloadAndPaginationGuards)
{
    using namespace memochat::tests::r18::jm_adapter;

    EXPECT_TRUE(ShouldTrimJsonPayload(false, false, false));
    EXPECT_FALSE(ShouldTrimJsonPayload(true, false, false));
    EXPECT_FALSE(ShouldTrimJsonPayload(false, true, false));
    EXPECT_FALSE(ShouldTrimJsonPayload(false, false, true));
    EXPECT_STREQ(InvalidDecryptedPayloadMessage(), "decrypted payload is not JSON");
    EXPECT_STREQ(OfficialApiFailureMessage(), "JM official API request failed");

    EXPECT_EQ(NormalizeSearchPage(-10), 1);
    EXPECT_EQ(NormalizeSearchPage(0), 1);
    EXPECT_EQ(NormalizeSearchPage(3), 3);
    EXPECT_FALSE(ShouldAppendSearchPage(1));
    EXPECT_TRUE(ShouldAppendSearchPage(2));
    EXPECT_FALSE(ShouldUseNextSearchPage(39, SearchPageSize()));
    EXPECT_TRUE(ShouldUseNextSearchPage(40, SearchPageSize()));
}

TEST(R18JmAdapterAlgorithmsTest, ExposesChapterAndImageGuards)
{
    using namespace memochat::tests::r18::jm_adapter;

    EXPECT_TRUE(ShouldStripJmPrefix(true));
    EXPECT_FALSE(ShouldStripJmPrefix(false));
    EXPECT_EQ(DefaultChapterOrder(), 1);
    EXPECT_TRUE(ShouldUseFallbackChapter(true));
    EXPECT_FALSE(ShouldUseFallbackChapter(false));
    EXPECT_STREQ(DefaultChapterTitle(), "第1話");

    EXPECT_STREQ(ImageUrlRejectedMessage(), "image url is not an allowed JM media URL");
    EXPECT_TRUE(IsAllowedImageUrl(true, true, true));
    EXPECT_FALSE(IsAllowedImageUrl(false, true, true));
    EXPECT_FALSE(IsAllowedImageUrl(true, false, true));
    EXPECT_FALSE(IsAllowedImageUrl(true, true, false));
    EXPECT_STREQ(ImageTargetPrefix(), "/media/");
    EXPECT_TRUE(ShouldUseDefaultImageContentType(true));
    EXPECT_FALSE(ShouldUseDefaultImageContentType(false));
    EXPECT_STREQ(DefaultImageContentType(), "image/jpeg");
}
