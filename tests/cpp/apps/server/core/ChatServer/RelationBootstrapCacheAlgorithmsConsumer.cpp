import memochat.chat.relation_bootstrap_cache_algorithms;

int MemoChatTestRelationBootstrapCacheSchemaVersion()
{
    return memochat::chat::persistence::relation_bootstrap_cache::modules::SchemaVersion();
}

bool MemoChatTestRelationBootstrapCacheValidUid(int uid)
{
    return memochat::chat::persistence::relation_bootstrap_cache::modules::IsValidUid(uid);
}

int MemoChatTestRelationBootstrapCacheTtlSec(bool has_configured_ttl,
                                             int configured_ttl,
                                             int fallback_ttl,
                                             int minimum_ttl)
{
    return memochat::chat::persistence::relation_bootstrap_cache::modules::SelectTtlSec(has_configured_ttl,
                                                                                        configured_ttl,
                                                                                        fallback_ttl,
                                                                                        minimum_ttl);
}

bool MemoChatTestRelationBootstrapCacheCurrentSchema(bool payload_is_object, int schema_version)
{
    return memochat::chat::persistence::relation_bootstrap_cache::modules::IsCurrentSchemaVersion(payload_is_object,
                                                                                                  schema_version);
}
