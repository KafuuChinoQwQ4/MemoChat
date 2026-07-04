export module memochat.media.local_storage_algorithms;

export namespace memochat::media::local_storage::modules
{
// Default uploads subdirectory name under the current working directory when
// no "Media/RootPath" is configured.
const char* DefaultUploadsDirName()
{
    return "uploads";
}

// Fallback object name when a sanitized name becomes empty.
const char* EmptyNameFallback()
{
    return "file.bin";
}

// Default content type when no extension mapping matches and no mime hint.
const char* DefaultContentType()
{
    return "application/octet-stream";
}

// The "uploads/" storage-path prefix that selects parent-relative resolution.
const char* UploadsPathPrefix()
{
    return "uploads/";
}

// Maximum sanitized file-name length before tail truncation.
int MaxSanitizedNameLength()
{
    return 96;
}

// Replacement character for disallowed file-name bytes.
char SanitizedReplacementChar()
{
    return '_';
}

// ASCII file-name character policy: alphanumerics plus '_', '-', '.'.
bool IsAllowedStorageNameChar(unsigned char c)
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

// Whether a value looks like an HTTP(S) URL given the two prefix probes
// (value.rfind("http://",0)==0 || value.rfind("https://",0)==0).
bool LooksLikeHttpUrl(bool has_http_prefix, bool has_https_prefix)
{
    return has_http_prefix || has_https_prefix;
}

// Whether a path segment is parent traversal.
bool IsParentTraversalSegment(bool equals_dot_dot)
{
    return equals_dot_dot;
}

// Public URL redirects are off unless an explicit current config enables them.
bool IsPublicRedirectAllowed(bool equals_true, bool equals_one)
{
    return equals_true || equals_one;
}

// Whether a lower-cased storage-provider value selects the S3/MinIO backend.
bool IsS3Provider(bool equals_s3, bool equals_minio)
{
    return equals_s3 || equals_minio;
}
} // namespace memochat::media::local_storage::modules
