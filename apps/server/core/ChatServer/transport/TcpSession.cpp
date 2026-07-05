#include "TcpSession.hpp"

#include "CServer.hpp"
#include "ChatFrameDispatch.hpp"
#include "runtime/AsioCoScheduler.hpp"

#include <exec/asio/use_sender.hpp>
#include <exec/start_detached.hpp>
#include <stdexec/execution.hpp>

#include <iostream>
#include <limits>

#ifdef _WIN32
#include <mstcpip.h>
#include <ws2tcpip.h>
#else
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

TcpSession::TcpSession(boost::asio::io_context& io_context, IChatSessionHost* host)
    : CSession(host)
    , _socket(io_context)
    , _b_close(false)
{
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

TcpSession::~TcpSession() = default;

tcp::socket& TcpSession::GetSocket()
{
    return _socket;
}

void TcpSession::Start()
{
#ifdef _WIN32
    auto native_sock = _socket.native_handle();
    int keepalive = 1;
    ::setsockopt(native_sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&keepalive), sizeof(keepalive));
    struct tcp_keepalive alive_out{};
    alive_out.onoff = 1;
    alive_out.keepalivetime = 60000;
    alive_out.keepaliveinterval = 10000;
    DWORD bytes_returned = 0;
    ::WSAIoctl(native_sock,
               SIO_KEEPALIVE_VALS,
               &alive_out,
               sizeof(alive_out),
               nullptr,
               0,
               &bytes_returned,
               nullptr,
               nullptr);
#else
    int keepalive = 1;
    int keepidle = 60;
    int keepcnt = 3;
    int keepintvl = 10;
    auto native_sock = _socket.native_handle();
    ::setsockopt(native_sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
    ::setsockopt(native_sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
    ::setsockopt(native_sock, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
    ::setsockopt(native_sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
#endif
    auto& ioc = static_cast<boost::asio::io_context&>(_socket.get_executor().context());
    exec::start_detached(stdexec::on(memochat::runtime::IoContextScheduler{&ioc}, ReadLoop(shared_from_this())));
}

void TcpSession::Send(std::string msg, short msgid)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = static_cast<int>(_send_que.size());
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << GetSessionId() << " send que fulled, size is " << MAX_SENDQUE << std::endl;
        return;
    }

    const size_t payload_len = msg.size();
    if (payload_len > static_cast<size_t>(MAX_LENGTH) ||
        payload_len > static_cast<size_t>(std::numeric_limits<short>::max()))
    {
        std::cout << "session: " << GetSessionId() << " send payload too large, size is " << payload_len << std::endl;
        return;
    }

    const short payload_len_short = static_cast<short>(payload_len);
    _send_que.push(std::make_shared<SendNode>(msg.c_str(), payload_len_short, msgid));
    if (send_que_size > 0)
    {
        return;
    }
    auto& msgnode = _send_que.front();
    boost::asio::async_write(_socket,
                             boost::asio::buffer(msgnode->_data, msgnode->_total_len),
                             std::bind(&TcpSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void TcpSession::Send(char* msg, short max_length, short msgid)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = static_cast<int>(_send_que.size());
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << GetSessionId() << " send que fulled, size is " << MAX_SENDQUE << std::endl;
        return;
    }

    if (msg == nullptr || max_length <= 0 || max_length > MAX_LENGTH ||
        static_cast<int>(max_length) + HEAD_TOTAL_LEN > std::numeric_limits<short>::max())
    {
        std::cout << "session: " << GetSessionId() << " send payload invalid, size is " << max_length << std::endl;
        return;
    }

    _send_que.push(std::make_shared<SendNode>(msg, max_length, msgid));
    if (send_que_size > 0)
    {
        return;
    }
    auto& msgnode = _send_que.front();
    boost::asio::async_write(_socket,
                             boost::asio::buffer(msgnode->_data, msgnode->_total_len),
                             std::bind(&TcpSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void TcpSession::Close()
{
    std::lock_guard<std::mutex> lock(_session_mtx);
    if (_socket.is_open())
    {
        boost::system::error_code ignored;
        _socket.close(ignored);
    }
    _b_close = true;
}

std::string TcpSession::transportName() const
{
    return "tcp";
}

exec::task<void> TcpSession::ReadLoop(std::shared_ptr<CSession> self)
{
    namespace net = boost::asio;
    using exec::asio::use_sender;

    try
    {
        while (!_b_close)
        {
            co_await net::async_read(_socket,
                                     net::buffer(_data, HEAD_TOTAL_LEN),
                                     net::transfer_exactly(HEAD_TOTAL_LEN),
                                     use_sender);

            {
                auto* srv = Server();
                if (srv == nullptr || !srv->CheckValid(GetSessionId()))
                {
                    Close();
                    co_return;
                }
            }

            _recv_head_node->Clear();
            memcpy(_recv_head_node->_data, _data, HEAD_TOTAL_LEN);

            short msg_id = 0;
            memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
            msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
            if (msg_id > MAX_LENGTH)
            {
                std::cout << "invalid msg_id is " << msg_id << std::endl;
                if (auto* srv = Server(); srv != nullptr)
                {
                    srv->ClearSession(GetSessionId());
                }
                co_return;
            }

            short msg_len = 0;
            memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
            msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
            if (msg_len > MAX_LENGTH)
            {
                std::cout << "invalid data length is " << msg_len << std::endl;
                if (auto* srv = Server(); srv != nullptr)
                {
                    srv->ClearSession(GetSessionId());
                }
                co_return;
            }

            co_await net::async_read(_socket, net::buffer(_data, msg_len), net::transfer_exactly(msg_len), use_sender);

            {
                auto* srv = Server();
                if (srv == nullptr || !srv->CheckValid(GetSessionId()))
                {
                    Close();
                    co_return;
                }
            }

            memochat::chatserver::transport::PostInboundChatFrame(
                self,
                msg_id,
                std::string_view(_data, static_cast<std::size_t>(msg_len)));
        }
    }
    catch (const boost::system::system_error& e)
    {
        std::cout << "handle read failed, error is " << e.what() << std::endl;
        Close();
        DealExceptionSession();
    }
    catch (const std::exception& e)
    {
        std::cout << "ReadLoop exception is " << e.what() << std::endl;
        Close();
    }
    co_return;
}

void TcpSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self)
{
    try
    {
        auto self = shared_from_this();
        (void) self;
        if (!error)
        {
            std::lock_guard<std::mutex> lock(_send_lock);
            _send_que.pop();
            if (!_send_que.empty())
            {
                auto& msgnode = _send_que.front();
                boost::asio::async_write(
                    _socket,
                    boost::asio::buffer(msgnode->_data, msgnode->_total_len),
                    std::bind(&TcpSession::HandleWrite, this, std::placeholders::_1, std::move(shared_self)));
            }
        }
        else
        {
            std::cout << "handle write failed, error is " << error.what() << std::endl;
            Close();
            DealExceptionSession();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception code : " << e.what() << std::endl;
    }
}
