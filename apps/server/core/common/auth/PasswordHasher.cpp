#include "auth/PasswordHasher.hpp"

#include <sodium.h>

namespace memochat::auth
{
namespace
{

bool EnsureSodiumInitialized()
{
    static const bool initialized = sodium_init() >= 0;
    return initialized;
}

} // namespace

bool HashPassword(const std::string& password, std::string& password_hash)
{
    password_hash.clear();
    if (password.empty() || !EnsureSodiumInitialized())
    {
        return false;
    }

    char hash[crypto_pwhash_STRBYTES] = {};
    if (crypto_pwhash_str(hash,
                          password.data(),
                          static_cast<unsigned long long>(password.size()),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        return false;
    }
    password_hash = hash;
    return true;
}

bool VerifyPassword(const std::string& password_hash, const std::string& password)
{
    if (!LooksLikePasswordHash(password_hash) || password.empty() || !EnsureSodiumInitialized())
    {
        return false;
    }
    return crypto_pwhash_str_verify(password_hash.c_str(),
                                    password.data(),
                                    static_cast<unsigned long long>(password.size())) == 0;
}

bool NeedsPasswordRehash(const std::string& password_hash)
{
    if (!LooksLikePasswordHash(password_hash) || !EnsureSodiumInitialized())
    {
        return false;
    }
    return crypto_pwhash_str_needs_rehash(password_hash.c_str(),
                                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0;
}

bool LooksLikePasswordHash(const std::string& password_hash)
{
    return password_hash.size() < crypto_pwhash_STRBYTES && password_hash.starts_with(crypto_pwhash_STRPREFIX);
}

} // namespace memochat::auth
