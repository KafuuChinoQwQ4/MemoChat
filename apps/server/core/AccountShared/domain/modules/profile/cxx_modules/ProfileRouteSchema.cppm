export module memochat.account.profile_route_schema_algorithms;

export namespace memochat::account::profile_route_schema::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* UpdateProfilePath()
{
    return "/user_update_profile";
}

const char* UpdateProfileRouteName()
{
    return "profile.user.update";
}

const char* UpdateProfileRequestTypeName()
{
    return "ProfileUpdateRequestDto";
}

const char* UpdateProfileResponseTypeName()
{
    return "ProfileUpdateResponseDto";
}

const char* GetUserInfoPath()
{
    return "/get_user_info";
}

const char* GetUserInfoRouteName()
{
    return "profile.user.info";
}

const char* GetUserInfoRequestTypeName()
{
    return "GetUserInfoRequestDto";
}

const char* UserInfoResponseTypeName()
{
    return "UserInfoResponseDto";
}
} // namespace memochat::account::profile_route_schema::modules
