export module memochat.media.upload_session_algorithms;

export namespace memochat::media::upload_session::modules
{
bool ShouldUseDefaultMediaType(bool media_type_empty)
{
    return media_type_empty;
}

bool ShouldUseDefaultStorageProvider(bool storage_provider_empty)
{
    return storage_provider_empty;
}

bool HasValidUploadSessionShape(bool uid_positive,
                                bool upload_id_present,
                                bool file_name_present,
                                bool file_size_positive,
                                bool chunk_size_positive,
                                bool total_chunks_positive)
{
    return uid_positive && upload_id_present && file_name_present && file_size_positive && chunk_size_positive &&
           total_chunks_positive;
}
} // namespace memochat::media::upload_session::modules
