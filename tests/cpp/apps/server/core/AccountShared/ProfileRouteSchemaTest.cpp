#include "modules/profile/ProfileRouteModule.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace memochat::tests::account::profile_route_schema
{
const char* PostMethod();
const char* UpdateProfilePath();
const char* UpdateProfileRouteName();
const char* UpdateProfileRequestTypeName();
const char* UpdateProfileResponseTypeName();
const char* GetUserInfoPath();
const char* GetUserInfoRouteName();
const char* GetUserInfoRequestTypeName();
const char* UserInfoResponseTypeName();
} // namespace memochat::tests::account::profile_route_schema

namespace
{

void ExpectFields(const memochat::gate::routing::RouteBodySchema& schema, const std::vector<std::string>& names)
{
    ASSERT_EQ(schema.fields.size(), names.size());
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        EXPECT_EQ(schema.fields[i].name, names[i]);
    }
}

const char* ExpectedProfileRouteSchemaSnapshot()
{
    return "route: profile.user.update\n"
           "method: POST\n"
           "path: /user_update_profile\n"
           "request: ProfileUpdateRequestDto\n"
           "  - name\n"
           "  - nick\n"
           "  - desc\n"
           "  - icon\n"
           "response: ProfileUpdateResponseDto\n"
           "  - error\n"
           "  - uid\n"
           "  - name\n"
           "  - nick\n"
           "  - desc\n"
           "  - icon\n"
           "\n"
           "route: profile.user.info\n"
           "method: POST\n"
           "path: /get_user_info\n"
           "request: GetUserInfoRequestDto\n"
           "response: UserInfoResponseDto\n"
           "  - error\n"
           "  - uid\n"
           "  - user_id\n"
           "  - name\n"
           "  - email\n"
           "  - nick\n"
           "  - icon\n"
           "  - desc\n"
           "  - sex\n"
           "\n";
}

} // namespace

TEST(ProfileRouteSchemaTest, ListsOnlyStableProfileRoutes)
{
    const auto schemas = memochat::gate::modules::profile::ProfileRouteModule::RouteSchemas();

    ASSERT_EQ(schemas.size(), 2U);
    EXPECT_EQ(schemas[0].name, memochat::tests::account::profile_route_schema::UpdateProfileRouteName());
    EXPECT_EQ(schemas[0].method, memochat::tests::account::profile_route_schema::PostMethod());
    EXPECT_EQ(schemas[0].path, memochat::tests::account::profile_route_schema::UpdateProfilePath());
    EXPECT_EQ(schemas[0].request.type_name,
              memochat::tests::account::profile_route_schema::UpdateProfileRequestTypeName());
    EXPECT_EQ(schemas[0].response.type_name,
              memochat::tests::account::profile_route_schema::UpdateProfileResponseTypeName());

    EXPECT_EQ(schemas[1].name, memochat::tests::account::profile_route_schema::GetUserInfoRouteName());
    EXPECT_EQ(schemas[1].method, memochat::tests::account::profile_route_schema::PostMethod());
    EXPECT_EQ(schemas[1].path, memochat::tests::account::profile_route_schema::GetUserInfoPath());
    EXPECT_EQ(schemas[1].request.type_name,
              memochat::tests::account::profile_route_schema::GetUserInfoRequestTypeName());
    EXPECT_EQ(schemas[1].response.type_name,
              memochat::tests::account::profile_route_schema::UserInfoResponseTypeName());
}

TEST(ProfileRouteSchemaTest, BuildsFieldInventoriesFromProfileDtos)
{
    const auto schemas = memochat::gate::modules::profile::ProfileRouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 2U);

    ExpectFields(schemas[0].request, {"name", "nick", "desc", "icon"});
    ExpectFields(schemas[0].response, {"error", "uid", "name", "nick", "desc", "icon"});

    ExpectFields(schemas[1].request, {});
    ExpectFields(schemas[1].response, {"error", "uid", "user_id", "name", "email", "nick", "icon", "desc", "sex"});
}

TEST(ProfileRouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::profile::ProfileRouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedProfileRouteSchemaSnapshot());
}
