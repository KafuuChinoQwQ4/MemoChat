#include <gtest/gtest.h>

#include "AuthPublicDtos.hpp"
#include "json/GlazeCompat.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthEmailRequestDto>(
    std::array<std::string_view, 1>{"email"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthResetPasswordRequestDto>(
    std::array<std::string_view, 4>{"email", "user", "passwd", "varifycode"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthRegisterRequestDto>(
    std::array<std::string_view, 7>{"email", "user", "passwd", "confirm", "icon", "varifycode", "sex"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthLoginRequestDto>(
    std::array<std::string_view, 3>{"email", "passwd", "client_ver"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthRefreshRequestDto>(
    std::array<std::string_view, 2>{"refresh_token", "client_ver"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthLogoutRequestDto>(
    std::array<std::string_view, 5>{"uid", "token", "refresh_token", "client_ver", "all_devices"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::ProfileUpdateRequestDto>(
    std::array<std::string_view, 5>{"token", "name", "nick", "desc", "icon"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::GetUserInfoRequestDto>(
    std::array<std::string_view, 1>{"token"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthResetPasswordResponseDto>(
    std::array<std::string_view, 4>{"error", "email", "user", "varifycode"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthRegisterResponseDto>(
    std::array<std::string_view, 7>{"error", "uid", "user_id", "email", "user", "icon", "varifycode"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::ProfileUpdateResponseDto>(
    std::array<std::string_view, 6>{"error", "uid", "name", "nick", "desc", "icon"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::UserInfoResponseDto>(
    std::array<std::string_view, 9>{"error", "uid", "user_id", "name", "email", "nick", "icon", "desc", "sex"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthLogoutResponseDto>(
    std::array<std::string_view, 3>{"error", "uid", "all_devices"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthLoginUserProfileDto>(
    std::array<std::string_view, 8>{"uid", "user_id", "name", "nick", "icon", "desc", "email", "sex"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthChatEndpointDto>(
    std::array<std::string_view, 7>{"transport", "host", "port", "path", "tls", "server_name", "priority"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthLoginStageMetricsDto>(
    std::array<std::string_view, 6>{"mysql_check_pwd_ms",
                                    "route_select_ms",
                                    "ticket_issue_ms",
                                    "user_login_total_ms",
                                    "route_source",
                                    "status_route_detail"}));
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthLoginResponseDto>(
    std::array<std::string_view, 15>{"error",
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
                                     "stage_metrics"}));
#endif

namespace
{
template <typename T, typename = void> struct HasUidField : std::false_type
{
};

template <typename T> struct HasUidField<T, std::void_t<decltype(std::declval<T&>().uid)>> : std::true_type
{
};

memochat::json::JsonValue Parse(std::string_view body)
{
    memochat::json::JsonValue root;
    EXPECT_TRUE(memochat::json::glaze_parse(root, body));
    return root;
}

std::string ValidRefreshToken()
{
    return std::string(36, 'a') + "." + std::string(64, 'b');
}

} // namespace

static_assert(!HasUidField<gateauthsupport::ProfileUpdateRequestDto>::value,
              "profile update requests must derive uid from the authenticated token, not the JSON body");
static_assert(!HasUidField<gateauthsupport::GetUserInfoRequestDto>::value,
              "get-user-info requests must derive uid from the authenticated token, not the JSON body");

TEST(AuthPublicDtosTest, DecodesAuthEmailRequestAndRequiredKey)
{
    memochat::json::JsonValue parsed;
    gateauthsupport::AuthEmailRequestDto request;
    ASSERT_TRUE(gateauthsupport::DecodeAuthEmailRequest(R"({"email":"alice@example.com"})", &request, &parsed));

    EXPECT_EQ(request.email, "alice@example.com");
    EXPECT_TRUE(gateauthsupport::HasAuthEmailRequiredFields(parsed));
    EXPECT_FALSE(gateauthsupport::HasAuthEmailRequiredFields(Parse(R"({"user":"alice"})")));
}

TEST(AuthPublicDtosTest, DecodesResetPasswordRequest)
{
    const auto request = gateauthsupport::AuthResetPasswordRequestFromJsonValue(
        Parse(R"({"email":"alice@example.com","user":"alice","passwd":"new","varifycode":"123456"})"));

    EXPECT_EQ(request.email, "alice@example.com");
    EXPECT_EQ(request.user, "alice");
    EXPECT_EQ(request.passwd, "new");
    EXPECT_EQ(request.varifycode, "123456");
}

TEST(AuthPublicDtosTest, DecodesRegisterRequestWithOptionalSexDefault)
{
    gateauthsupport::AuthRegisterRequestDto full;
    ASSERT_TRUE(gateauthsupport::DecodeAuthRegisterRequest(
        R"({"email":"alice@example.com","user":"alice","passwd":"pw","confirm":"pw","icon":"i.png","varifycode":"654321","sex":2})",
        &full));

    EXPECT_EQ(full.email, "alice@example.com");
    EXPECT_EQ(full.user, "alice");
    EXPECT_EQ(full.passwd, "pw");
    EXPECT_EQ(full.confirm, "pw");
    EXPECT_EQ(full.icon, "i.png");
    EXPECT_EQ(full.varifycode, "654321");
    EXPECT_EQ(full.sex, 2);

    const auto missing_sex = gateauthsupport::AuthRegisterRequestFromJsonValue(Parse(
        R"({"email":"alice@example.com","user":"alice","passwd":"pw","confirm":"pw","icon":"i.png","varifycode":"654321"})"));
    EXPECT_EQ(missing_sex.sex, 0);
}

TEST(AuthPublicDtosTest, DecodesLoginRequestWithClientVersionDefault)
{
    const auto full = gateauthsupport::AuthLoginRequestFromJsonValue(
        Parse(R"({"email":"alice@example.com","passwd":"pw","client_ver":"1.2.3"})"));
    EXPECT_EQ(full.email, "alice@example.com");
    EXPECT_EQ(full.passwd, "pw");
    EXPECT_EQ(full.client_ver, "1.2.3");

    const auto missing_client_ver =
        gateauthsupport::AuthLoginRequestFromJsonValue(Parse(R"({"email":"alice@example.com","passwd":"pw"})"));
    EXPECT_EQ(missing_client_ver.client_ver, "");
}

TEST(AuthPublicDtosTest, DecodesRefreshRequestWithClientVersionDefault)
{
    const auto full = gateauthsupport::AuthRefreshRequestFromJsonValue(
        Parse(R"({"refresh_token":"selector.verifier","client_ver":"3.0.0"})"));
    EXPECT_EQ(full.refresh_token, "selector.verifier");
    EXPECT_EQ(full.client_ver, "3.0.0");

    const auto missing_client_ver =
        gateauthsupport::AuthRefreshRequestFromJsonValue(Parse(R"({"refresh_token":"selector.verifier"})"));
    EXPECT_EQ(missing_client_ver.refresh_token, "selector.verifier");
    EXPECT_EQ(missing_client_ver.client_ver, "");
}

TEST(AuthPublicDtosTest, DecodesLogoutRequestWithDefaults)
{
    const auto full = gateauthsupport::AuthLogoutRequestFromJsonValue(Parse(
        R"({"uid":42,"token":"access","refresh_token":"selector.verifier","client_ver":"3.0.0","all_devices":true})"));
    EXPECT_EQ(full.uid, 42);
    EXPECT_EQ(full.token, "access");
    EXPECT_EQ(full.refresh_token, "selector.verifier");
    EXPECT_EQ(full.client_ver, "3.0.0");
    EXPECT_TRUE(full.all_devices);

    const auto minimal = gateauthsupport::AuthLogoutRequestFromJsonValue(Parse(R"({"uid":42,"token":"access"})"));
    EXPECT_EQ(minimal.uid, 42);
    EXPECT_EQ(minimal.token, "access");
    EXPECT_EQ(minimal.refresh_token, "");
    EXPECT_EQ(minimal.client_ver, "");
    EXPECT_FALSE(minimal.all_devices);
}

TEST(AuthPublicDtosTest, DecodesProfileUpdateAndRequiredKeys)
{
    memochat::json::JsonValue parsed;
    gateauthsupport::ProfileUpdateRequestDto request;
    ASSERT_TRUE(gateauthsupport::DecodeProfileUpdateRequest(
        R"({"uid":42,"token":"access","name":"alice","nick":"Alice","desc":"hello","icon":"i.png"})",
        &request,
        &parsed));

    EXPECT_EQ(request.token, "access");
    EXPECT_EQ(request.name, "alice");
    EXPECT_EQ(request.nick, "Alice");
    EXPECT_EQ(request.desc, "hello");
    EXPECT_EQ(request.icon, "i.png");
    EXPECT_TRUE(gateauthsupport::HasProfileUpdateRequiredFields(parsed));

    const auto missing_name = gateauthsupport::ProfileUpdateRequestFromJsonValue(
        Parse(R"({"uid":42,"token":"access","nick":"Alice","desc":"hello","icon":"i.png"})"));
    EXPECT_EQ(missing_name.name, "");
    EXPECT_TRUE(gateauthsupport::HasProfileUpdateRequiredFields(
        Parse(R"({"token":"access","nick":"Alice","desc":"hello","icon":"i.png"})")));
    EXPECT_FALSE(gateauthsupport::HasProfileUpdateRequiredFields(
        Parse(R"({"uid":42,"nick":"Alice","desc":"hello","icon":"i.png"})")));
    EXPECT_FALSE(gateauthsupport::HasProfileUpdateRequiredFields(Parse(R"({"uid":42,"nick":"Alice","icon":"i.png"})")));
}

TEST(AuthPublicDtosTest, DecodesGetUserInfoRequest)
{
    gateauthsupport::GetUserInfoRequestDto request;
    ASSERT_TRUE(gateauthsupport::DecodeGetUserInfoRequest(R"({"uid":42,"token":"access"})", &request));
    EXPECT_EQ(request.token, "access");
}

TEST(AuthPublicDtosTest, PreservesRepresentativeWrongTypeDefaults)
{
    const auto profile = gateauthsupport::ProfileUpdateRequestFromJsonValue(
        Parse(R"({"uid":"bad","name":false,"nick":7,"desc":{},"icon":[]})"));
    EXPECT_EQ(profile.token, "");
    EXPECT_EQ(profile.name, "");
    EXPECT_EQ(profile.nick, "");
    EXPECT_EQ(profile.desc, "");
    EXPECT_EQ(profile.icon, "");

    const auto login = gateauthsupport::AuthLoginRequestFromJsonValue(Parse(R"({"email":7,"passwd":false})"));
    EXPECT_EQ(login.email, "");
    EXPECT_EQ(login.passwd, "");
}

TEST(AuthPublicDtosTest, ValidatesAuthEmailFormatBeforeSideEffects)
{
    EXPECT_TRUE(gateauthsupport::IsValidAuthEmail("alice.smith+tag@example.co"));
    EXPECT_TRUE(gateauthsupport::ValidateAuthEmailRequest({"alice@example.com"}).ok());

    for (const std::string invalid : {"alice",
                                      "alice@@example.com",
                                      " alice@example.com",
                                      "alice@example",
                                      ".alice@example.com",
                                      "alice@example-.com",
                                      "alice@example.c1"})
    {
        const auto result = gateauthsupport::ValidateAuthEmailRequest({invalid});
        EXPECT_EQ(result.code, gateauthsupport::AuthInputValidationCode::InvalidEmail) << invalid;
        EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Email);
    }
}

TEST(AuthPublicDtosTest, RejectsAuthFieldLengthsBeforeRedisDbOrHashing)
{
    gateauthsupport::AuthRegisterRequestDto register_request{.email = "alice@example.com",
                                                             .user = "alice",
                                                             .passwd = "password",
                                                             .confirm = "password",
                                                             .icon = "i.png",
                                                             .varifycode = "123456",
                                                             .sex = 0};
    register_request.email =
        std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Email) + 1, 'a');
    auto result = gateauthsupport::ValidateAuthRegisterRequest(register_request);
    EXPECT_EQ(result.code, gateauthsupport::AuthInputValidationCode::TooLong);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Email);

    register_request.email = "alice@example.com";
    register_request.user =
        std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::User) + 1, 'u');
    result = gateauthsupport::ValidateAuthRegisterRequest(register_request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::User);

    register_request.user = "alice";
    register_request.passwd =
        std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Passwd) + 1, 'p');
    result = gateauthsupport::ValidateAuthRegisterRequest(register_request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Passwd);

    register_request.passwd = "password";
    register_request.confirm =
        std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Confirm) + 1, 'c');
    result = gateauthsupport::ValidateAuthRegisterRequest(register_request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Confirm);

    register_request.confirm = "password";
    register_request.icon =
        std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Icon) + 1, 'i');
    result = gateauthsupport::ValidateAuthRegisterRequest(register_request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Icon);

    register_request.icon = "i.png";
    register_request.varifycode =
        std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::VarifyCode) + 1, '1');
    result = gateauthsupport::ValidateAuthRegisterRequest(register_request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::VarifyCode);

    gateauthsupport::AuthRefreshRequestDto refresh_request{
        .refresh_token =
            std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::RefreshToken) + 1, 'a'),
        .client_ver = "3.0.0"};
    result = gateauthsupport::ValidateAuthRefreshRequest(refresh_request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::RefreshToken);

    refresh_request.refresh_token = ValidRefreshToken();
    refresh_request.client_ver =
        std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::ClientVer) + 1, '1');
    result = gateauthsupport::ValidateAuthRefreshRequest(refresh_request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::ClientVer);
}

TEST(AuthPublicDtosTest, ValidatesRefreshTokenShapeAndRateLimitSubject)
{
    const auto token = ValidRefreshToken();
    EXPECT_TRUE(gateauthsupport::IsValidAuthRefreshTokenShape(token));
    EXPECT_EQ(gateauthsupport::AuthRefreshTokenRateLimitSubject(token), std::string(36, 'a'));

    gateauthsupport::AuthRefreshRequestDto request{.refresh_token = token, .client_ver = "3.0.0"};
    EXPECT_TRUE(gateauthsupport::ValidateAuthRefreshRequest(request).ok());

    request.refresh_token = std::string(36, 'A') + "." + std::string(64, 'b');
    const auto result = gateauthsupport::ValidateAuthRefreshRequest(request);
    EXPECT_EQ(result.code, gateauthsupport::AuthInputValidationCode::InvalidRefreshToken);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::RefreshToken);
    EXPECT_EQ(gateauthsupport::AuthRefreshTokenRateLimitSubject(request.refresh_token), "");
}

TEST(AuthPublicDtosTest, ValidatesLogoutTokensBeforeRedis)
{
    gateauthsupport::AuthLogoutRequestDto logout_request{.uid = 42,
                                                         .token = "access-token",
                                                         .refresh_token = ValidRefreshToken(),
                                                         .client_ver = "3.0.0",
                                                         .all_devices = false};
    EXPECT_TRUE(gateauthsupport::ValidateAuthLogoutRequest(logout_request).ok());

    logout_request.refresh_token.clear();
    auto result = gateauthsupport::ValidateAuthLogoutRequest(logout_request);
    EXPECT_EQ(result.code, gateauthsupport::AuthInputValidationCode::MissingRequired);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::RefreshToken);

    logout_request.all_devices = true;
    EXPECT_TRUE(gateauthsupport::ValidateAuthLogoutRequest(logout_request).ok());

    logout_request.token =
        std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Token) + 1, 't');
    result = gateauthsupport::ValidateAuthLogoutRequest(logout_request);
    EXPECT_EQ(result.code, gateauthsupport::AuthInputValidationCode::TooLong);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Token);
}

TEST(AuthPublicDtosTest, ValidatesProfileUpdateFieldsBeforeTokenLookupOrDb)
{
    gateauthsupport::ProfileUpdateRequestDto request{.token = "access-token",
                                                     .name = "alice",
                                                     .nick = "Alice",
                                                     .desc = "",
                                                     .icon = ""};
    EXPECT_TRUE(gateauthsupport::ValidateProfileUpdateRequest(request).ok());

    request.token = std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Token) + 1, 't');
    auto result = gateauthsupport::ValidateProfileUpdateRequest(request);
    EXPECT_EQ(result.code, gateauthsupport::AuthInputValidationCode::TooLong);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Token);

    request.token = "access-token";
    request.name = std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::User) + 1, 'n');
    result = gateauthsupport::ValidateProfileUpdateRequest(request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::User);

    request.name = "alice";
    request.nick.clear();
    result = gateauthsupport::ValidateProfileUpdateRequest(request);
    EXPECT_EQ(result.code, gateauthsupport::AuthInputValidationCode::MissingRequired);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Nick);

    request.nick = std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Nick) + 1, 'A');
    result = gateauthsupport::ValidateProfileUpdateRequest(request);
    EXPECT_EQ(result.code, gateauthsupport::AuthInputValidationCode::TooLong);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Nick);

    request.nick = "Alice";
    request.desc = std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Desc) + 1, 'd');
    result = gateauthsupport::ValidateProfileUpdateRequest(request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Desc);

    request.desc.clear();
    request.icon = std::string(gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Icon) + 1, 'i');
    result = gateauthsupport::ValidateProfileUpdateRequest(request);
    EXPECT_EQ(result.field, gateauthsupport::AuthInputField::Icon);
}

