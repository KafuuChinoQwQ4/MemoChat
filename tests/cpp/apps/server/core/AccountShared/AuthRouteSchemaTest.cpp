#include "modules/auth/AuthRouteModule.h"

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

const char* ExpectedAuthRouteSchemaSnapshot()
{
    return "route: auth.user.register\n"
           "method: POST\n"
           "path: /user_register\n"
           "request: AuthRegisterRequestDto\n"
           "  - email\n"
           "  - user\n"
           "  - passwd\n"
           "  - confirm\n"
           "  - icon\n"
           "  - varifycode\n"
           "  - sex\n"
           "response: AuthRegisterResponseDto\n"
           "  - error\n"
           "  - uid\n"
           "  - user_id\n"
           "  - email\n"
           "  - user\n"
           "  - passwd\n"
           "  - confirm\n"
           "  - icon\n"
           "  - varifycode\n"
           "\n"
           "route: auth.reset.password\n"
           "method: POST\n"
           "path: /reset_pwd\n"
           "request: AuthResetPasswordRequestDto\n"
           "  - email\n"
           "  - user\n"
           "  - passwd\n"
           "  - varifycode\n"
           "response: AuthResetPasswordResponseDto\n"
           "  - error\n"
           "  - email\n"
           "  - user\n"
           "  - passwd\n"
           "  - varifycode\n"
           "\n";
}

} // namespace

TEST(AuthRouteSchemaTest, ListsOnlyStableAuthRegisterAndResetRoutes)
{
    const auto schemas = memochat::gate::modules::auth::AuthRouteModule::RouteSchemas();

    ASSERT_EQ(schemas.size(), 2U);
    EXPECT_EQ(schemas[0].name, "auth.user.register");
    EXPECT_EQ(schemas[0].method, "POST");
    EXPECT_EQ(schemas[0].path, "/user_register");
    EXPECT_EQ(schemas[0].request.type_name, "AuthRegisterRequestDto");
    EXPECT_EQ(schemas[0].response.type_name, "AuthRegisterResponseDto");

    EXPECT_EQ(schemas[1].name, "auth.reset.password");
    EXPECT_EQ(schemas[1].method, "POST");
    EXPECT_EQ(schemas[1].path, "/reset_pwd");
    EXPECT_EQ(schemas[1].request.type_name, "AuthResetPasswordRequestDto");
    EXPECT_EQ(schemas[1].response.type_name, "AuthResetPasswordResponseDto");
}

TEST(AuthRouteSchemaTest, BuildsFieldInventoriesFromAuthDtos)
{
    const auto schemas = memochat::gate::modules::auth::AuthRouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 2U);

    ExpectFields(schemas[0].request, {"email", "user", "passwd", "confirm", "icon", "varifycode", "sex"});
    ExpectFields(schemas[0].response,
                 {"error", "uid", "user_id", "email", "user", "passwd", "confirm", "icon", "varifycode"});

    ExpectFields(schemas[1].request, {"email", "user", "passwd", "varifycode"});
    ExpectFields(schemas[1].response, {"error", "email", "user", "passwd", "varifycode"});
}

TEST(AuthRouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::auth::AuthRouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedAuthRouteSchemaSnapshot());
}
