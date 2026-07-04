export module memochat.media.service_algorithms;

export namespace memochat::media::service::modules
{
const char* FallbackFileName()
{
    return "file.bin";
}

int MaxSanitizedFileNameLength()
{
    return 96;
}

bool ShouldUseFallbackFileName(bool safe_empty)
{
    return safe_empty;
}

bool ShouldTrimSanitizedFileName(unsigned long long safe_size)
{
    return safe_size > static_cast<unsigned long long>(MaxSanitizedFileNameLength());
}

const char* DefaultContentType()
{
    return "application/octet-stream";
}

const char* JsonContentType()
{
    return "text/json";
}

const char* PlainTextContentType()
{
    return "text/plain";
}

const char* MediaDownloadUrlPrefix()
{
    return "/media/download?asset=";
}

const char* PersistMediaFailedMessage()
{
    return "persist media failed";
}

const char* FileNotFoundMessage()
{
    return "file not found";
}

const char* RedirectBody()
{
    return "redirecting to object storage";
}

const char* LegacyFileDownloadDisabledMessage()
{
    return "legacy file download disabled; use asset";
}

const char* EncodedPayloadTooLargeMessage()
{
    return "encoded payload too large";
}

const char* RequestBodyTooLargeMessage()
{
    return "request body too large";
}

int SuccessHttpStatus()
{
    return 200;
}

int RedirectHttpStatus()
{
    return 307;
}

int ActiveAssetStatus()
{
    return 1;
}

long long NotDeletedTimestamp()
{
    return 0;
}

bool HasValidUploadInitRequest(int uid, bool file_name_empty, long long file_size, bool token_valid)
{
    return uid > 0 && !file_name_empty && file_size > 0 && token_valid;
}

bool ShouldRejectFileSize(long long file_size, long long limit)
{
    return file_size > limit;
}

unsigned long long Base64EncodedLimitForDecodedBytes(unsigned long long decoded_limit)
{
    return ((decoded_limit + 2ULL) / 3ULL) * 4ULL;
}

unsigned long long JsonBase64BodyLimitForDecodedBytes(unsigned long long decoded_limit)
{
    return Base64EncodedLimitForDecodedBytes(decoded_limit) + 16ULL * 1024ULL;
}

long long SimpleJsonDecodedHardLimit()
{
    return 64LL * 1024LL * 1024LL;
}

long long EffectiveSimpleJsonDecodedLimit(long long configured_limit)
{
    if (configured_limit <= 0)
    {
        return SimpleJsonDecodedHardLimit();
    }
    return configured_limit < SimpleJsonDecodedHardLimit() ? configured_limit : SimpleJsonDecodedHardLimit();
}

bool ShouldRejectRequestBodySize(unsigned long long body_size, unsigned long long decoded_limit)
{
    return body_size > JsonBase64BodyLimitForDecodedBytes(decoded_limit);
}

bool ShouldRejectEncodedPayloadSize(unsigned long long encoded_size, unsigned long long decoded_limit)
{
    return encoded_size > Base64EncodedLimitForDecodedBytes(decoded_limit);
}

bool ShouldGuessMime(bool mime_empty)
{
    return mime_empty;
}

bool IsInvalidEncodedPayload(bool encoded_empty, bool decode_ok)
{
    return encoded_empty || !decode_ok;
}

bool HasValidChunkUploadRequest(int uid, bool upload_id_empty, int index, bool binary_empty, bool token_valid)
{
    return uid > 0 && !upload_id_empty && index >= 0 && !binary_empty && token_valid;
}

bool IsExpiredSession(long long expires_at, long long now_ms)
{
    return expires_at > 0 && now_ms > expires_at;
}

bool HasValidChunkIndex(int index, int total_chunks, int chunk_size)
{
    return index < total_chunks && total_chunks > 0 && chunk_size > 0;
}

bool ExceedsChunkSize(int payload_size, int chunk_size)
{
    return payload_size > chunk_size;
}

bool HasValidStatusAuth(int uid, bool token_empty, bool upload_id_empty, bool token_valid)
{
    return uid > 0 && !token_empty && !upload_id_empty && token_valid;
}

bool HasValidCompleteAuth(int uid, bool token_empty, bool upload_id_empty, bool token_valid)
{
    return uid > 0 && !token_empty && !upload_id_empty && token_valid;
}

bool HasValidUploadSessionShape(int total_chunks, int chunk_size, long long file_size, bool file_name_empty)
{
    return total_chunks > 0 && chunk_size > 0 && file_size > 0 && !file_name_empty;
}

bool ShouldUseFallbackStorageMessage(bool storage_error_empty)
{
    return storage_error_empty;
}

bool HasValidSimpleUploadRequest(int uid, bool file_name_empty, bool encoded_empty, bool token_valid)
{
    return uid > 0 && !file_name_empty && !encoded_empty && token_valid;
}

bool ShouldRejectEmptyBinary(bool binary_empty)
{
    return binary_empty;
}

bool HasDownloadLocator(bool media_key_empty, bool legacy_file_empty)
{
    (void) legacy_file_empty;
    return !media_key_empty;
}

bool ShouldRejectLegacyFileDownload(bool legacy_file_empty)
{
    return !legacy_file_empty;
}

bool HasRequiredDownloadAuth(bool uid_raw_empty, bool token_empty)
{
    return !uid_raw_empty && !token_empty;
}

bool HasValidDownloadAuth(int uid, bool token_valid)
{
    return uid > 0 && token_valid;
}

bool IsReadableAsset(bool asset_loaded, int status, long long deleted_at_ms)
{
    return asset_loaded && status == ActiveAssetStatus() && deleted_at_ms <= NotDeletedTimestamp();
}

bool ShouldAuditCrossOwner(int owner_uid, int uid)
{
    return owner_uid != uid;
}

bool ShouldUseFallbackContentType(bool content_type_empty)
{
    return content_type_empty;
}

bool ShouldRedirectToPublicUrl(bool resolved, bool public_url_empty)
{
    return resolved && !public_url_empty;
}

bool ShouldUseFallbackReadError(bool storage_error_empty)
{
    return storage_error_empty;
}
} // namespace memochat::media::service::modules
