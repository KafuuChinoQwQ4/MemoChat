export module memochat.gate.postgres_dao_algorithms;

export namespace memochat::gate::postgres_dao::modules
{
const char* DefaultSchema()
{
    return "public";
}

const char* DefaultSslMode()
{
    return "disable";
}

int ConnectTimeoutSeconds()
{
    return 3;
}

int DefaultPoolSize()
{
    return 12;
}

int MinPoolSize()
{
    return 1;
}

int MaxPoolSize()
{
    return 64;
}

int NormalizePoolSize(bool configured_empty, int parsed_pool_size)
{
    if (configured_empty)
    {
        return DefaultPoolSize();
    }
    if (parsed_pool_size < MinPoolSize())
    {
        return MinPoolSize();
    }
    if (parsed_pool_size > MaxPoolSize())
    {
        return MaxPoolSize();
    }
    return parsed_pool_size;
}

bool ShouldFetchUserProfiles(bool connection_string_empty, bool uid_list_empty)
{
    return !connection_string_empty && !uid_list_empty;
}

bool ShouldUseAccountPostgresSection(bool account_host_empty)
{
    return !account_host_empty;
}

} // namespace memochat::gate::postgres_dao::modules
