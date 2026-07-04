#include "modules/auth/AuthRouteModule.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace memochat::tests::account::auth_route_schema
{
const char* PostMethod();
const char* RegisterPath();
const char* RegisterRouteName();
const char* RegisterRequestTypeName();
const char* RegisterResponseTypeName();
const char* ResetPasswordPath();
const char* ResetPasswordRouteName();
const char* ResetPasswordRequestTypeName();
const char* ResetPasswordResponseTypeName();
const char* LoginPath();
const char* LoginRouteName();
const char* LoginRequestTypeName();
const char* LoginResponseTypeName();
const char* RefreshPath();
const char* RefreshRouteName();
const char* RefreshRequestTypeName();
const char* RefreshResponseTypeName();
const char* LogoutPath();
const char* LogoutRouteName();
const char* LogoutRequestTypeName();
const char* LogoutResponseTypeName();
} // namespace memochat::tests::account::auth_route_schema

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
           "  - varifycode\n"
           "\n"
           "route: auth.user.login\n"
           "method: POST\n"
           "path: /user_login\n"
           "request: AuthLoginRequestDto\n"
           "  - email\n"
           "  - passwd\n"
           "  - client_ver\n"
           "response: AuthLoginResponseDto\n"
           "  - error\n"
           "  - protocol_version\n"
           "  - preferred_transport\n"
           "  - fallback_transport\n"
           "  - email\n"
           "  - uid\n"
           "  - user_id\n"
           "  - token\n"
           "  - login_ticket\n"
           "  - ticket_expire_ms\n"
           "  - refresh_token\n"
           "  - refresh_token_expires_in_sec\n"
           "  - user_profile\n"
           "  - chat_endpoints\n"
           "  - stage_metrics\n"
           "\n"
           "route: auth.refresh\n"
           "method: POST\n"
           "path: /auth/refresh\n"
           "request: AuthRefreshRequestDto\n"
           "  - refresh_token\n"
           "  - client_ver\n"
           "response: AuthLoginResponseDto\n"
           "  - error\n"
           "  - protocol_version\n"
           "  - preferred_transport\n"
           "  - fallback_transport\n"
           "  - email\n"
           "  - uid\n"
           "  - user_id\n"
           "  - token\n"
           "  - login_ticket\n"
           "  - ticket_expire_ms\n"
           "  - refresh_token\n"
           "  - refresh_token_expires_in_sec\n"
           "  - user_profile\n"
           "  - chat_endpoints\n"
           "  - stage_metrics\n"
           "\n"
           "route: auth.logout\n"
           "method: POST\n"
           "path: /auth/logout\n"
           "request: AuthLogoutRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - refresh_token\n"
           "  - client_ver\n"
           "  - all_devices\n"
           "response: AuthLogoutResponseDto\n"
           "  - error\n"
           "  - uid\n"
           "  - all_devices\n"
           "\n";
}

} // namespace

