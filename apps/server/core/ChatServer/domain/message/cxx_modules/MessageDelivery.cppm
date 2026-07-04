export module memochat.chat.message_delivery_algorithms;

export namespace memochat::chat::delivery::modules
{
int SelectPrivateNotifyRecipient(int explicit_recipient_uid, int payload_recipient_uid)
{
    if (explicit_recipient_uid > 0)
    {
        return explicit_recipient_uid;
    }
    return payload_recipient_uid;
}

bool HasValidPrivateNotifyEnvelope(bool payload_is_object, int from_uid, int to_uid, bool text_array_is_non_empty)
{
    return payload_is_object && from_uid > 0 && to_uid > 0 && text_array_is_non_empty;
}

bool IsValidPrivateTextItem(bool has_msg_id, bool has_content)
{
    return has_msg_id && has_content;
}
} // namespace memochat::chat::delivery::modules
