#include <gtest/gtest.h>

#include "domain/users/ChatUserProfileDto.hpp"
#include "data.hpp"
#include "json/GlazeCompat.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<chatusersupport::ChatUserProfileDto>(
    std::array<std::string_view, 8>{"uid", "user_id", "name", "email", "nick", "desc", "sex", "icon"}));
#endif

namespace
{

UserInfo MakeUser()
{
    UserInfo user;
    user.uid = 42;
    user.user_id = "u-42";
    user.pwd = "hash";
    user.name = "alice";
    user.email = "alice@example.test";
    user.nick = "Alice";
    user.desc = "hello";
    user.sex = 2;
    user.icon = "alice.png";
    return user;
}

} // namespace

TEST(ChatUserProfileDtoTest, ConvertsUserInfoAndCachesAllStableFields)
{
    const UserInfo user = MakeUser();
    const auto dto = chatusersupport::ChatUserProfileFromUserInfo(user);

    EXPECT_EQ(dto.uid, 42);
    EXPECT_EQ(dto.user_id, "u-42");
    EXPECT_EQ(dto.name, "alice");
    EXPECT_EQ(dto.email, "alice@example.test");
    EXPECT_EQ(dto.nick, "Alice");
    EXPECT_EQ(dto.desc, "hello");
    EXPECT_EQ(dto.sex, 2);
    EXPECT_EQ(dto.icon, "alice.png");

    std::string body;
    std::string error;
    ASSERT_TRUE(chatusersupport::EncodeChatUserProfileCache(dto, &body, &error)) << error;
    SCOPED_TRACE(body);
    EXPECT_EQ(body.find(R"("pwd")"), std::string::npos);
    EXPECT_EQ(body.find(R"("password")"), std::string::npos);
    EXPECT_EQ(body.find(R"("password_hash")"), std::string::npos);

    chatusersupport::ChatUserProfileDto decoded;
    ASSERT_TRUE(chatusersupport::DecodeChatUserProfileCache(body, &decoded, &error)) << error;
    EXPECT_EQ(decoded.uid, 42);
    EXPECT_EQ(decoded.user_id, "u-42");
    EXPECT_EQ(decoded.name, "alice");
    EXPECT_EQ(decoded.email, "alice@example.test");
    EXPECT_EQ(decoded.nick, "Alice");
    EXPECT_EQ(decoded.desc, "hello");
    EXPECT_EQ(decoded.sex, 2);
    EXPECT_EQ(decoded.icon, "alice.png");
}

TEST(ChatUserProfileDtoTest, FillsUserInfoFromCachedDto)
{
    const auto dto = chatusersupport::ChatUserProfileFromUserInfo(MakeUser());

    UserInfo user;
    chatusersupport::FillUserInfoFromChatUserProfile(dto, user);

    EXPECT_EQ(user.uid, 42);
    EXPECT_EQ(user.user_id, "u-42");
    EXPECT_TRUE(user.pwd.empty());
    EXPECT_EQ(user.name, "alice");
    EXPECT_EQ(user.email, "alice@example.test");
    EXPECT_EQ(user.nick, "Alice");
    EXPECT_EQ(user.desc, "hello");
    EXPECT_EQ(user.sex, 2);
    EXPECT_EQ(user.icon, "alice.png");
}

TEST(ChatUserProfileDtoTest, ProjectsResponseFieldsWithoutPassword)
{
    const auto dto = chatusersupport::ChatUserProfileFromUserInfo(MakeUser());

    const memochat::json::JsonValue uid_response = chatusersupport::ChatUserProfileToJsonValue(dto, true);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(uid_response, "uid", 0), 42);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(uid_response, "user_id", ""), "u-42");
    EXPECT_FALSE(uid_response.isMember("pwd")) << uid_response.toStyledString();
    EXPECT_FALSE(uid_response.isMember("password")) << uid_response.toStyledString();
    EXPECT_FALSE(uid_response.isMember("password_hash")) << uid_response.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(uid_response, "icon", ""), "alice.png");

    const memochat::json::JsonValue cached_name_response = chatusersupport::ChatUserProfileToJsonValue(dto, false);
    EXPECT_FALSE(cached_name_response.isMember("pwd")) << cached_name_response.toStyledString();
    EXPECT_FALSE(cached_name_response.isMember("password")) << cached_name_response.toStyledString();
    EXPECT_FALSE(cached_name_response.isMember("password_hash")) << cached_name_response.toStyledString();
    EXPECT_FALSE(cached_name_response.isMember("icon")) << cached_name_response.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(cached_name_response, "nick", ""), "Alice");

    memochat::json::JsonValue appended(memochat::json::object_t{});
    chatusersupport::AppendChatUserProfileToJsonValue(dto, appended, true);
    EXPECT_FALSE(appended.isMember("pwd")) << appended.toStyledString();
    EXPECT_FALSE(appended.isMember("password")) << appended.toStyledString();
    EXPECT_FALSE(appended.isMember("password_hash")) << appended.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(appended, "icon", ""), "alice.png");
}

TEST(ChatUserProfileDtoTest, RejectsMalformedCacheJsonAndNullOutputs)
{
    std::string error;
    chatusersupport::ChatUserProfileDto dto;

    EXPECT_FALSE(chatusersupport::DecodeChatUserProfileCache("not-json", &dto, &error));
    EXPECT_FALSE(error.empty());

    error.clear();
    EXPECT_FALSE(chatusersupport::EncodeChatUserProfileCache(dto, nullptr, &error));
    EXPECT_EQ(error, "output pointer is null");

    error.clear();
    EXPECT_FALSE(chatusersupport::DecodeChatUserProfileCache(R"({"uid":1})", nullptr, &error));
    EXPECT_EQ(error, "output pointer is null");
}
