import memochat.media.service_algorithms;

namespace memochat::tests::media::service
{
const char* FallbackFileName()
{
    return memochat::media::service::modules::FallbackFileName();
}

int MaxSanitizedFileNameLength()
{
    return memochat::media::service::modules::MaxSanitizedFileNameLength();
}

bool ShouldUseFallbackFileName(bool safe_empty)
{
    return memochat::media::service::modules::ShouldUseFallbackFileName(safe_empty);
}

bool ShouldTrimSanitizedFileName(unsigned long long safe_size)
{
    return memochat::media::service::modules::ShouldTrimSanitizedFileName(safe_size);
}

const char* DefaultContentType()
{
    return memochat::media::service::modules::DefaultContentType();
}

const char* JsonContentType()
{
    return memochat::media::service::modules::JsonContentType();
}

const char* PlainTextContentType()
{
    return memochat::media::service::modules::PlainTextContentType();
}

const char* MediaDownloadUrlPrefix()
{
    return memochat::media::service::modules::MediaDownloadUrlPrefix();
}

const char* PersistMediaFailedMessage()
{
    return memochat::media::service::modules::PersistMediaFailedMessage();
}

const char* FileNotFoundMessage()
{
    return memochat::media::service::modules::FileNotFoundMessage();
}

const char* RedirectBody()
{
    return memochat::media::service::modules::RedirectBody();
}

const char* LegacyFileDownloadDisabledMessage()
{
    return memochat::media::service::modules::LegacyFileDownloadDisabledMessage();
}

const char* EncodedPayloadTooLargeMessage()
{
    return memochat::media::service::modules::EncodedPayloadTooLargeMessage();
}

const char* RequestBodyTooLargeMessage()
{
    return memochat::media::service::modules::RequestBodyTooLargeMessage();
}

int SuccessHttpStatus()
{
    return memochat::media::service::modules::SuccessHttpStatus();
}

int RedirectHttpStatus()
{
    return memochat::media::service::modules::RedirectHttpStatus();
}

int ActiveAssetStatus()
{
    return memochat::media::service::modules::ActiveAssetStatus();
}

long long NotDeletedTimestamp()
{
    return memochat::media::service::modules::NotDeletedTimestamp();
}

bool HasValidUploadInitRequest(int uid, bool file_name_empty, long long file_size, bool token_valid)
{
    return memochat::media::service::modules::HasValidUploadInitRequest(uid, file_name_empty, file_size, token_valid);
}

bool ShouldRejectFileSize(long long file_size, long long limit)
{
    return memochat::media::service::modules::ShouldRejectFileSize(file_size, limit);
}

unsigned long long Base64EncodedLimitForDecodedBytes(unsigned long long decoded_limit)
{
    return memochat::media::service::modules::Base64EncodedLimitForDecodedBytes(decoded_limit);
}

unsigned long long JsonBase64BodyLimitForDecodedBytes(unsigned long long decoded_limit)
{
    return memochat::media::service::modules::JsonBase64BodyLimitForDecodedBytes(decoded_limit);
}

long long SimpleJsonDecodedHardLimit()
{
    return memochat::media::service::modules::SimpleJsonDecodedHardLimit();
}

long long EffectiveSimpleJsonDecodedLimit(long long configured_limit)
{
    return memochat::media::service::modules::EffectiveSimpleJsonDecodedLimit(configured_limit);
}

bool ShouldRejectRequestBodySize(unsigned long long body_size, unsigned long long decoded_limit)
{
    return memochat::media::service::modules::ShouldRejectRequestBodySize(body_size, decoded_limit);
}

bool ShouldRejectEncodedPayloadSize(unsigned long long encoded_size, unsigned long long decoded_limit)
{
    return memochat::media::service::modules::ShouldRejectEncodedPayloadSize(encoded_size, decoded_limit);
}

bool ShouldGuessMime(bool mime_empty)
{
    return memochat::media::service::modules::ShouldGuessMime(mime_empty);
}

bool IsInvalidEncodedPayload(bool encoded_empty, bool decode_ok)
{
    return memochat::media::service::modules::IsInvalidEncodedPayload(encoded_empty, decode_ok);
}

bool HasValidChunkUploadRequest(int uid, bool upload_id_empty, int index, bool binary_empty, bool token_valid)
{
    return memochat::media::service::modules::HasValidChunkUploadRequest(uid,
                                                                         upload_id_empty,
                                                                         index,
                                                                         binary_empty,
                                                                         token_valid);
}

bool IsExpiredSession(long long expires_at, long long now_ms)
{
    return memochat::media::service::modules::IsExpiredSession(expires_at, now_ms);
}

bool HasValidChunkIndex(int index, int total_chunks, int chunk_size)
{
    return memochat::media::service::modules::HasValidChunkIndex(index, total_chunks, chunk_size);
}

bool ExceedsChunkSize(int payload_size, int chunk_size)
{
    return memochat::media::service::modules::ExceedsChunkSize(payload_size, chunk_size);
}

bool HasValidStatusAuth(int uid, bool token_empty, bool upload_id_empty, bool token_valid)
{
    return memochat::media::service::modules::HasValidStatusAuth(uid, token_empty, upload_id_empty, token_valid);
}

bool HasValidCompleteAuth(int uid, bool token_empty, bool upload_id_empty, bool token_valid)
{
    return memochat::media::service::modules::HasValidCompleteAuth(uid, token_empty, upload_id_empty, token_valid);
}

bool HasValidUploadSessionShape(int total_chunks, int chunk_size, long long file_size, bool file_name_empty)
{
    return memochat::media::service::modules::HasValidUploadSessionShape(total_chunks,
                                                                         chunk_size,
                                                                         file_size,
                                                                         file_name_empty);
}

bool ShouldUseFallbackStorageMessage(bool storage_error_empty)
{
    return memochat::media::service::modules::ShouldUseFallbackStorageMessage(storage_error_empty);
}

bool HasValidSimpleUploadRequest(int uid, bool file_name_empty, bool encoded_empty, bool token_valid)
{
    return memochat::media::service::modules::HasValidSimpleUploadRequest(uid,
                                                                          file_name_empty,
                                                                          encoded_empty,
                                                                          token_valid);
}

bool ShouldRejectEmptyBinary(bool binary_empty)
{
    return memochat::media::service::modules::ShouldRejectEmptyBinary(binary_empty);
}

bool HasDownloadLocator(bool media_key_empty, bool legacy_file_empty)
{
    return memochat::media::service::modules::HasDownloadLocator(media_key_empty, legacy_file_empty);
}

bool ShouldRejectLegacyFileDownload(bool legacy_file_empty)
{
    return memochat::media::service::modules::ShouldRejectLegacyFileDownload(legacy_file_empty);
}

bool HasRequiredDownloadAuth(bool uid_raw_empty, bool token_empty)
{
    return memochat::media::service::modules::HasRequiredDownloadAuth(uid_raw_empty, token_empty);
}

bool HasValidDownloadAuth(int uid, bool token_valid)
{
    return memochat::media::service::modules::HasValidDownloadAuth(uid, token_valid);
}

bool IsReadableAsset(bool asset_loaded, int status, long long deleted_at_ms)
{
    return memochat::media::service::modules::IsReadableAsset(asset_loaded, status, deleted_at_ms);
}

bool ShouldAuditCrossOwner(int owner_uid, int uid)
{
    return memochat::media::service::modules::ShouldAuditCrossOwner(owner_uid, uid);
}

bool ShouldUseFallbackContentType(bool content_type_empty)
{
    return memochat::media::service::modules::ShouldUseFallbackContentType(content_type_empty);
}

bool ShouldRedirectToPublicUrl(bool resolved, bool public_url_empty)
{
    return memochat::media::service::modules::ShouldRedirectToPublicUrl(resolved, public_url_empty);
}

bool ShouldUseFallbackReadError(bool storage_error_empty)
{
    return memochat::media::service::modules::ShouldUseFallbackReadError(storage_error_empty);
}
} // namespace memochat::tests::media::service
