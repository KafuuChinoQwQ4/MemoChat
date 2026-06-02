#include <gtest/gtest.h>

#include "json/GlazeCompat.h"

TEST(GlazeCompatTest, JsonValueProxyIsMemberChecksRequestedKey)
{
    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::reader_parse(std::string(R"({"msg":{"content":"hello"}})"), root));

    auto msg = root["msg"];

    EXPECT_TRUE(memochat::json::isMember(msg, "content"));
    EXPECT_FALSE(memochat::json::isMember(msg, "mentions"));
}

TEST(GlazeCompatTest, AppendToObjectMemberProxyMutatesParentArray)
{
    memochat::json::JsonValue root;
    root["messages"] = memochat::json::arrayValue;

    memochat::json::JsonValue item;
    item["content"] = "hello";

    append(root["messages"], item);

    const auto messages = root["messages"].get<memochat::json::JsonValue>();
    ASSERT_TRUE(messages.isArray());
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0]["content"].asString(), "hello");
}
