export module memochat.chat.delivery_task_payload_algorithms;

export namespace memochat::chat::delivery::task_payload::modules
{
int NormalizeTaskMessageId(short msgid)
{
    return static_cast<int>(msgid);
}

bool HasRequiredTaskPayloadShape(bool root_is_object, bool has_nested_payload)
{
    return root_is_object && has_nested_payload;
}

bool IsValidParsedDeliveryTask(int recipient_uid, int msgid, bool nested_payload_is_object)
{
    return recipient_uid > 0 && msgid > 0 && nested_payload_is_object;
}
} // namespace memochat::chat::delivery::task_payload::modules
