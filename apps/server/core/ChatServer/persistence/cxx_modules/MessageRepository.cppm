export module memochat.chat.message_repository_algorithms;

export namespace memochat::chat::persistence::message_repository::modules
{
bool ShouldAbortAfterPrimaryWrite(bool primary_write_success)
{
    return !primary_write_success;
}

bool ShouldAttemptMongo(bool mongo_enabled)
{
    return mongo_enabled;
}

bool HasLoadedMessage(bool query_success, bool message_present)
{
    return query_success && message_present;
}

bool ShouldReturnMongoMessage(bool mongo_success)
{
    return mongo_success;
}

bool ShouldFallbackToPostgres(bool mongo_success, bool result_empty)
{
    return !mongo_success || result_empty;
}

bool MergeReadSuccess(bool mongo_success, bool postgres_success)
{
    return mongo_success || postgres_success;
}

bool ShouldLogMongoWriteFailure(bool mongo_enabled, bool mongo_write_success)
{
    return mongo_enabled && !mongo_write_success;
}
} // namespace memochat::chat::persistence::message_repository::modules
