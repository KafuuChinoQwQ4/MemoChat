export module memochat.account.profile_route_registration_algorithms;

export namespace memochat::account::profile_route_registration::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* UserUpdateProfilePath()
{
    return "/user_update_profile";
}

const char* GetUserInfoPath()
{
    return "/get_user_info";
}
} // namespace memochat::account::profile_route_registration::modules
