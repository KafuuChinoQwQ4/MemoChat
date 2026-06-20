#include "modules/profile/ProfileRouteModule.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

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
           "  - uid\n"
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
           "  - uid\n"
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
    EXPECT_EQ(schemas[0].name, "profile.user.update");
    EXPECT_EQ(schemas[0].method, "POST");
    EXPECT_EQ(schemas[0].path, "/user_update_profile");
    EXPECT_EQ(schemas[0].request.type_name, "ProfileUpdateRequestDto");
    EXPECT_EQ(schemas[0].response.type_name, "ProfileUpdateResponseDto");

    EXPECT_EQ(schemas[1].name, "profile.user.info");
    EXPECT_EQ(schemas[1].method, "POST");
    EXPECT_EQ(schemas[1].path, "/get_user_info");
    EXPECT_EQ(schemas[1].request.type_name, "GetUserInfoRequestDto");
    EXPECT_EQ(schemas[1].response.type_name, "UserInfoResponseDto");
}

TEST(ProfileRouteSchemaTest, BuildsFieldInventoriesFromProfileDtos)
{
    const auto schemas = memochat::gate::modules::profile::ProfileRouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 2U);

    ExpectFields(schemas[0].request, {"uid", "name", "nick", "desc", "icon"});
    ExpectFields(schemas[0].response, {"error", "uid", "name", "nick", "desc", "icon"});

    ExpectFields(schemas[1].request, {"uid"});
    ExpectFields(schemas[1].response, {"error", "uid", "user_id", "name", "email", "nick", "icon", "desc", "sex"});
}

TEST(ProfileRouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::profile::ProfileRouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedProfileRouteSchemaSnapshot());
}
