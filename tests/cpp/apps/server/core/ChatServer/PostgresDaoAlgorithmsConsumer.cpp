import memochat.chat.postgres_dao_algorithms;

namespace postgres_dao_modules = memochat::chat::persistence::postgres_dao::modules;

bool MemoChatTestPostgresDaoUsesFallbackSection(bool primary_host_empty, bool fallback_section_empty)
{
    return postgres_dao_modules::ShouldUseFallbackSection(primary_host_empty, fallback_section_empty);
}

bool MemoChatTestPostgresDaoHasPostgresHost(bool host_empty)
{
    return postgres_dao_modules::HasPostgresHost(host_empty);
}

const char* MemoChatTestPostgresDaoDefaultSslMode()
{
    return postgres_dao_modules::DefaultSslMode();
}

const char* MemoChatTestPostgresDaoDefaultSchema()
{
    return postgres_dao_modules::DefaultSchema();
}

const char* MemoChatTestPostgresDaoSelectSslMode(bool sslmode_empty, const char* sslmode)
{
    return postgres_dao_modules::SelectSslMode(sslmode_empty, sslmode);
}

const char* MemoChatTestPostgresDaoSelectSchema(bool schema_empty, const char* schema)
{
    return postgres_dao_modules::SelectSchema(schema_empty, schema);
}

int MemoChatTestPostgresDaoConnectTimeoutSeconds()
{
    return postgres_dao_modules::ConnectTimeoutSeconds();
}

int MemoChatTestPostgresDaoStatementTimeoutMilliseconds()
{
    return postgres_dao_modules::StatementTimeoutMilliseconds();
}

int MemoChatTestPostgresDaoHealthProbeIntervalMilliseconds()
{
    return postgres_dao_modules::HealthProbeIntervalMilliseconds();
}

int MemoChatTestPostgresDaoTcpUserTimeoutMilliseconds()
{
    return postgres_dao_modules::TcpUserTimeoutMilliseconds();
}

int MemoChatTestPostgresDaoHealthProbeDeadlineMilliseconds()
{
    return postgres_dao_modules::HealthProbeDeadlineMilliseconds();
}

const char* MemoChatTestPostgresDaoHealthProbeSessionOptions()
{
    return postgres_dao_modules::HealthProbeSessionOptions();
}
