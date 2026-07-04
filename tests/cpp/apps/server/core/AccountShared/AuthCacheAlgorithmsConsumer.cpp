import memochat.account.auth_cache_algorithms;

namespace memochat::tests::account::auth_cache
{
const char* VerificationCodePrefix()
{
    return memochat::account::auth_cache::modules::VerificationCodePrefix();
}

const char* HttpTokenPrefix()
{
    return memochat::account::auth_cache::modules::HttpTokenPrefix();
}

const char* LoginProfilePrefix()
{
    return memochat::account::auth_cache::modules::LoginProfilePrefix();
}

const char* LoginProfileUidPrefix()
{
    return memochat::account::auth_cache::modules::LoginProfileUidPrefix();
}
} // namespace memochat::tests::account::auth_cache
