#include "MessageDeliveryPayload.h"

namespace memochat::chat::delivery
{

std::string SerializeDeliveryPayloadForWire(const memochat::json::JsonValue& payload)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, payload);
}

bool BuildPrivateTextNotifyRequest(int recipient_uid,
                                   const memochat::json::JsonValue& payload,
                                   message::TextChatMsgReq* request)
{
    if (request == nullptr)
    {
        return false;
    }

    request->Clear();
    if (!payload.isObject())
    {
        return false;
    }

    const int from_uid = payload.get("fromuid", 0).asInt();
    const int to_uid = recipient_uid > 0 ? recipient_uid : payload.get("touid", 0).asInt();
    const memochat::json::JsonValue text_array = payload["text_array"];
    if (from_uid <= 0 || to_uid <= 0 || !text_array.isArray() || text_array.empty())
    {
        return false;
    }

    request->set_fromuid(from_uid);
    request->set_touid(to_uid);
    for (const auto& item : text_array)
    {
        const std::string msg_id = item.get("msgid", "").asString();
        const std::string content = item.get("content", "").asString();
        if (msg_id.empty() || content.empty())
        {
            continue;
        }
        auto* text_msg = request->add_textmsgs();
        text_msg->set_msgid(msg_id);
        text_msg->set_msgcontent(content);
    }

    if (request->textmsgs_size() == 0)
    {
        request->Clear();
        return false;
    }
    return true;
}

} // namespace memochat::chat::delivery
