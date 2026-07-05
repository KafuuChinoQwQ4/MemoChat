#include <gtest/gtest.h>

#include "auth/RefreshToken.hpp"

#include <cstdlib>
#include <optional>
#include <string>

namespace
{
class ScopedEnvVar
{
public:
    ScopedEnvVar(const char* name, const char* value)
        : _name(name)
    {
        const char* current = std::getenv(name);
        if (current != nullptr)
        {
            _previous = current;
        }
        if (value == nullptr)
        {
            unsetenv(name);
        }
        else
        {
            setenv(name, value, 1);
        }
    }

    ~ScopedEnvVar()
    {
        if (_previous)
        {
            setenv(_name.c_str(), _previous->c_str(), 1);
        }
        else
        {
            unsetenv(_name.c_str());
        }
    }

    ScopedEnvVar(const ScopedEnvVar&) = delete;
    ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;

private:
    std::string _name;
    std::optional<std::string> _previous;
};
} // namespace

TEST(RefreshTokenSecurityTest, VerifierHashRequiresServerPepper)
{
    ScopedEnvVar pepper("MEMOCHAT_AUTH_REFRESH_PEPPER", nullptr);
    ScopedEnvVar allow_dev("MEMOCHAT_ALLOW_DEV_SECRETS", nullptr);
    std::string hash;

    EXPECT_FALSE(memochat::auth::HashRefreshTokenVerifier("0123456789abcdef", hash));
    EXPECT_TRUE(hash.empty());
}

TEST(RefreshTokenSecurityTest, VerifierHashChangesWhenPepperChanges)
{
    std::string first;
    std::string second;
    {
        ScopedEnvVar pepper("MEMOCHAT_AUTH_REFRESH_PEPPER", "0123456789abcdef0123456789abcdef");
        ASSERT_TRUE(memochat::auth::HashRefreshTokenVerifier("0123456789abcdef", first));
    }
    {
        ScopedEnvVar pepper("MEMOCHAT_AUTH_REFRESH_PEPPER", "abcdef0123456789abcdef0123456789");
        ASSERT_TRUE(memochat::auth::HashRefreshTokenVerifier("0123456789abcdef", second));
    }

    EXPECT_NE(first, second);
}