TEST(AuthPublicDtosTest, RejectsMalformedJsonAndNullOutputs)
{
    gateauthsupport::AuthEmailRequestDto email_request;
    std::string error;

    EXPECT_FALSE(gateauthsupport::DecodeAuthEmailRequest("not-json", &email_request, nullptr, &error));
    EXPECT_EQ(error, "invalid json");

    error.clear();
    EXPECT_FALSE(gateauthsupport::DecodeAuthEmailRequest(R"({"email":"alice@example.com"})",
                                                         static_cast<gateauthsupport::AuthEmailRequestDto*>(nullptr),
                                                         nullptr,
                                                         &error));
    EXPECT_EQ(error, "output pointer is null");
}

TEST(AuthPublicDtosTest, WritesResetPasswordResponseWithExistingWireFields)
{
    gateauthsupport::AuthResetPasswordResponseDto reset;
    reset.error = 0;
    reset.email = "alice@example.com";
    reset.user = "alice";
    reset.varifycode = "123456";

    const memochat::json::JsonValue reset_json = gateauthsupport::AuthResetPasswordResponseToJsonValue(reset);
    ASSERT_TRUE(reset_json.isObject()) << reset_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(reset_json, "error", -1), 0);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(reset_json, "email", ""), "alice@example.com");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(reset_json, "user", ""), "alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(reset_json, "varifycode", ""), "123456");
    EXPECT_FALSE(memochat::json::glaze_has_key(reset_json, "passwd"));
}

