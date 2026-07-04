import memochat.media.local_storage_algorithms;

namespace memochat::tests::media::local_storage
{
const char* DefaultUploadsDirName()
{
    return memochat::media::local_storage::modules::DefaultUploadsDirName();
}

const char* EmptyNameFallback()
{
    return memochat::media::local_storage::modules::EmptyNameFallback();
}

const char* DefaultContentType()
{
    return memochat::media::local_storage::modules::DefaultContentType();
}

const char* UploadsPathPrefix()
{
    return memochat::media::local_storage::modules::UploadsPathPrefix();
}

int MaxSanitizedNameLength()
{
    return memochat::media::local_storage::modules::MaxSanitizedNameLength();
}

char SanitizedReplacementChar()
{
    return memochat::media::local_storage::modules::SanitizedReplacementChar();
}

bool IsAllowedStorageNameChar(unsigned char c)
{
    return memochat::media::local_storage::modules::IsAllowedStorageNameChar(c);
}

bool ShouldTruncateSanitizedName(unsigned long long size)
{
    return memochat::media::local_storage::modules::ShouldTruncateSanitizedName(size);
}

bool LooksLikeHttpUrl(bool has_http_prefix, bool has_https_prefix)
{
    return memochat::media::local_storage::modules::LooksLikeHttpUrl(has_http_prefix, has_https_prefix);
}

bool IsParentTraversalSegment(bool equals_dot_dot)
{
    return memochat::media::local_storage::modules::IsParentTraversalSegment(equals_dot_dot);
}

bool IsPublicRedirectAllowed(bool equals_true, bool equals_one)
{
    return memochat::media::local_storage::modules::IsPublicRedirectAllowed(equals_true, equals_one);
}

bool IsS3Provider(bool equals_s3, bool equals_minio)
{
    return memochat::media::local_storage::modules::IsS3Provider(equals_s3, equals_minio);
}
} // namespace memochat::tests::media::local_storage
