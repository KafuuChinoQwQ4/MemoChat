import memochat.chat.postgres_pool_algorithms;

bool MemoChatTestPostgresPoolShouldCreateInitialConnection(int index, int requested_pool_size)
{
    return memochat::chat::persistence::postgres_pool::modules::ShouldCreateInitialConnection(index,
                                                                                              requested_pool_size);
}

int MemoChatTestPostgresPoolConnectionWaitTimeoutSeconds()
{
    return memochat::chat::persistence::postgres_pool::modules::ConnectionWaitTimeoutSeconds();
}

bool MemoChatTestPostgresPoolShouldReconnect(bool stopped)
{
    return memochat::chat::persistence::postgres_pool::modules::ShouldReconnect(stopped);
}

bool MemoChatTestPostgresPoolShouldWakeForConnection(bool stopped, bool pool_empty)
{
    return memochat::chat::persistence::postgres_pool::modules::ShouldWakeForConnection(stopped, pool_empty);
}

bool MemoChatTestPostgresPoolShouldReturnNullAfterWait(bool wait_completed, bool stopped)
{
    return memochat::chat::persistence::postgres_pool::modules::ShouldReturnNullAfterWait(wait_completed, stopped);
}

bool MemoChatTestPostgresPoolHasReturnedConnection(bool has_connection)
{
    return memochat::chat::persistence::postgres_pool::modules::HasReturnedConnection(has_connection);
}

bool MemoChatTestPostgresPoolShouldAcceptReturnedConnection(bool stopped)
{
    return memochat::chat::persistence::postgres_pool::modules::ShouldAcceptReturnedConnection(stopped);
}
