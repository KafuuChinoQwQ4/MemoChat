export module memochat.chat.postgres_dao_algorithms;

export namespace memochat::chat::persistence::postgres_dao::modules
{
bool ShouldUseFallbackSection(bool primary_host_empty, bool fallback_section_empty)
{
    return primary_host_empty && !fallback_section_empty;
}

bool HasPostgresHost(bool host_empty)
{
    return !host_empty;
}

const char* DefaultSslMode()
{
    return "disable";
}

const char* DefaultSchema()
{
    return "public";
}

const char* SelectSslMode(bool sslmode_empty, const char* sslmode)
{
    return sslmode_empty || sslmode == nullptr ? DefaultSslMode() : sslmode;
}

const char* SelectSchema(bool schema_empty, const char* schema)
{
    return schema_empty || schema == nullptr ? DefaultSchema() : schema;
}

} // namespace memochat::chat::persistence::postgres_dao::modules
