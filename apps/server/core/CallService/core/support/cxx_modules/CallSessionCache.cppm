export module memochat.call.session_cache_algorithms;

export namespace memochat::call::session_cache::modules
{
bool HasValidCacheIdentity(bool call_id_present)
{
    return call_id_present;
}
} // namespace memochat::call::session_cache::modules
