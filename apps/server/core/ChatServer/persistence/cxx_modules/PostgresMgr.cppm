export module memochat.chat.postgres_mgr_algorithms;

export namespace memochat::chat::persistence::postgres_mgr::modules
{
bool ShouldResetDao(bool dao_exists)
{
    return dao_exists;
}

bool ShouldInitializeDao(bool dao_exists)
{
    return !dao_exists;
}

const char* InitFailureEvent()
{
    return "service.postgres_init_failed";
}

const char* InitFailureMessage()
{
    return "PostgresMgr failed to initialize PostgresDao - service cannot start";
}
} // namespace memochat::chat::persistence::postgres_mgr::modules
