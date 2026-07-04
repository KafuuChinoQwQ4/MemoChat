#include <gtest/gtest.h>

#include <cctype>

namespace memochat::tests::media::local_storage
{
const char* DefaultUploadsDirName();
const char* EmptyNameFallback();
const char* DefaultContentType();
const char* UploadsPathPrefix();
int MaxSanitizedNameLength();
char SanitizedReplacementChar();
bool IsAllowedStorageNameChar(unsigned char c);
bool ShouldTruncateSanitizedName(unsigned long long size);
bool LooksLikeHttpUrl(bool has_http_prefix, bool has_https_prefix);
bool IsParentTraversalSegment(bool equals_dot_dot);
bool IsPublicRedirectAllowed(bool equals_true, bool equals_one);
bool IsS3Provider(bool equals_s3, bool equals_minio);
} // namespace memochat::tests::media::local_storage

namespace
{
using namespace memochat::tests::media::local_storage;

TEST(LocalMediaStorageAlgorithmsTest, LiteralConstantsPinExactValues)
{
    EXPECT_STREQ(DefaultUploadsDirName(), "uploads");
    EXPECT_STREQ(EmptyNameFallback(), "file.bin");
    EXPECT_STREQ(DefaultContentType(), "application/octet-stream");
    EXPECT_STREQ(UploadsPathPrefix(), "uploads/");
    EXPECT_EQ(MaxSanitizedNameLength(), 96);
    EXPECT_EQ(SanitizedReplacementChar(), '_');
}

TEST(LocalMediaStorageAlgorithmsTest, IsAllowedStorageNameCharMatchesLegacyPolicy)
{
    // Legacy: std::isalnum(c) || c=='_' || c=='-' || c=='.'
    for (int i = 0; i < 256; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(i);
        const bool legacy = std::isalnum(c) != 0 || c == '_' || c == '-' || c == '.';
        EXPECT_EQ(IsAllowedStorageNameChar(c), legacy) << "byte=" << i;
    }
}

TEST(LocalMediaStorageAlgorithmsTest, ShouldTruncateSanitizedNameBoundary)
{
    EXPECT_FALSE(ShouldTruncateSanitizedName(0ULL));
    EXPECT_FALSE(ShouldTruncateSanitizedName(96ULL));
    EXPECT_TRUE(ShouldTruncateSanitizedName(97ULL));
}

TEST(LocalMediaStorageAlgorithmsTest, LooksLikeHttpUrlMatchesLegacyOr)
{
    EXPECT_TRUE(LooksLikeHttpUrl(true, false));
    EXPECT_TRUE(LooksLikeHttpUrl(false, true));
    EXPECT_TRUE(LooksLikeHttpUrl(true, true));
    EXPECT_FALSE(LooksLikeHttpUrl(false, false));
}

TEST(LocalMediaStorageAlgorithmsTest, SecurityDecisionHelpersRequireExplicitSafeInputs)
{
    EXPECT_TRUE(IsParentTraversalSegment(true));
    EXPECT_FALSE(IsParentTraversalSegment(false));

    EXPECT_TRUE(IsPublicRedirectAllowed(true, false));
    EXPECT_TRUE(IsPublicRedirectAllowed(false, true));
    EXPECT_TRUE(IsPublicRedirectAllowed(true, true));
    EXPECT_FALSE(IsPublicRedirectAllowed(false, false));
}

TEST(LocalMediaStorageAlgorithmsTest, IsS3ProviderMatchesLegacyOr)
{
    EXPECT_TRUE(IsS3Provider(true, false));
    EXPECT_TRUE(IsS3Provider(false, true));
    EXPECT_TRUE(IsS3Provider(true, true));
    EXPECT_FALSE(IsS3Provider(false, false));
}
} // namespace