TEST(AuthPublicDtosTest, WritesRegisterResponseWithExistingWireFields)
{
    gateauthsupport::AuthRegisterResponseDto response;
    response.error = 0;
    response.uid = 42;
    response.user_id = "u-42";
    response.email = "alice@example.com";
    response.user = "alice";
    response.icon = "i.png";
    response.varifycode = "654321";

    const memochat::json::JsonValue root = gateauthsupport::AuthRegisterResponseToJsonValue(response);
    ASSERT_TRUE(root.isObject()) << root.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(root, "error", -1), 0);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(root, "uid", 0), 42);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "user_id", ""), "u-42");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "email", ""), "alice@example.com");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "user", ""), "alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "icon", ""), "i.png");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "varifycode", ""), "654321");
    EXPECT_FALSE(memochat::json::glaze_has_key(root, "passwd"));
    EXPECT_FALSE(memochat::json::glaze_has_key(root, "confirm"));
}

TEST(AuthPublicDtosTest, WritesLogoutResponseWithoutSecrets)
{
    gateauthsupport::AuthLogoutResponseDto response;
    response.error = 0;
    response.uid = 42;
    response.all_devices = true;

    const memochat::json::JsonValue root = gateauthsupport::AuthLogoutResponseToJsonValue(response);
    ASSERT_TRUE(root.isObject()) << root.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(root, "error", -1), 0);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(root, "uid", 0), 42);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(root, "all_devices", false));
    EXPECT_FALSE(memochat::json::glaze_has_key(root, "token"));
    EXPECT_FALSE(memochat::json::glaze_has_key(root, "refresh_token"));
    EXPECT_FALSE(memochat::json::glaze_has_key(root, "login_ticket"));
}

