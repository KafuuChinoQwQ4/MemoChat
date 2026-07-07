#include <gtest/gtest.h>

namespace memochat::tests::media::service
{
const char* FallbackFileName();
int MaxSanitizedFileNameLength();
bool ShouldUseFallbackFileName(bool safe_empty);
bool ShouldTrimSanitizedFileName(unsigned long long safe_size);
const char* DefaultContentType();
const char* JsonContentType();
const char* PlainTextContentType();
const char* MediaDownloadUrlPrefix();
const char* PersistMediaFailedMessage();
const char* FileNotFoundMessage();
const char* RedirectBody();
const char* LegacyFileDownloadDisabledMessage();
const char* EncodedPayloadTooLargeMessage();
const char* RequestBodyTooLargeMessage();
int SuccessHttpStatus();
int RedirectHttpStatus();
int ActiveAssetStatus();
long long NotDeletedTimestamp();
bool HasValidUploadInitRequest(int uid, bool file_name_empty, long long file_size, bool token_valid);
bool ShouldRejectFileSize(long long file_size, long long limit);
unsigned long long Base64EncodedLimitForDecodedBytes(unsigned long long decoded_limit);
unsigned long long JsonBase64BodyLimitForDecodedBytes(unsigned long long decoded_limit);
long long SimpleJsonDecodedHardLimit();
long long EffectiveSimpleJsonDecodedLimit(long long configured_limit);
bool ShouldRejectRequestBodySize(unsigned long long body_size, unsigned long long decoded_limit);
bool ShouldRejectEncodedPayloadSize(unsigned long long encoded_size, unsigned long long decoded_limit);
bool ShouldGuessMime(bool mime_empty);
bool IsInvalidEncodedPayload(bool encoded_empty, bool decode_ok);
bool HasValidChunkUploadRequest(int uid, bool upload_id_empty, int index, bool binary_empty, bool token_valid);
bool IsExpiredSession(long long expires_at, long long now_ms);
bool HasValidChunkIndex(int index, int total_chunks, int chunk_size);
bool ExceedsChunkSize(int payload_size, int chunk_size);
bool HasValidStatusAuth(int uid, bool token_empty, bool upload_id_empty, bool token_valid);
bool HasValidCompleteAuth(int uid, bool token_empty, bool upload_id_empty, bool token_valid);
bool HasValidUploadSessionShape(int total_chunks, int chunk_size, long long file_size, bool file_name_empty);
bool ShouldUseFallbackStorageMessage(bool storage_error_empty);
bool HasValidSimpleUploadRequest(int uid, bool file_name_empty, bool encoded_empty, bool token_valid);
bool ShouldRejectEmptyBinary(bool binary_empty);
bool HasDownloadLocator(bool media_key_empty, bool legacy_file_empty);
bool ShouldRejectLegacyFileDownload(bool legacy_file_empty);
bool HasValidDownloadAuth(int uid, bool token_valid);
bool IsReadableAsset(bool asset_loaded, int status, long long deleted_at_ms);
bool ShouldAuditCrossOwner(int owner_uid, int uid);
bool ShouldUseFallbackContentType(bool content_type_empty);
bool ShouldRedirectToPublicUrl(bool resolved, bool public_url_empty);
bool ShouldUseFallbackReadError(bool storage_error_empty);
} // namespace memochat::tests::media::service

