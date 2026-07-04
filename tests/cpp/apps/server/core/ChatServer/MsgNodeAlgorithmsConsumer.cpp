import memochat.chat.msg_node_algorithms;

int MemoChatTestMsgNodeHeaderTotalLength(int id_len, int data_len)
{
    return memochat::chat::transport::msg_node::modules::HeaderTotalLength(id_len, data_len);
}

int MemoChatTestMsgNodeSendBufferTotalLength(int payload_len, int header_len)
{
    return memochat::chat::transport::msg_node::modules::SendBufferTotalLength(payload_len, header_len);
}

int MemoChatTestMsgNodeMessageIdOffset()
{
    return memochat::chat::transport::msg_node::modules::MessageIdOffset();
}

int MemoChatTestMsgNodeMessageLengthOffset(int id_len)
{
    return memochat::chat::transport::msg_node::modules::MessageLengthOffset(id_len);
}

int MemoChatTestMsgNodePayloadOffset(int id_len, int data_len)
{
    return memochat::chat::transport::msg_node::modules::PayloadOffset(id_len, data_len);
}
