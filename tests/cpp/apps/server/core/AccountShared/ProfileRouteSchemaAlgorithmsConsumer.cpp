import memochat.account.profile_route_schema_algorithms;

namespace memochat::tests::account::profile_route_schema
{
const char* PostMethod()
{
    return memochat::account::profile_route_schema::modules::PostMethod();
}

const char* UpdateProfilePath()
{
    return memochat::account::profile_route_schema::modules::UpdateProfilePath();
}

const char* UpdateProfileRouteName()
{
    return memochat::account::profile_route_schema::modules::UpdateProfileRouteName();
}

const char* UpdateProfileRequestTypeName()
{
    return memochat::account::profile_route_schema::modules::UpdateProfileRequestTypeName();
}

const char* UpdateProfileResponseTypeName()
{
    return memochat::account::profile_route_schema::modules::UpdateProfileResponseTypeName();
}

const char* GetUserInfoPath()
{
    return memochat::account::profile_route_schema::modules::GetUserInfoPath();
}

const char* GetUserInfoRouteName()
{
    return memochat::account::profile_route_schema::modules::GetUserInfoRouteName();
}

const char* GetUserInfoRequestTypeName()
{
    return memochat::account::profile_route_schema::modules::GetUserInfoRequestTypeName();
}

const char* UserInfoResponseTypeName()
{
    return memochat::account::profile_route_schema::modules::UserInfoResponseTypeName();
}
} // namespace memochat::tests::account::profile_route_schema