TEST(MediaServiceAlgorithmsTest, ExposesStableResponseAndFileDefaults)
{
    using namespace memochat::tests::media::service;

    EXPECT_STREQ(FallbackFileName(), "file.bin");
    EXPECT_EQ(MaxSanitizedFileNameLength(), 96);
    EXPECT_TRUE(ShouldUseFallbackFileName(true));
    EXPECT_FALSE(ShouldUseFallbackFileName(false));
    EXPECT_FALSE(ShouldTrimSanitizedFileName(96));
    EXPECT_TRUE(ShouldTrimSanitizedFileName(97));

    EXPECT_EQ(SuccessHttpStatus(), 200);
    EXPECT_EQ(RedirectHttpStatus(), 307);
    EXPECT_EQ(ActiveAssetStatus(), 1);
    EXPECT_EQ(NotDeletedTimestamp(), 0);
    EXPECT_STREQ(DefaultContentType(), "application/octet-stream");
    EXPECT_STREQ(JsonContentType(), "text/json");
    EXPECT_STREQ(PlainTextContentType(), "text/plain");
    EXPECT_STREQ(MediaDownloadUrlPrefix(), "/media/download?asset=");
    EXPECT_STREQ(PersistMediaFailedMessage(), "persist media failed");
    EXPECT_STREQ(FileNotFoundMessage(), "file not found");
    EXPECT_STREQ(RedirectBody(), "redirecting to object storage");
    EXPECT_STREQ(LegacyFileDownloadDisabledMessage(), "legacy file download disabled; use asset");
    EXPECT_STREQ(EncodedPayloadTooLargeMessage(), "encoded payload too large");
    EXPECT_STREQ(RequestBodyTooLargeMessage(), "request body too large");
}

TEST(MediaServiceAlgorithmsTest, ExposesUploadAndChunkGuardDecisions)
{
    using namespace memochat::tests::media::service;

    EXPECT_TRUE(HasValidUploadInitRequest(1, false, 10, true));
    EXPECT_FALSE(HasValidUploadInitRequest(0, false, 10, true));
    EXPECT_FALSE(HasValidUploadInitRequest(1, true, 10, true));
    EXPECT_FALSE(HasValidUploadInitRequest(1, false, 0, true));
    EXPECT_FALSE(HasValidUploadInitRequest(1, false, 10, false));
    EXPECT_FALSE(ShouldRejectFileSize(10, 10));
    EXPECT_TRUE(ShouldRejectFileSize(11, 10));
    EXPECT_TRUE(ShouldGuessMime(true));
    EXPECT_FALSE(ShouldGuessMime(false));

    EXPECT_EQ(Base64EncodedLimitForDecodedBytes(0), 0);
    EXPECT_EQ(Base64EncodedLimitForDecodedBytes(1), 4);
    EXPECT_EQ(Base64EncodedLimitForDecodedBytes(2), 4);
    EXPECT_EQ(Base64EncodedLimitForDecodedBytes(3), 4);
    EXPECT_EQ(Base64EncodedLimitForDecodedBytes(4), 8);
    EXPECT_EQ(JsonBase64BodyLimitForDecodedBytes(3), 4ULL + 16ULL * 1024ULL);
    EXPECT_EQ(SimpleJsonDecodedHardLimit(), 64LL * 1024LL * 1024LL);
    EXPECT_EQ(EffectiveSimpleJsonDecodedLimit(10), 10);
    EXPECT_EQ(EffectiveSimpleJsonDecodedLimit(SimpleJsonDecodedHardLimit() + 1), SimpleJsonDecodedHardLimit());
    EXPECT_TRUE(ShouldRejectRequestBodySize(JsonBase64BodyLimitForDecodedBytes(3) + 1, 3));
    EXPECT_FALSE(ShouldRejectRequestBodySize(JsonBase64BodyLimitForDecodedBytes(3), 3));
    EXPECT_TRUE(ShouldRejectEncodedPayloadSize(Base64EncodedLimitForDecodedBytes(3) + 1, 3));
    EXPECT_FALSE(ShouldRejectEncodedPayloadSize(Base64EncodedLimitForDecodedBytes(3), 3));

    EXPECT_TRUE(IsInvalidEncodedPayload(true, false));
    EXPECT_TRUE(IsInvalidEncodedPayload(false, false));
    EXPECT_FALSE(IsInvalidEncodedPayload(false, true));
    EXPECT_TRUE(HasValidChunkUploadRequest(1, false, 0, false, true));
    EXPECT_FALSE(HasValidChunkUploadRequest(1, true, 0, false, true));
    EXPECT_FALSE(HasValidChunkUploadRequest(1, false, -1, false, true));
    EXPECT_FALSE(HasValidChunkUploadRequest(1, false, 0, true, true));
    EXPECT_TRUE(HasValidChunkIndex(0, 1, 1));
    EXPECT_FALSE(HasValidChunkIndex(1, 1, 1));
    EXPECT_FALSE(HasValidChunkIndex(0, 0, 1));
    EXPECT_FALSE(HasValidChunkIndex(0, 1, 0));
    EXPECT_FALSE(ExceedsChunkSize(4, 4));
    EXPECT_TRUE(ExceedsChunkSize(5, 4));
}

