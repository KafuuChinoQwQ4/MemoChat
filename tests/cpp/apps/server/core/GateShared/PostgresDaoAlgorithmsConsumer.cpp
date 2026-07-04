import memochat.gate.postgres_dao_algorithms;

namespace memochat::tests::gate::postgres_dao
{
const char* DefaultSchema()
{
    return memochat::gate::postgres_dao::modules::DefaultSchema();
}

const char* DefaultSslMode()
{
    return memochat::gate::postgres_dao::modules::DefaultSslMode();
}

int ConnectTimeoutSeconds()
{
    return memochat::gate::postgres_dao::modules::ConnectTimeoutSeconds();
}

int DefaultPoolSize()
{
    return memochat::gate::postgres_dao::modules::DefaultPoolSize();
}

int MinPoolSize()
{
    return memochat::gate::postgres_dao::modules::MinPoolSize();
}

int MaxPoolSize()
{
    return memochat::gate::postgres_dao::modules::MaxPoolSize();
}

int NormalizePoolSize(bool configured_empty, int parsed_pool_size)
{
    return memochat::gate::postgres_dao::modules::NormalizePoolSize(configured_empty, parsed_pool_size);
}

bool ShouldFetchUserProfiles(bool connection_string_empty, bool uid_list_empty)
{
    return memochat::gate::postgres_dao::modules::ShouldFetchUserProfiles(connection_string_empty, uid_list_empty);
}

bool ShouldUseAccountPostgresSection(bool account_host_empty)
{
    return memochat::gate::postgres_dao::modules::ShouldUseAccountPostgresSection(account_host_empty);
}
} // namespace memochat::tests::gate::postgres_dao
