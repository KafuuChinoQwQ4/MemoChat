import memochat.chat.message_repository_algorithms;

bool MemoChatTestMessageRepositoryAbortAfterPrimaryWrite(bool primary_write_success)
{
    return memochat::chat::persistence::message_repository::modules::ShouldAbortAfterPrimaryWrite(
        primary_write_success);
}

bool MemoChatTestMessageRepositoryAttemptMongo(bool mongo_enabled)
{
    return memochat::chat::persistence::message_repository::modules::ShouldAttemptMongo(mongo_enabled);
}

bool MemoChatTestMessageRepositoryHasLoadedMessage(bool query_success, bool message_present)
{
    return memochat::chat::persistence::message_repository::modules::HasLoadedMessage(query_success, message_present);
}

bool MemoChatTestMessageRepositoryReturnMongoMessage(bool mongo_success)
{
    return memochat::chat::persistence::message_repository::modules::ShouldReturnMongoMessage(mongo_success);
}

bool MemoChatTestMessageRepositoryFallbackToPostgres(bool mongo_success, bool result_empty)
{
    return memochat::chat::persistence::message_repository::modules::ShouldFallbackToPostgres(mongo_success,
                                                                                              result_empty);
}

bool MemoChatTestMessageRepositoryMergeReadSuccess(bool mongo_success, bool postgres_success)
{
    return memochat::chat::persistence::message_repository::modules::MergeReadSuccess(mongo_success, postgres_success);
}

bool MemoChatTestMessageRepositoryLogMongoWriteFailure(bool mongo_enabled, bool mongo_write_success)
{
    return memochat::chat::persistence::message_repository::modules::ShouldLogMongoWriteFailure(mongo_enabled,
                                                                                                mongo_write_success);
}
