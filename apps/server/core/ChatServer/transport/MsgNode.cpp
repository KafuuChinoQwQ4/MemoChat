#include "MsgNode.hpp"

import memochat.chat.msg_node_algorithms;

namespace msg_node_modules = memochat::chat::transport::msg_node::modules;

RecvNode::RecvNode(short max_len, short msg_id)
    : MsgNode(max_len)
    , _msg_id(msg_id)
{
}

SendNode::SendNode(const char* msg, short max_len, short msg_id)
    : MsgNode(static_cast<short>(
          msg_node_modules::SendBufferTotalLength(max_len,
                                                  msg_node_modules::HeaderTotalLength(HEAD_ID_LEN, HEAD_DATA_LEN))))
    , _msg_id(msg_id)
{
    short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(_data + msg_node_modules::MessageIdOffset(), &msg_id_host, HEAD_ID_LEN);

    short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(_data + msg_node_modules::MessageLengthOffset(HEAD_ID_LEN), &max_len_host, HEAD_DATA_LEN);
    memcpy(_data + msg_node_modules::PayloadOffset(HEAD_ID_LEN, HEAD_DATA_LEN), msg, max_len);
}
