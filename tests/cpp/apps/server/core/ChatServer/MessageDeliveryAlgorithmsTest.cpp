#include <gtest/gtest.h>

#include "MessageDeliveryPayload.hpp"
#include "json/GlazeCompat.hpp"

TEST(MessageDeliveryAlgorithmsTest, PrivateNotifyRecipientUsesExplicitRecipientWhenPresent)
{
    memochat::json::JsonValue payload(memochat::json::objectValue);
    payload["fromuid"] = 10;
    payload["touid"] = 20;
    payload["text_array"] = memochat::json::arrayValue;

    memochat::json::JsonValue message(memochat::json::objectValue);
    message["msgid"] = "m-explicit";
    message["content"] = "hello";
    payload["text_array"].append(message);

    message::TextChatMsgReq request;
    ASSERT_TRUE(memochat::chat::delivery::BuildPrivateTextNotifyRequest(21, payload, &request));
    EXPECT_EQ(request.touid(), 21);
}

TEST(MessageDeliveryAlgorithmsTest, PrivateNotifyRecipientFallsBackToPayloadRecipient)
{
    memochat::json::JsonValue payload(memochat::json::objectValue);
    payload["fromuid"] = 10;
    payload["touid"] = 20;
    payload["text_array"] = memochat::json::arrayValue;

    memochat::json::JsonValue message(memochat::json::objectValue);
    message["msgid"] = "m-fallback";
    message["content"] = "hello";
    payload["text_array"].append(message);

    message::TextChatMsgReq request;
    ASSERT_TRUE(memochat::chat::delivery::BuildPrivateTextNotifyRequest(0, payload, &request));
    EXPECT_EQ(request.touid(), 20);
}

TEST(MessageDeliveryAlgorithmsTest, PrivateNotifySkipsInvalidTextItems)
{
    memochat::json::JsonValue payload(memochat::json::objectValue);
    payload["fromuid"] = 10;
    payload["touid"] = 20;
    payload["text_array"] = memochat::json::arrayValue;

    memochat::json::JsonValue missing_content(memochat::json::objectValue);
    missing_content["msgid"] = "m-missing-content";
    payload["text_array"].append(missing_content);

    memochat::json::JsonValue valid(memochat::json::objectValue);
    valid["msgid"] = "m-valid";
    valid["content"] = "hello";
    payload["text_array"].append(valid);

    message::TextChatMsgReq request;
    ASSERT_TRUE(memochat::chat::delivery::BuildPrivateTextNotifyRequest(0, payload, &request));
    ASSERT_EQ(request.textmsgs_size(), 1);
    EXPECT_EQ(request.textmsgs(0).msgid(), "m-valid");
}
