export module memochat.chat.msg_node_algorithms;

export namespace memochat::chat::transport::msg_node::modules
{
int HeaderTotalLength(int id_len, int data_len)
{
    return id_len + data_len;
}

int SendBufferTotalLength(int payload_len, int header_len)
{
    return payload_len + header_len;
}

int MessageIdOffset()
{
    return 0;
}

int MessageLengthOffset(int id_len)
{
    return id_len;
}

int PayloadOffset(int id_len, int data_len)
{
    return id_len + data_len;
}
} // namespace memochat::chat::transport::msg_node::modules