TEST(AuthRouteSchemaTest, ListsStableAuthRoutes)
{
    const auto schemas = memochat::gate::modules::auth::AuthRouteModule::RouteSchemas();

    ASSERT_EQ(schemas.size(), 5U);
    EXPECT_EQ(schemas[0].name, memochat::tests::account::auth_route_schema::RegisterRouteName());
    EXPECT_EQ(schemas[0].method, memochat::tests::account::auth_route_schema::PostMethod());
    EXPECT_EQ(schemas[0].path, memochat::tests::account::auth_route_schema::RegisterPath());
    EXPECT_EQ(schemas[0].request.type_name, memochat::tests::account::auth_route_schema::RegisterRequestTypeName());
    EXPECT_EQ(schemas[0].response.type_name, memochat::tests::account::auth_route_schema::RegisterResponseTypeName());

    EXPECT_EQ(schemas[1].name, memochat::tests::account::auth_route_schema::ResetPasswordRouteName());
    EXPECT_EQ(schemas[1].method, memochat::tests::account::auth_route_schema::PostMethod());
    EXPECT_EQ(schemas[1].path, memochat::tests::account::auth_route_schema::ResetPasswordPath());
    EXPECT_EQ(schemas[1].request.type_name,
              memochat::tests::account::auth_route_schema::ResetPasswordRequestTypeName());
    EXPECT_EQ(schemas[1].response.type_name,
              memochat::tests::account::auth_route_schema::ResetPasswordResponseTypeName());

    EXPECT_EQ(schemas[2].name, memochat::tests::account::auth_route_schema::LoginRouteName());
    EXPECT_EQ(schemas[2].method, memochat::tests::account::auth_route_schema::PostMethod());
    EXPECT_EQ(schemas[2].path, memochat::tests::account::auth_route_schema::LoginPath());
    EXPECT_EQ(schemas[2].request.type_name, memochat::tests::account::auth_route_schema::LoginRequestTypeName());
    EXPECT_EQ(schemas[2].response.type_name, memochat::tests::account::auth_route_schema::LoginResponseTypeName());

    EXPECT_EQ(schemas[3].name, memochat::tests::account::auth_route_schema::RefreshRouteName());
    EXPECT_EQ(schemas[3].method, memochat::tests::account::auth_route_schema::PostMethod());
    EXPECT_EQ(schemas[3].path, memochat::tests::account::auth_route_schema::RefreshPath());
    EXPECT_EQ(schemas[3].request.type_name, memochat::tests::account::auth_route_schema::RefreshRequestTypeName());
    EXPECT_EQ(schemas[3].response.type_name, memochat::tests::account::auth_route_schema::RefreshResponseTypeName());

    EXPECT_EQ(schemas[4].name, memochat::tests::account::auth_route_schema::LogoutRouteName());
    EXPECT_EQ(schemas[4].method, memochat::tests::account::auth_route_schema::PostMethod());
    EXPECT_EQ(schemas[4].path, memochat::tests::account::auth_route_schema::LogoutPath());
    EXPECT_EQ(schemas[4].request.type_name, memochat::tests::account::auth_route_schema::LogoutRequestTypeName());
    EXPECT_EQ(schemas[4].response.type_name, memochat::tests::account::auth_route_schema::LogoutResponseTypeName());
}

TEST(AuthRouteSchemaTest, BuildsFieldInventoriesFromAuthDtos)
{
    const auto schemas = memochat::gate::modules::auth::AuthRouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 5U);

    ExpectFields(schemas[0].request, {"email", "user", "passwd", "confirm", "icon", "varifycode", "sex"});
    ExpectFields(schemas[0].response, {"error", "uid", "user_id", "email", "user", "icon", "varifycode"});

    ExpectFields(schemas[1].request, {"email", "user", "passwd", "varifycode"});
    ExpectFields(schemas[1].response, {"error", "email", "user", "varifycode"});

    const std::vector<std::string> login_response_fields = {"error",
                                                            "protocol_version",
                                                            "preferred_transport",
                                                            "fallback_transport",
                                                            "email",
                                                            "uid",
                                                            "user_id",
                                                            "token",
                                                            "login_ticket",
                                                            "ticket_expire_ms",
                                                            "refresh_token",
                                                            "refresh_token_expires_in_sec",
                                                            "user_profile",
                                                            "chat_endpoints",
                                                            "stage_metrics"};
    ExpectFields(schemas[2].request, {"email", "passwd", "client_ver"});
    ExpectFields(schemas[2].response, login_response_fields);

    ExpectFields(schemas[3].request, {"refresh_token", "client_ver"});
    ExpectFields(schemas[3].response, login_response_fields);

    ExpectFields(schemas[4].request, {"uid", "token", "refresh_token", "client_ver", "all_devices"});
    ExpectFields(schemas[4].response, {"error", "uid", "all_devices"});
}

TEST(AuthRouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::auth::AuthRouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedAuthRouteSchemaSnapshot());
}