TEST(AuthPublicDtosTest, WritesProfileResponsesWithExistingWireFields)
{
    gateauthsupport::ProfileUpdateResponseDto profile;
    profile.error = 0;
    profile.uid = 42;
    profile.name = "alice";
    profile.nick = "Alice";
    profile.desc = "hello";
    profile.icon = "i.png";

    const memochat::json::JsonValue profile_json = gateauthsupport::ProfileUpdateResponseToJsonValue(profile);
    ASSERT_TRUE(profile_json.isObject()) << profile_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(profile_json, "error", -1), 0);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(profile_json, "uid", 0), 42);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(profile_json, "name", ""), "alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(profile_json, "nick", ""), "Alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(profile_json, "desc", ""), "hello");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(profile_json, "icon", ""), "i.png");

    gateauthsupport::UserInfoResponseDto user_info;
    user_info.error = 0;
    user_info.uid = 42;
    user_info.user_id = "u-42";
    user_info.name = "alice";
    user_info.email = "alice@example.com";
    user_info.nick = "Alice";
    user_info.icon = "i.png";
    user_info.desc = "hello";
    user_info.sex = 2;

    const memochat::json::JsonValue user_info_json = gateauthsupport::UserInfoResponseToJsonValue(user_info);
    ASSERT_TRUE(user_info_json.isObject()) << user_info_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(user_info_json, "error", -1), 0);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(user_info_json, "uid", 0), 42);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(user_info_json, "user_id", ""), "u-42");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(user_info_json, "name", ""), "alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(user_info_json, "email", ""), "alice@example.com");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(user_info_json, "nick", ""), "Alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(user_info_json, "icon", ""), "i.png");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(user_info_json, "desc", ""), "hello");
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(user_info_json, "sex", 0), 2);
}

