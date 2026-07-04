export module memochat.account.auth_login_cache_profile_algorithms;

export namespace memochat::account::auth_login_cache_profile::modules
{
bool HasDecodeOutput(bool output_present)
{
    return output_present;
}

bool HasValidCacheProfileIdentity(int uid)
{
    return uid > 0;
}
} // namespace memochat::account::auth_login_cache_profile::modules
