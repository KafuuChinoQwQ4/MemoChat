#include "AuthLoginSupport.hpp"

#include <gtest/gtest.h>

TEST(AuthLoginSupportTest, ClientVersionGateUsesImportedSemverAlgorithms)
{
    EXPECT_TRUE(gateauthsupport::IsClientVersionAllowed("", "2.0.0"));
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
