#pragma once

#include "json/GlazeCompat.hpp"

#include <string>

namespace memochat::chat::delivery
{

struct MessageDeliveryTaskPayload
{
    int recipient_uid = 0;
    int msgid = 0;
    int exclude_uid = 0;
    std::string reason;
    memochat::json::JsonValue payload;

    MessageDeliveryTaskPayload();
    MessageDeliveryTaskPayload(const MessageDeliveryTaskPayload& other);
    MessageDeliveryTaskPayload& operator=(const MessageDeliveryTaskPayload& other);
    ~MessageDeliveryTaskPayload();
};

memochat::json::JsonValue BuildDeliveryTaskPayloadJson(int recipient_uid,
                                                       short msgid,
                                                       const memochat::json::JsonValue& payload,
                                                       int exclude_uid,
                                                       const std::string& reason);

bool ParseDeliveryTaskPayload(const memochat::json::JsonValue& root, MessageDeliveryTaskPayload* out);

} // namespace memochat::chat::delivery
