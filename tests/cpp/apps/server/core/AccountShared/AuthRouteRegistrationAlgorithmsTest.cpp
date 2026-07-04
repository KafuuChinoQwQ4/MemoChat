#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace memochat::tests::account::auth_route_registration
{
const char* PostMethod();
const char* GetVarifyCodePath();
const char* UserRegisterPath();
const char* ResetPasswordPath();
const char* UserLoginPath();
const char* AuthRefreshPath();
const char* AuthLogoutPath();
} // namespace memochat::tests::account::auth_route_registration

TEST(AuthRouteRegistrationAlgorithmsTest, ExposesStableHttpMethod)
{
    EXPECT_STREQ(memochat::tests::account::auth_route_registration::PostMethod(), "POST");
}

TEST(AuthRouteRegistrationAlgorithmsTest, ExposesAuthFocusedServiceRouteRegistrationPaths)
{
    using memochat::tests::account::auth_route_registration::AuthLogoutPath;
    using memochat::tests::account::auth_route_registration::AuthRefreshPath;
    using memochat::tests::account::auth_route_registration::GetVarifyCodePath;
    using memochat::tests::account::auth_route_registration::ResetPasswordPath;
    using memochat::tests::account::auth_route_registration::UserLoginPath;
    using memochat::tests::account::auth_route_registration::UserRegisterPath;

    constexpr std::array<std::string_view, 6> expected = {
        "/get_varifycode",
        "/user_register",
        "/reset_pwd",
        "/user_login",
        "/auth/refresh",
        "/auth/logout",
    };
    const std::array<std::string_view, 6> actual = {
        GetVarifyCodePath(),
        UserRegisterPath(),
        ResetPasswordPath(),
        UserLoginPath(),
        AuthRefreshPath(),
        AuthLogoutPath(),
    };

    EXPECT_EQ(actual, expected);
}
