#include <gtest/gtest.h>

#include "json/TypedJsonCodec.h"

#include <string>

namespace
{

struct TypedJsonCodecSample
{
    int uid = 0;
    std::string name;
    bool enabled = false;
};

} // namespace

template <> struct glz::meta<TypedJsonCodecSample>
{
    using T = TypedJsonCodecSample;
    static constexpr auto value = glz::object("uid", &T::uid, "name", &T::name, "enabled", &T::enabled);
};

TEST(TypedJsonCodecTest, WritesTypedJsonWithGlazeMetadata)
{
    TypedJsonCodecSample sample;
    sample.uid = 42;
    sample.name = "alice";
    sample.enabled = true;

    std::string body;
    ASSERT_TRUE(memochat::json::WriteTypedJson(sample, &body));

    EXPECT_EQ(body, R"({"uid":42,"name":"alice","enabled":true})");
}

TEST(TypedJsonCodecTest, ReadsTypedJsonWithGlazeMetadata)
{
    TypedJsonCodecSample sample;
    std::string error;

    ASSERT_TRUE(memochat::json::ReadTypedJson(R"({"uid":7,"name":"bob","enabled":false})", &sample, &error))
        << error;

    EXPECT_EQ(sample.uid, 7);
    EXPECT_EQ(sample.name, "bob");
    EXPECT_FALSE(sample.enabled);
}

TEST(TypedJsonCodecTest, ReportsParseAndPointerFailures)
{
    TypedJsonCodecSample sample;
    std::string error;

    EXPECT_FALSE(memochat::json::ReadTypedJson("not-json", &sample, &error));
    EXPECT_FALSE(error.empty());

    error.clear();
    EXPECT_FALSE(memochat::json::ReadTypedJson(R"({"uid":1})", static_cast<TypedJsonCodecSample*>(nullptr), &error));
    EXPECT_EQ(error, "output pointer is null");

    error.clear();
    EXPECT_FALSE(memochat::json::WriteTypedJson(sample, nullptr, &error));
    EXPECT_EQ(error, "output pointer is null");
}
