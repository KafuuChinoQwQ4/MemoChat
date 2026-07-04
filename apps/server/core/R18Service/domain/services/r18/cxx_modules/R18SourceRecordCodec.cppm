export module memochat.r18.source_record_codec_algorithms;

export namespace memochat::r18::source_record_codec::modules
{
bool ShouldUseFallbackString(bool value_empty)
{
    return value_empty;
}

bool HasDecodeOutput(bool output_present)
{
    return output_present;
}

bool HasRequiredSourceRecordIdentity(bool id_empty)
{
    return !id_empty;
}
} // namespace memochat::r18::source_record_codec::modules
