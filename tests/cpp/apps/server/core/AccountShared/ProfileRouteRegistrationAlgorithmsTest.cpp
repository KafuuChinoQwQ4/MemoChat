#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace memochat::tests::account::profile_route_registration
{
const char* PostMethod();
const char* UserUpdateProfilePath();
const char* GetUserInfoPath();
} // namespace memochat::tests::account::profile_route_registration

TEST(ProfileRouteRegistrationAlgorithmsTest, ExposesStableHttpMethod)
{
    EXPECT_STREQ(memochat::tests::account::profile_route_registration::PostMethod(), "POST");
}

TEST(ProfileRouteRegistrationAlgorithmsTest, ExposesProfileRouteRegistrationPaths)
{
    using memochat::tests::account::profile_route_registration::GetUserInfoPath;
    using memochat::tests::account::profile_route_registration::UserUpdateProfilePath;

    constexpr std::array<std::string_view, 2> expected = {
        "/user_update_profile",
        "/get_user_info",
    };
    const std::array<std::string_view, 2> actual = {
        UserUpdateProfilePath(),
        GetUserInfoPath(),
    };

    EXPECT_EQ(actual, expected);
}
