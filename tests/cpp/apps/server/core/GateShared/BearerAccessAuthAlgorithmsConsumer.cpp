import memochat.gate.bearer_access_auth_algorithms;

namespace memochat::tests::gate::auth
{

bool HeaderNameEquals(const char* lhs, unsigned long long lhs_size, const char* rhs, unsigned long long rhs_size)
{
    return memochat::gate::auth::bearer_modules::HeaderNameEquals(lhs, lhs_size, rhs, rhs_size);
}

bool TryLocateBearerToken(const char* data,
                          unsigned long long size,
                          unsigned long long& token_offset,
                          unsigned long long& token_size)
{
    return memochat::gate::auth::bearer_modules::TryLocateBearerToken(data, size, token_offset, token_size);
}

} // namespace memochat::tests::gate::auth
