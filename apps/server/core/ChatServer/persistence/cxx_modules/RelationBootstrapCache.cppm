export module memochat.chat.relation_bootstrap_cache_algorithms;

export namespace memochat::chat::persistence::relation_bootstrap_cache::modules
{
int SchemaVersion()
{
    return 2;
}

bool IsValidUid(int uid)
{
    return uid > 0;
}

int SelectTtlSec(bool has_configured_ttl, int configured_ttl, int fallback_ttl, int minimum_ttl)
{
    if (!has_configured_ttl)
    {
        return fallback_ttl;
    }
    return configured_ttl < minimum_ttl ? minimum_ttl : configured_ttl;
}

bool IsCurrentSchemaVersion(bool payload_is_object, int schema_version)
{
    return payload_is_object && schema_version == SchemaVersion();
}
} // namespace memochat::chat::persistence::relation_bootstrap_cache::modules
