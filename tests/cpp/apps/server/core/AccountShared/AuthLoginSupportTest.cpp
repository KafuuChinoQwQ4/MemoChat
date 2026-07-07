#include "AuthLoginSupport.hpp"
#include "AuthLoginRateLimiter.hpp"

#include <gtest/gtest.h>

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

TEST(AuthLoginSupportTest, ClientVersionGateUsesImportedSemverAlgorithms)
{
    EXPECT_FALSE(gateauthsupport::IsClientVersionAllowed("", "2.0.0"));
    EXPECT_TRUE(gateauthsupport::IsClientVersionAllowed("2.0", "2.0.0"));
    EXPECT_TRUE(gateauthsupport::IsClientVersionAllowed("2.0.1", "2.0.0"));
    EXPECT_TRUE(gateauthsupport::IsClientVersionAllowed("2.1.0", "2.0.9"));
    EXPECT_TRUE(gateauthsupport::IsClientVersionAllowed("3.0.0", "2.9.9"));

    EXPECT_FALSE(gateauthsupport::IsClientVersionAllowed("1.9.9", "2.0.0"));
    EXPECT_FALSE(gateauthsupport::IsClientVersionAllowed("2.0.0-beta", "2.0.0"));
    EXPECT_FALSE(gateauthsupport::IsClientVersionAllowed("2..0", "2.0.0"));
    EXPECT_FALSE(gateauthsupport::IsClientVersionAllowed("999999999999999999999.0.0", "2.0.0"));
}

TEST(AuthLoginSupportTest, InvalidMinimumVersionFallsBackToAllow)
{
    EXPECT_TRUE(gateauthsupport::IsClientVersionAllowed("1.0.0", "not-a-version"));
}

TEST(AuthLoginSupportTest, ChatTicketTtlIsClampedToFiveMinutes)
{
    ScopedEnvVar ttl("MEMOCHAT_CHATAUTH_TICKETTTLSEC", "999999999");

    EXPECT_EQ(gateauthsupport::GetChatTicketTtlSec(), 300);
}

TEST(AuthLoginSupportTest, AccessTokenTtlIsShortAndConfigurable)
{
    ScopedEnvVar ttl("MEMOCHAT_AUTHTOKEN_ACCESSTOKENTTLSEC", "999999999");

    EXPECT_EQ(gateauthsupport::GetAccessTokenTtlSec(), 3600);
}

TEST(AuthLoginSupportTest, JwtAccessTokenConfigUsesEnvOverrides)
{
    ScopedEnvVar secret("MEMOCHAT_AUTHTOKEN_JWTSECRET", "abcdefghijklmnopqrstuvwxyz123456");
    ScopedEnvVar issuer("MEMOCHAT_AUTHTOKEN_JWTISSUER", "memochat-test");
    ScopedEnvVar audience("MEMOCHAT_AUTHTOKEN_JWTAUDIENCE", "memochat-test-http");

    EXPECT_EQ(gateauthsupport::GetJwtAccessSecret(), "abcdefghijklmnopqrstuvwxyz123456");
    EXPECT_EQ(gateauthsupport::GetJwtAccessIssuer(), "memochat-test");
    EXPECT_EQ(gateauthsupport::GetJwtAccessAudience(), "memochat-test-http");
}

TEST(AuthLoginSupportTest, WebTransportEndpointRequiresRuntimeProviderAndAdvertiseGates)
{
    ScopedEnvVar enable_wt("MEMOCHAT_ENABLE_WEBTRANSPORT", nullptr);
    ScopedEnvVar enable_provider("MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER", nullptr);
    ScopedEnvVar advertise_wt("MEMOCHAT_ADVERTISE_WEBTRANSPORT", nullptr);

    EXPECT_FALSE(gateauthsupport::ShouldAdvertiseWebTransportEndpoint(false));
    EXPECT_FALSE(gateauthsupport::ShouldAdvertiseWebTransportEndpoint(true));
}

