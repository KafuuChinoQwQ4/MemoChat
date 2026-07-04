#include <gtest/gtest.h>

#include <cctype>
#include <string>

namespace memochat::tests::media::s3_storage
{
const char* DefaultRegion();
const char* DefaultMomentsBucket();
const char* AssetKeyPrefix();
const char* DefaultObjectContentType();
const char* EmptyNameFallback();
int MaxSanitizedNameLength();
char SanitizedReplacementChar();
bool IsAllowedS3NameChar(unsigned char c);
bool ShouldTruncateSanitizedName(unsigned long long size);
bool IsEnabledConfigValue(bool equals_true, bool equals_one);
bool IsPublicRedirectAllowed(bool equals_true, bool equals_one);
bool HasLeadingBucketSegment(bool slash_found, unsigned long long slash_pos);
} // namespace memochat::tests::media::s3_storage

namespace
{
using namespace memochat::tests::media::s3_storage;

TEST(S3MediaStorageAlgorithmsTest, LiteralConstantsPinExactValues)
{
    EXPECT_STREQ(DefaultRegion(), "us-east-1");
    EXPECT_STREQ(DefaultMomentsBucket(), "memochat-moments");
    EXPECT_STREQ(AssetKeyPrefix(), "assets/");
    EXPECT_STREQ(DefaultObjectContentType(), "application/octet-stream");
    EXPECT_STREQ(EmptyNameFallback(), "file.bin");
    EXPECT_EQ(MaxSanitizedNameLength(), 96);
    EXPECT_EQ(SanitizedReplacementChar(), '_');
}

TEST(S3MediaStorageAlgorithmsTest, IsAllowedS3NameCharMatchesLegacyPolicy)
{
    // Legacy policy was: std::isalnum(c) || c=='_' || c=='-' || c=='.'
    for (int i = 0; i < 256; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(i);
        const bool legacy = std::isalnum(c) != 0 || c == '_' || c == '-' || c == '.';
        EXPECT_EQ(IsAllowedS3NameChar(c), legacy) << "byte=" << i;
    }
}

TEST(S3MediaStorageAlgorithmsTest, ShouldTruncateSanitizedNameBoundary)
{
    EXPECT_FALSE(ShouldTruncateSanitizedName(0ULL));
    EXPECT_FALSE(ShouldTruncateSanitizedName(96ULL));
    EXPECT_TRUE(ShouldTruncateSanitizedName(97ULL));
    EXPECT_TRUE(ShouldTruncateSanitizedName(1000ULL));
}

TEST(S3MediaStorageAlgorithmsTest, IsEnabledConfigValueMatchesLegacyOr)
{
    // Legacy: (enabled_str == "true" || enabled_str == "1")
    EXPECT_TRUE(IsEnabledConfigValue(true, false));
    EXPECT_TRUE(IsEnabledConfigValue(false, true));
    EXPECT_TRUE(IsEnabledConfigValue(true, true));
    EXPECT_FALSE(IsEnabledConfigValue(false, false));
}

TEST(S3MediaStorageAlgorithmsTest, PublicRedirectRequiresExplicitOptIn)
{
    EXPECT_TRUE(IsPublicRedirectAllowed(true, false));
    EXPECT_TRUE(IsPublicRedirectAllowed(false, true));
    EXPECT_TRUE(IsPublicRedirectAllowed(true, true));
    EXPECT_FALSE(IsPublicRedirectAllowed(false, false));
}

TEST(S3MediaStorageAlgorithmsTest, HasLeadingBucketSegmentMatchesLegacyGuard)
{
    // Legacy: NOT(slash == npos || slash == 0) == (slash found && slash != 0)
    EXPECT_FALSE(HasLeadingBucketSegment(false, 0ULL)); // no slash
    EXPECT_FALSE(HasLeadingBucketSegment(true, 0ULL));  // leading slash
    EXPECT_TRUE(HasLeadingBucketSegment(true, 1ULL));   // "b/..."
    EXPECT_TRUE(HasLeadingBucketSegment(true, 7ULL));
}
} // namespace