TEST(AuthPublicDtosTest, WritesLoginUserProfileInWireOrder)
{
    const auto json =
        gateauthsupport::AuthLoginUserProfileToJsonValue(gateauthsupport::AuthLoginUserProfileDto{.uid = 7,
                                                                                                  .user_id = "u-7",
                                                                                                  .name = "Alice",
                                                                                                  .nick = "A",
                                                                                                  .icon = "i.png",
                                                                                                  .desc = "hello",
                                                                                                  .email = "a@x.com",
                                                                                                  .sex = 2});

    EXPECT_EQ(memochat::json::glaze_safe_get<int>(json, "uid", 0), 7);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "user_id", ""), "u-7");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "name", ""), "Alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "nick", ""), "A");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "icon", ""), "i.png");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "desc", ""), "hello");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "email", ""), "a@x.com");
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(json, "sex", 0), 2);
}

TEST(AuthPublicDtosTest, WritesChatEndpointInWireOrder)
{
    const auto json =
        gateauthsupport::AuthChatEndpointToJsonValue(gateauthsupport::AuthChatEndpointDto{.transport = "quic",
                                                                                          .host = "10.0.0.1",
                                                                                          .port = "8443",
                                                                                          .path = "/ws",
                                                                                          .tls = true,
                                                                                          .server_name = "chat1",
                                                                                          .priority = 3});

    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "transport", ""), "quic");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "host", ""), "10.0.0.1");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "port", ""), "8443");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "path", ""), "/ws");
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(json, "tls", false));
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "server_name", ""), "chat1");
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(json, "priority", 0), 3);
}

TEST(AuthPublicDtosTest, WritesLoginStageMetricsInWireOrder)
{
    const auto json = gateauthsupport::AuthLoginStageMetricsToJsonValue(
        gateauthsupport::AuthLoginStageMetricsDto{.mysql_check_pwd_ms = 11,
                                                  .route_select_ms = 22,
                                                  .ticket_issue_ms = 33,
                                                  .user_login_total_ms = 66,
                                                  .route_source = "status",
                                                  .status_route_detail = "ok"});

    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(json, "mysql_check_pwd_ms", 0), 11);
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(json, "route_select_ms", 0), 22);
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(json, "ticket_issue_ms", 0), 33);
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(json, "user_login_total_ms", 0), 66);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "route_source", ""), "status");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(json, "status_route_detail", ""), "ok");
}
