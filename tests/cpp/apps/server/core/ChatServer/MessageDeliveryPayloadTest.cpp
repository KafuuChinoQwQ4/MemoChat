#include <gtest/gtest.h>

#include "MessageDeliveryPayload.h"
#include "json/GlazeCompat.h"

TEST(MessageDeliveryPayloadTest, SerializeDeliveryPayloadForWireKeepsJsonObjectShape)
{
    memochat::json::JsonValue payload(memochat::json::objectValue);
    payload["error"] = 0;
    payload["fromuid"] = 1;
    payload["touid"] = 2;
    payload["text_array"] = memochat::json::arrayValue;

    memochat::json::JsonValue message(memochat::json::objectValue);
    message["msgid"] = "m1";
    message["content"] = "hello";
    payload["text_array"].append(message);

    const std::string wire = memochat::chat::delivery::SerializeDeliveryPayloadForWire(payload);

    ASSERT_FALSE(wire.empty());
    EXPECT_EQ(wire.front(), '{') << wire;
    EXPECT_EQ(wire.find("\\\"error\\\""), std::string::npos) << wire;

    memochat::json::JsonValue parsed;
    ASSERT_TRUE(memochat::json::reader_parse(wire, parsed)) << wire;
    EXPECT_TRUE(parsed.isObject()) << wire;
    const memochat::json::JsonValue messages = parsed["text_array"];
    ASSERT_TRUE(messages.isArray()) << wire;
    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0]["msgid"].asString(), "m1");
}

TEST(MessageDeliveryPayloadTest, BuildPrivateTextNotifyRequestUsesRecipientAndTextArray)
{
    memochat::json::JsonValue payload(memochat::json::objectValue);
    payload["fromuid"] = 10;
    payload["touid"] = 20;
    payload["text_array"] = memochat::json::arrayValue;

    memochat::json::JsonValue message(memochat::json::objectValue);
    message["msgid"] = "m-private";
    message["content"] = "hello private";
    payload["text_array"].append(message);

    message::TextChatMsgReq request;
    ASSERT_TRUE(memochat::chat::delivery::BuildPrivateTextNotifyRequest(21, payload, &request));

    EXPECT_EQ(request.fromuid(), 10);
    EXPECT_EQ(request.touid(), 21);
    ASSERT_EQ(request.textmsgs_size(), 1);
    EXPECT_EQ(request.textmsgs(0).msgid(), "m-private");
    EXPECT_EQ(request.textmsgs(0).msgcontent(), "hello private");
}

TEST(MessageDeliveryPayloadTest, BuildPrivateTextNotifyRequestRejectsStringPayloadShape)
{
    memochat::json::JsonValue payload("not an object");
    message::TextChatMsgReq request;

    EXPECT_FALSE(memochat::chat::delivery::BuildPrivateTextNotifyRequest(21, payload, &request));
    EXPECT_EQ(request.textmsgs_size(), 0);
}
