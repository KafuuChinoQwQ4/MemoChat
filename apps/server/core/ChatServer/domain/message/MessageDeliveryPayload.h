#pragma once

#include "common/proto/message.pb.h"
#include "json/GlazeCompat.h"

#include <string>

namespace memochat::chat::delivery
{

std::string SerializeDeliveryPayloadForWire(const memochat::json::JsonValue& payload);

bool BuildPrivateTextNotifyRequest(int recipient_uid,
                                   const memochat::json::JsonValue& payload,
                                   message::TextChatMsgReq* request);

} // namespace memochat::chat::delivery
