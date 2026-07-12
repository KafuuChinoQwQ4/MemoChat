#include <gtest/gtest.h>

#include "auth/AuthSecret.hpp"

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
        if (const char* current = std::getenv(name); current != nullptr)
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

TEST(AuthSecretTest, ProductionDefaultRejectsEmptyWellKnownAndShortSecrets)
{
    ScopedEnvVar allow_dev("MEMOCHAT_ALLOW_DEV_SECRETS", nullptr);
    std::string error;

    EXPECT_FALSE(memochat::auth::RequireNonDefaultChatAuthSecretInProduction("LoginServer", "", error));
    EXPECT_NE(error.find("must be non-default"), std::string::npos);
    EXPECT_FALSE(memochat::auth::RequireNonDefaultChatAuthSecretInProduction("LoginServer",
                                                                             memochat::auth::kWellKnownDevHmacSecret,
                                                                             error));
    EXPECT_NE(error.find("must be non-default"), std::string::npos);
    EXPECT_FALSE(memochat::auth::RequireNonDefaultChatAuthSecretInProduction("LoginServer", "short", error));
    EXPECT_NE(error.find("at least 32 bytes"), std::string::npos);

    EXPECT_FALSE(memochat::auth::RequireNonDefaultJwtAccessSecretInProduction("LoginServer", "", error));
    EXPECT_NE(error.find("must be non-default"), std::string::npos);
    EXPECT_FALSE(
        memochat::auth::RequireNonDefaultJwtAccessSecretInProduction("LoginServer",
                                                                     memochat::auth::kWellKnownDevJwtAccessSecret,
                                                                     error));
    EXPECT_NE(error.find("must be non-default"), std::string::npos);
    EXPECT_FALSE(memochat::auth::RequireNonDefaultJwtAccessSecretInProduction("LoginServer", "short", error));
    EXPECT_NE(error.find("at least 32 bytes"), std::string::npos);
}

TEST(AuthSecretTest, ProductionDefaultAcceptsStrongSecrets)
{
    ScopedEnvVar allow_dev("MEMOCHAT_ALLOW_DEV_SECRETS", nullptr);
    std::string error = "stale";

    EXPECT_TRUE(memochat::auth::RequireNonDefaultChatAuthSecretInProduction("LoginServer",
                                                                            "0123456789abcdef0123456789abcdef",
                                                                            error));
    EXPECT_TRUE(error.empty());
    error = "stale";
    EXPECT_TRUE(memochat::auth::RequireNonDefaultJwtAccessSecretInProduction("LoginServer",
                                                                             "abcdef0123456789abcdef0123456789",
                                                                             error));
    EXPECT_TRUE(error.empty());
}

TEST(AuthSecretTest, ExplicitLocalOptInAllowsDevelopmentSecrets)
{
    ScopedEnvVar allow_dev("MEMOCHAT_ALLOW_DEV_SECRETS", "1");
    std::string error = "stale";

    EXPECT_TRUE(memochat::auth::RequireNonDefaultChatAuthSecretInProduction("LoginServer",
                                                                            memochat::auth::kWellKnownDevHmacSecret,
                                                                            error));
    EXPECT_TRUE(error.empty());
    error = "stale";
    EXPECT_TRUE(
        memochat::auth::RequireNonDefaultJwtAccessSecretInProduction("LoginServer",
                                                                     memochat::auth::kWellKnownDevJwtAccessSecret,
                                                                     error));
    EXPECT_TRUE(error.empty());
}