TEST(MediaServiceAlgorithmsTest, ExposesSessionSimpleUploadAndDownloadGuards)
{
    using namespace memochat::tests::media::service;

    EXPECT_FALSE(IsExpiredSession(0, 100));
    EXPECT_FALSE(IsExpiredSession(100, 100));
    EXPECT_TRUE(IsExpiredSession(100, 101));
    EXPECT_TRUE(HasValidStatusAuth(1, false, false, true));
    EXPECT_FALSE(HasValidStatusAuth(1, true, false, true));
    EXPECT_TRUE(HasValidCompleteAuth(1, false, false, true));
    EXPECT_FALSE(HasValidCompleteAuth(0, false, false, true));
    EXPECT_TRUE(HasValidUploadSessionShape(1, 1, 1, false));
    EXPECT_FALSE(HasValidUploadSessionShape(0, 1, 1, false));
    EXPECT_FALSE(HasValidUploadSessionShape(1, 0, 1, false));
    EXPECT_FALSE(HasValidUploadSessionShape(1, 1, 0, false));
    EXPECT_FALSE(HasValidUploadSessionShape(1, 1, 1, true));
    EXPECT_TRUE(ShouldUseFallbackStorageMessage(true));
    EXPECT_FALSE(ShouldUseFallbackStorageMessage(false));

    EXPECT_TRUE(HasValidSimpleUploadRequest(1, false, false, true));
    EXPECT_FALSE(HasValidSimpleUploadRequest(1, false, true, true));
    EXPECT_TRUE(ShouldRejectEmptyBinary(true));
    EXPECT_FALSE(ShouldRejectEmptyBinary(false));
    EXPECT_TRUE(HasDownloadLocator(false, true));
    EXPECT_FALSE(HasDownloadLocator(true, false));
    EXPECT_FALSE(HasDownloadLocator(true, true));
    EXPECT_TRUE(ShouldRejectLegacyFileDownload(false));
    EXPECT_FALSE(ShouldRejectLegacyFileDownload(true));
    EXPECT_TRUE(HasValidDownloadAuth(1, true));
    EXPECT_FALSE(HasValidDownloadAuth(0, true));
    EXPECT_FALSE(HasValidDownloadAuth(1, false));
}

TEST(MediaServiceAlgorithmsTest, ExposesDownloadAssetAndRedirectDecisions)
{
    using namespace memochat::tests::media::service;

    EXPECT_TRUE(IsReadableAsset(true, ActiveAssetStatus(), NotDeletedTimestamp()));
    EXPECT_FALSE(IsReadableAsset(false, ActiveAssetStatus(), NotDeletedTimestamp()));
    EXPECT_FALSE(IsReadableAsset(true, 0, NotDeletedTimestamp()));
    EXPECT_FALSE(IsReadableAsset(true, ActiveAssetStatus(), 1));
    EXPECT_TRUE(ShouldAuditCrossOwner(1, 2));
    EXPECT_FALSE(ShouldAuditCrossOwner(1, 1));
    EXPECT_TRUE(ShouldUseFallbackContentType(true));
    EXPECT_FALSE(ShouldUseFallbackContentType(false));
    EXPECT_TRUE(ShouldRedirectToPublicUrl(true, false));
    EXPECT_FALSE(ShouldRedirectToPublicUrl(true, true));
    EXPECT_FALSE(ShouldRedirectToPublicUrl(false, false));
    EXPECT_TRUE(ShouldUseFallbackReadError(true));
    EXPECT_FALSE(ShouldUseFallbackReadError(false));
}
