import memochat.media.s3_storage_algorithms;

namespace memochat::tests::media::s3_storage
{
const char* DefaultRegion()
{
    return memochat::media::s3_storage::modules::DefaultRegion();
}

const char* DefaultMomentsBucket()
{
    return memochat::media::s3_storage::modules::DefaultMomentsBucket();
}

const char* AssetKeyPrefix()
{
    return memochat::media::s3_storage::modules::AssetKeyPrefix();
}

const char* DefaultObjectContentType()
{
    return memochat::media::s3_storage::modules::DefaultObjectContentType();
}

const char* EmptyNameFallback()
{
    return memochat::media::s3_storage::modules::EmptyNameFallback();
}

int MaxSanitizedNameLength()
{
    return memochat::media::s3_storage::modules::MaxSanitizedNameLength();
}

char SanitizedReplacementChar()
{
    return memochat::media::s3_storage::modules::SanitizedReplacementChar();
}

bool IsAllowedS3NameChar(unsigned char c)
{
    return memochat::media::s3_storage::modules::IsAllowedS3NameChar(c);
}

bool ShouldTruncateSanitizedName(unsigned long long size)
{
    return memochat::media::s3_storage::modules::ShouldTruncateSanitizedName(size);
}

bool IsEnabledConfigValue(bool equals_true, bool equals_one)
{
    return memochat::media::s3_storage::modules::IsEnabledConfigValue(equals_true, equals_one);
}

bool IsPublicRedirectAllowed(bool equals_true, bool equals_one)
{
    return memochat::media::s3_storage::modules::IsPublicRedirectAllowed(equals_true, equals_one);
}

bool HasLeadingBucketSegment(bool slash_found, unsigned long long slash_pos)
{
    return memochat::media::s3_storage::modules::HasLeadingBucketSegment(slash_found, slash_pos);
}
} // namespace memochat::tests::media::s3_storage
