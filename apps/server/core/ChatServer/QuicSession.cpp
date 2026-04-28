#include "QuicSession.h"

#include "LogicSystem.h"
#include "MsgNode.h"

#include <cstring>
#include <iostream>

QuicSession::QuicSession(boost::asio::io_context& io_context,
                         SendCallback send_callback,
                         CloseCallback close_callback)
    : CSession(io_context, nullptr),
      _send_callback(std::move(send_callback)),
      _close_callback(std::move(close_callback))
{
}

QuicSession::~QuicSession() = default;

void QuicSession::Start()
{
}

void QuicSession::Send(std::string msg, short msgid)
{
    if (_closed) {
        return;
    }
    if (!_send_callback || !_send_callback(msg, msgid)) {
        std::cout << "quic session send failed, session id is " << GetSessionId() << std::endl;
        Close();
    }
}

void QuicSession::Send(char* msg, short max_length, short msgid)
{
    if (msg == nullptr || max_length <= 0) {
        return;
    }
    Send(std::string(msg, msg + max_length), msgid);
}

void QuicSession::Close()
{
    if (_closed) {
        return;
    }
    _closed = true;
    if (_close_callback) {
        _close_callback(GetSessionId());
    }
    DealExceptionSession();
}

void QuicSession::HandleInboundMessage(short msgid, const std::string& payload)
{
    auto recv_node = std::make_shared<RecvNode>(static_cast<short>(payload.size()), msgid);
    if (!payload.empty()) {
        std::memcpy(recv_node->_data, payload.data(), payload.size());
    }
    recv_node->_cur_len = static_cast<short>(payload.size());
    recv_node->_data[recv_node->_total_len] = '\0';
    UpdateHeartbeat();
    LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(SharedSelf(), recv_node));
}
