export module memochat.account.auth_route_schema_algorithms;

export namespace memochat::account::auth_route_schema::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* RegisterPath()
{
    return "/user_register";
}

const char* RegisterRouteName()
{
    return "auth.user.register";
}

const char* RegisterRequestTypeName()
{
    return "AuthRegisterRequestDto";
}

const char* RegisterResponseTypeName()
{
    return "AuthRegisterResponseDto";
}

const char* ResetPasswordPath()
{
    return "/reset_pwd";
}

const char* ResetPasswordRouteName()
{
    return "auth.reset.password";
}

const char* ResetPasswordRequestTypeName()
{
    return "AuthResetPasswordRequestDto";
}

const char* ResetPasswordResponseTypeName()
{
    return "AuthResetPasswordResponseDto";
}

const char* LoginPath()
{
    return "/user_login";
}

const char* LoginRouteName()
{
    return "auth.user.login";
}

const char* LoginRequestTypeName()
{
    return "AuthLoginRequestDto";
}

const char* LoginResponseTypeName()
{
    return "AuthLoginResponseDto";
}

const char* RefreshPath()
{
    return "/auth/refresh";
}

const char* RefreshRouteName()
{
    return "auth.refresh";
}

const char* RefreshRequestTypeName()
{
    return "AuthRefreshRequestDto";
}

const char* RefreshResponseTypeName()
{
    return "AuthLoginResponseDto";
}

const char* LogoutPath()
{
    return "/auth/logout";
}

const char* LogoutRouteName()
{
    return "auth.logout";
}

const char* LogoutRequestTypeName()
{
    return "AuthLogoutRequestDto";
}

const char* LogoutResponseTypeName()
{
    return "AuthLogoutResponseDto";
}
} // namespace memochat::account::auth_route_schema::modules
