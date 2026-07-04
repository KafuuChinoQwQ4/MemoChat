export module memochat.chat.postgres_pool_algorithms;

export namespace memochat::chat::persistence::postgres_pool::modules
{
bool ShouldCreateInitialConnection(int index, int requested_pool_size)
{
    return index >= 0 && index < requested_pool_size;
}

int ConnectionWaitTimeoutSeconds()
{
    return 5;
}

bool ShouldReconnect(bool stopped)
{
    return !stopped;
}

bool ShouldWakeForConnection(bool stopped, bool pool_empty)
{
    return stopped || !pool_empty;
}

bool ShouldReturnNullAfterWait(bool wait_completed, bool stopped)
{
    return !wait_completed || stopped;
}

bool HasReturnedConnection(bool has_connection)
{
    return has_connection;
}

bool ShouldAcceptReturnedConnection(bool stopped)
{
    return !stopped;
}
} // namespace memochat::chat::persistence::postgres_pool::modules
