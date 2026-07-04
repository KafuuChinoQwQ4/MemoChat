export module memochat.media.s3_storage_algorithms;

export namespace memochat::media::s3_storage::modules
{
// Default MinIO/S3 region when the config value is empty.
const char* DefaultRegion()
{
    return "us-east-1";
}

// Default moments bucket when the config value is empty.
const char* DefaultMomentsBucket()
{
    return "memochat-moments";
}

// Object key prefix for stored assets: "assets/<date>/<key>_<name>".
const char* AssetKeyPrefix()
{
    return "assets/";
}

// Default content type for uploaded objects and read fallbacks.
const char* DefaultObjectContentType()
{
    return "application/octet-stream";
}

// Fallback object name when a sanitized name becomes empty.
const char* EmptyNameFallback()
{
    return "file.bin";
}

// Maximum sanitized object-name length before tail truncation.
int MaxSanitizedNameLength()
{
    return 96;
}

// Replacement character for disallowed object-name bytes.
char SanitizedReplacementChar()
{
    return '_';
}

// ASCII object-name character policy: alphanumerics plus '_', '-', '.'.
bool IsAllowedS3NameChar(unsigned char c)
{
    const bool is_digit = c >= static_cast<unsigned char>('0') && c <= static_cast<unsigned char>('9');
    const bool is_upper = c >= static_cast<unsigned char>('A') && c <= static_cast<unsigned char>('Z');
    const bool is_lower = c >= static_cast<unsigned char>('a') && c <= static_cast<unsigned char>('z');
    const bool is_symbol = c == static_cast<unsigned char>('_') || c == static_cast<unsigned char>('-') ||
                           c == static_cast<unsigned char>('.');
    return is_digit || is_upper || is_lower || is_symbol;
}

// Whether a sanitized name exceeds the max length and must be tail-truncated.
bool ShouldTruncateSanitizedName(unsigned long long size)
{
    return size > static_cast<unsigned long long>(MaxSanitizedNameLength());
}

// Whether the "Enabled" config value ("true" or "1") turns S3 storage on.
bool IsEnabledConfigValue(bool equals_true, bool equals_one)
{
    return equals_true || equals_one;
}

// Public URL redirects are off unless an explicit current config enables them.
bool IsPublicRedirectAllowed(bool equals_true, bool equals_one)
{
    return equals_true || equals_one;
}

// Whether the storage path carries a leading "<bucket>/" segment: the slash
// must exist and must not be at index 0.
bool HasLeadingBucketSegment(bool slash_found, unsigned long long slash_pos)
{
    return slash_found && slash_pos != 0ULL;
}
} // namespace memochat::media::s3_storage::modules
