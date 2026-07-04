import memochat.account.auth_route_registration_algorithms;

namespace memochat::tests::account::auth_route_registration
{
const char* PostMethod()
{
    return memochat::account::auth_route_registration::modules::PostMethod();
}

const char* GetVarifyCodePath()
{
    return memochat::account::auth_route_registration::modules::GetVarifyCodePath();
}

const char* UserRegisterPath()
{
    return memochat::account::auth_route_registration::modules::UserRegisterPath();
}

const char* ResetPasswordPath()
{
    return memochat::account::auth_route_registration::modules::ResetPasswordPath();
}

const char* UserLoginPath()
{
    return memochat::account::auth_route_registration::modules::UserLoginPath();
}

const char* AuthRefreshPath()
{
    return memochat::account::auth_route_registration::modules::AuthRefreshPath();
}

const char* AuthLogoutPath()
{
    return memochat::account::auth_route_registration::modules::AuthLogoutPath();
}
} // namespace memochat::tests::account::auth_route_registration
