export module memochat.account.auth_cache_algorithms;

export namespace memochat::account::auth_cache::modules
{
const char* VerificationCodePrefix()
{
    return "code_";
}

const char* HttpTokenPrefix()
{
    return "utoken_";
}

const char* LoginProfilePrefix()
{
    return "ulogin_profile_";
}

const char* LoginProfileUidPrefix()
{
    return "ulogin_profile_uid_";
}
} // namespace memochat::account::auth_cache::modules
