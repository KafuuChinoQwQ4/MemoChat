#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace memochat::tests::gate::auth
{
bool HeaderNameEquals(const char* lhs, unsigned long long lhs_size, const char* rhs, unsigned long long rhs_size);
bool TryLocateBearerToken(const char* data,
                          unsigned long long size,
                          unsigned long long& token_offset,
                          unsigned long long& token_size);
} // namespace memochat::tests::gate::auth

namespace
{
bool Locate(std::string_view value, std::string& token)
{
    unsigned long long offset = 0;
    unsigned long long size = 0;
    if (!memochat::tests::gate::auth::TryLocateBearerToken(value.data(), value.size(), offset, size))
    {
        token.clear();
        return false;
    }
    token = std::string(value.substr(static_cast<std::size_t>(offset), static_cast<std::size_t>(size)));
    return true;
}
} // namespace

TEST(BearerAccessAuthAlgorithmsTest, MatchesAuthorizationHeaderNameCaseInsensitively)
{
    EXPECT_TRUE(memochat::tests::gate::auth::HeaderNameEquals("Authorization", 13, "authorization", 13));
    EXPECT_TRUE(memochat::tests::gate::auth::HeaderNameEquals("AUTHORIZATION", 13, "authorization", 13));
    EXPECT_FALSE(memochat::tests::gate::auth::HeaderNameEquals("X-Authorization", 15, "authorization", 13));
}

TEST(BearerAccessAuthAlgorithmsTest, ExtractsOnlyBearerScheme)
{
    std::string token;
    EXPECT_TRUE(Locate("Bearer access.jwt.token", token));
    EXPECT_EQ(token, "access.jwt.token");

    EXPECT_TRUE(Locate("  bearer\tabc.def.ghi  ", token));
    EXPECT_EQ(token, "abc.def.ghi");
}

TEST(BearerAccessAuthAlgorithmsTest, RejectsMissingOrLegacyTokenSchemes)
{
    std::string token;
    EXPECT_FALSE(Locate("", token));
    EXPECT_FALSE(Locate("Bearer   ", token));
    EXPECT_FALSE(Locate("Token access.jwt.token", token));
    EXPECT_FALSE(Locate("access.jwt.token", token));
}
