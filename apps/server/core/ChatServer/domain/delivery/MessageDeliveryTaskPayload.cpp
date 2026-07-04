#include "MessageDeliveryTaskPayload.hpp"

#include "json/TypedJsonCodec.hpp"

import memochat.chat.delivery_task_payload_algorithms;

#include <glaze/glaze.hpp>

namespace
{

struct MessageDeliveryTaskPayloadWireDto
{
    int recipient_uid = 0;
    int msgid = 0;
    int exclude_uid = 0;
    std::string reason;
    glz::generic_json<> payload = memochat::json::object_t{};
};

MessageDeliveryTaskPayloadWireDto ToWireDto(const memochat::chat::delivery::MessageDeliveryTaskPayload& payload)
{
    MessageDeliveryTaskPayloadWireDto dto;
    dto.recipient_uid = payload.recipient_uid;
    dto.msgid = payload.msgid;
    dto.exclude_uid = payload.exclude_uid;
    dto.reason = payload.reason;
    dto.payload = payload.payload.impl();
    return dto;
}

void FromWireDto(const MessageDeliveryTaskPayloadWireDto& dto,
                 memochat::chat::delivery::MessageDeliveryTaskPayload& payload)
{
    payload.recipient_uid = dto.recipient_uid;
    payload.msgid = dto.msgid;
    payload.exclude_uid = dto.exclude_uid;
    payload.reason = dto.reason;
    payload.payload = memochat::json::JsonValue(dto.payload);
}

} // namespace

namespace memochat::chat::delivery
{

MessageDeliveryTaskPayload::MessageDeliveryTaskPayload()
    : payload(memochat::json::object_t{})
{
}

MessageDeliveryTaskPayload::MessageDeliveryTaskPayload(const MessageDeliveryTaskPayload& other)
    : recipient_uid(other.recipient_uid)
    , msgid(other.msgid)
    , exclude_uid(other.exclude_uid)
    , reason(other.reason)
    , payload(other.payload)
{
}

MessageDeliveryTaskPayload& MessageDeliveryTaskPayload::operator=(const MessageDeliveryTaskPayload& other)
{
    if (this == &other)
    {
        return *this;
    }
    recipient_uid = other.recipient_uid;
    msgid = other.msgid;
    exclude_uid = other.exclude_uid;
    reason = other.reason;
    payload = other.payload;
    return *this;
}

MessageDeliveryTaskPayload::~MessageDeliveryTaskPayload() = default;

memochat::json::JsonValue BuildDeliveryTaskPayloadJson(int recipient_uid,
                                                       short msgid,
                                                       const memochat::json::JsonValue& payload,
                                                       int exclude_uid,
                                                       const std::string& reason)
{
    MessageDeliveryTaskPayload task;
    task.recipient_uid = recipient_uid;
    task.msgid = task_payload::modules::NormalizeTaskMessageId(msgid);
    task.exclude_uid = exclude_uid;
    task.reason = reason;
    task.payload = payload;

    std::string body;
    if (!memochat::json::WriteTypedJson(ToWireDto(task), &body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }

    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, body) || !root.isObject())
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    if (reason.empty() && root.impl().holds<memochat::json::object_t>())
    {
        root.impl().get<memochat::json::object_t>().erase("reason");
    }
    return root;
}

bool ParseDeliveryTaskPayload(const memochat::json::JsonValue& root, MessageDeliveryTaskPayload* out)
{
    const bool root_is_object = root.isObject();
    if (out == nullptr || !task_payload::modules::HasRequiredTaskPayloadShape(root_is_object, root.isMember("payload")))
    {
        return false;
    }

    const std::string body = memochat::json::glaze_stringify(root);
    MessageDeliveryTaskPayloadWireDto dto;
    if (!memochat::json::ReadTypedJson(body, &dto))
    {
        return false;
    }

    FromWireDto(dto, *out);
    return task_payload::modules::IsValidParsedDeliveryTask(out->recipient_uid, out->msgid, out->payload.isObject());
}

} // namespace memochat::chat::delivery
