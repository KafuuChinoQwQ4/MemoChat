import memochat.account.profile_route_registration_algorithms;

namespace memochat::tests::account::profile_route_registration
{
const char* PostMethod()
{
    return memochat::account::profile_route_registration::modules::PostMethod();
}

const char* UserUpdateProfilePath()
{
    return memochat::account::profile_route_registration::modules::UserUpdateProfilePath();
}

const char* GetUserInfoPath()
{
    return memochat::account::profile_route_registration::modules::GetUserInfoPath();
}
} // namespace memochat::tests::account::profile_route_registration
