export module memochat.account.auth_route_registration_algorithms;

export namespace memochat::account::auth_route_registration::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* GetVarifyCodePath()
{
    return "/get_varifycode";
}

const char* UserRegisterPath()
{
    return "/user_register";
}

const char* ResetPasswordPath()
{
    return "/reset_pwd";
}

const char* UserLoginPath()
{
    return "/user_login";
}

const char* AuthRefreshPath()
{
    return "/auth/refresh";
}

const char* AuthLogoutPath()
{
    return "/auth/logout";
}
} // namespace memochat::account::auth_route_registration::modules