TEST(AuthLoginSupportTest, WebTransportEndpointCanBeRuntimeEnabledWithoutChangingDefaultConfig)
{
    ScopedEnvVar enable_wt("MEMOCHAT_ENABLE_WEBTRANSPORT", "1");
    ScopedEnvVar enable_provider("MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER", "1");
    ScopedEnvVar advertise_wt("MEMOCHAT_ADVERTISE_WEBTRANSPORT", "1");

    EXPECT_TRUE(gateauthsupport::IsWebTransportRuntimeEnabled());
    EXPECT_TRUE(gateauthsupport::IsWebTransportProviderRuntimeEnabled());
    EXPECT_TRUE(gateauthsupport::IsWebTransportAdvertiseEnabled());
    EXPECT_TRUE(gateauthsupport::ShouldAdvertiseWebTransportEndpoint(false));
    EXPECT_TRUE(gateauthsupport::ShouldAdvertiseWebTransportEndpoint(true));
}

TEST(AuthLoginSupportTest, WebTransportEndpointDoesNotAdvertiseWithoutProviderGate)
{
    ScopedEnvVar enable_wt("MEMOCHAT_ENABLE_WEBTRANSPORT", "1");
    ScopedEnvVar enable_provider("MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER", nullptr);
    ScopedEnvVar advertise_wt("MEMOCHAT_ADVERTISE_WEBTRANSPORT", "1");

    EXPECT_FALSE(gateauthsupport::ShouldAdvertiseWebTransportEndpoint(false));
    EXPECT_FALSE(gateauthsupport::ShouldAdvertiseWebTransportEndpoint(true));
}

TEST(AuthLoginSupportTest, WebTransportEndpointDoesNotAdvertiseWithoutExplicitAdvertiseGate)
{
    ScopedEnvVar enable_wt("MEMOCHAT_ENABLE_WEBTRANSPORT", "1");
    ScopedEnvVar enable_provider("MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER", "1");
    ScopedEnvVar advertise_wt("MEMOCHAT_ADVERTISE_WEBTRANSPORT", nullptr);

    EXPECT_FALSE(gateauthsupport::ShouldAdvertiseWebTransportEndpoint(false));
    EXPECT_FALSE(gateauthsupport::ShouldAdvertiseWebTransportEndpoint(true));
}

TEST(AuthLoginSupportTest, AuthRequestRateLimitKeysNormalizeExpectedBuckets)
{
    EXPECT_EQ(gateauthsupport::BuildAuthRateLimitKey(gateauthsupport::AuthRateLimitAction::GetVarifyCode,
                                                     gateauthsupport::AuthRateLimitBucket::Email,
                                                     " Alice@Example.COM "),
              "auth_req:get_varifycode:email:alice@example.com");
    EXPECT_EQ(gateauthsupport::BuildAuthRateLimitKey(gateauthsupport::AuthRateLimitAction::Register,
                                                     gateauthsupport::AuthRateLimitBucket::Ip,
                                                     "203.0.113.10"),
              "auth_req:register:ip:203.0.113.10");
    EXPECT_EQ(gateauthsupport::BuildAuthRateLimitKey(gateauthsupport::AuthRateLimitAction::AuthRefresh,
                                                     gateauthsupport::AuthRateLimitBucket::Subject,
                                                     "abcdef"),
              "auth_req:auth_refresh:subject:abcdef");
}

TEST(AuthLoginSupportTest, RedisFailureBehaviorDefaultsToFailClosedUnlessExplicitlyBoundedOpen)
{
    EXPECT_EQ(gateauthsupport::ParseAuthRateLimitRedisFailureBehavior(""),
              gateauthsupport::AuthRateLimitRedisFailureBehavior::FailClosed);
    EXPECT_EQ(gateauthsupport::ParseAuthRateLimitRedisFailureBehavior("fail_open"),
              gateauthsupport::AuthRateLimitRedisFailureBehavior::FailClosed);
    EXPECT_EQ(gateauthsupport::ParseAuthRateLimitRedisFailureBehavior("bounded_fail_open"),
              gateauthsupport::AuthRateLimitRedisFailureBehavior::BoundedFailOpen);
    EXPECT_EQ(gateauthsupport::ParseAuthRateLimitRedisFailureBehavior(" BOUNDED-OPEN "),
              gateauthsupport::AuthRateLimitRedisFailureBehavior::BoundedFailOpen);
}
