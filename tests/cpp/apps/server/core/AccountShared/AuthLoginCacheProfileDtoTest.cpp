#include <gtest/gtest.h>

#include "AuthLoginCacheProfileDto.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<gateauthsupport::AuthLoginCacheProfileDto>(
    std::array<std::string_view, 9>{"uid", "pwd", "name", "email", "user_id", "nick", "icon", "desc", "sex"}));
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

TEST(AuthLoginCacheProfileDtoTest, EncodesCacheProfileWithExistingWireFieldNames)
{
    std::string body;
    ASSERT_TRUE(gateauthsupport::EncodeLoginCacheProfile(MakeUser(), &body));

    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::glaze_parse(root, body));
    EXPECT_EQ(root["uid"].asInt(), 42);
    EXPECT_EQ(root["pwd"].asString(), "secret");
    EXPECT_EQ(root["name"].asString(), "alice");
    EXPECT_EQ(root["email"].asString(), "alice@example.com");
    EXPECT_EQ(root["user_id"].asString(), "u-42");
    EXPECT_EQ(root["nick"].asString(), "Alice");
    EXPECT_EQ(root["icon"].asString(), "icon.png");
    EXPECT_EQ(root["desc"].asString(), "hello");
    EXPECT_EQ(root["sex"].asInt(), 2);
}

TEST(AuthLoginCacheProfileDtoTest, DecodesFullCacheProfile)
{
    gateauthsupport::UserInfo user;
    ASSERT_TRUE(gateauthsupport::DecodeLoginCacheProfile(
        R"({"uid":7,"pwd":"pw","name":"bob","email":"bob@example.com","user_id":"u-7","nick":"B","icon":"i","desc":"d","sex":1})",
        &user));

    EXPECT_EQ(user.uid, 7);
    EXPECT_EQ(user.pwd, "pw");
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
    ASSERT_TRUE(gateauthsupport::DecodeLoginCacheProfile(R"({"uid":7,"pwd":"pw"})", &user));

    EXPECT_EQ(user.uid, 7);
    EXPECT_EQ(user.pwd, "pw");
    EXPECT_TRUE(user.name.empty());
    EXPECT_TRUE(user.email.empty());
    EXPECT_TRUE(user.user_id.empty());
    EXPECT_EQ(user.sex, 0);
}

TEST(AuthLoginCacheProfileDtoTest, RejectsInvalidCacheProfile)
{
    gateauthsupport::UserInfo user;

    EXPECT_FALSE(gateauthsupport::DecodeLoginCacheProfile("not-json", &user));
    EXPECT_FALSE(gateauthsupport::DecodeLoginCacheProfile(R"({"uid":0,"pwd":"pw"})", &user));
    EXPECT_FALSE(gateauthsupport::DecodeLoginCacheProfile(R"({"uid":7,"pwd":""})", &user));
    EXPECT_FALSE(gateauthsupport::DecodeLoginCacheProfile(R"({"uid":7})", &user));
    EXPECT_FALSE(gateauthsupport::DecodeLoginCacheProfile(R"({"uid":7,"pwd":"pw"})",
                                                          static_cast<gateauthsupport::UserInfo*>(nullptr)));
}

TEST(AuthLoginCacheProfileDtoTest, ReportsNullEncodeOutput)
{
    std::string error;

    EXPECT_FALSE(gateauthsupport::EncodeLoginCacheProfile(MakeUser(), nullptr, &error));
    EXPECT_EQ(error, "output pointer is null");
}
