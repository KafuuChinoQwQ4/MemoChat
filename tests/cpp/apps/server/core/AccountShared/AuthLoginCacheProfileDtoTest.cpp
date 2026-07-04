#include <gtest/gtest.h>

#include "AuthLoginCacheProfileDto.hpp"
#include "json/GlazeCompat.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthLoginCacheProfileDto>(
    std::array<std::string_view, 8>{"uid", "name", "email", "user_id", "nick", "icon", "desc", "sex"}));
#endif

namespace
{

gateauthsupport::UserInfo MakeUser()
{
    gateauthsupport::UserInfo user;
    user.uid = 42;
    user.pwd = "secret";
    user.name = "alice";
    user.email = "alice@example.com";
    user.user_id = "u-42";
    user.nick = "Alice";
    user.icon = "icon.png";
    user.desc = "hello";
    user.sex = 2;
    return user;
}

} // namespace

TEST(AuthLoginCacheProfileDtoTest, EncodesCacheProfileWithoutPasswordFields)
{
    std::string body;
    ASSERT_TRUE(gateauthsupport::EncodeLoginCacheProfile(MakeUser(), &body));

    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::glaze_parse(root, body));
    EXPECT_EQ(root["uid"].asInt(), 42);
    EXPECT_EQ(root["name"].asString(), "alice");
    EXPECT_EQ(root["email"].asString(), "alice@example.com");
    EXPECT_EQ(root["user_id"].asString(), "u-42");
    EXPECT_EQ(root["nick"].asString(), "Alice");
    EXPECT_EQ(root["icon"].asString(), "icon.png");
    EXPECT_EQ(root["desc"].asString(), "hello");
    EXPECT_EQ(root["sex"].asInt(), 2);
    EXPECT_FALSE(root.isMember("pwd")) << body;
    EXPECT_FALSE(root.isMember("password")) << body;
    EXPECT_FALSE(root.isMember("password_hash")) << body;
}

TEST(AuthLoginCacheProfileDtoTest, DecodesFullCacheProfile)
{
    gateauthsupport::UserInfo user;
    ASSERT_TRUE(gateauthsupport::DecodeLoginCacheProfile(
        R"({"uid":7,"name":"bob","email":"bob@example.com","user_id":"u-7","nick":"B","icon":"i","desc":"d","sex":1})",
        &user));

    EXPECT_EQ(user.uid, 7);
    EXPECT_TRUE(user.pwd.empty());
    EXPECT_EQ(user.name, "bob");
    EXPECT_EQ(user.email, "bob@example.com");
    EXPECT_EQ(user.user_id, "u-7");
    EXPECT_EQ(user.nick, "B");
    EXPECT_EQ(user.icon, "i");
    EXPECT_EQ(user.desc, "d");
    EXPECT_EQ(user.sex, 1);
}

TEST(AuthLoginCacheProfileDtoTest, DecodesMissingOptionalFieldsAsDefaults)
{
    gateauthsupport::UserInfo user;
    ASSERT_TRUE(gateauthsupport::DecodeLoginCacheProfile(R"({"uid":7})", &user));

    EXPECT_EQ(user.uid, 7);
    EXPECT_TRUE(user.pwd.empty());
    EXPECT_TRUE(user.name.empty());
    EXPECT_TRUE(user.email.empty());
    EXPECT_TRUE(user.user_id.empty());
    EXPECT_EQ(user.sex, 0);
}

TEST(AuthLoginCacheProfileDtoTest, RejectsInvalidCacheProfile)
{
    gateauthsupport::UserInfo user;

    EXPECT_FALSE(gateauthsupport::DecodeLoginCacheProfile("not-json", &user));
    EXPECT_FALSE(gateauthsupport::DecodeLoginCacheProfile(R"({"uid":0})", &user));
    EXPECT_FALSE(
        gateauthsupport::DecodeLoginCacheProfile(R"({"uid":7})", static_cast<gateauthsupport::UserInfo*>(nullptr)));
}

TEST(AuthLoginCacheProfileDtoTest, ReportsNullEncodeOutput)
{
    std::string error;

    EXPECT_FALSE(gateauthsupport::EncodeLoginCacheProfile(MakeUser(), nullptr, &error));
    EXPECT_EQ(error, "output pointer is null");
}
