#include "CSession.hpp"
#include "CServer.hpp"
#include <iostream>
#include <sstream>
#include "json/GlazeCompat.hpp"

#include <limits>
#include "LogicSystem.hpp"
#include "RedisMgr.hpp"
#include "ConfigMgr.hpp"
#include "UserMgr.hpp"
#include "runtime/AsioCoScheduler.hpp" // memochat::runtime::IoContextScheduler

#include <exec/asio/use_sender.hpp>
#include <exec/start_detached.hpp>
#include <stdexec/execution.hpp>

// Compact wire JSON for TCP/QUIC transport (Qt QJsonDocument is strict).
static std::string JsonToWireString(const memochat::json::JsonValue& v)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, v);
}

#ifdef _WIN32
#include <ws2tcpip.h>
#include <mstcpip.h>
#else
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

CSession::CSession(boost::asio::io_context& io_context, CServer* server)
    : _socket(io_context)
    , _server(server)
    , _b_close(false)
    , _b_head_parse(false)
    , _user_uid(0)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _session_id = boost::uuids::to_string(a_uuid);
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
    _last_heartbeat = std::time(nullptr);
    _last_online_route_refresh = 0;
}
CSession::~CSession()
{
    std::cout << "~CSession destruct" << std::endl;
}

tcp::socket& CSession::GetSocket()
{
    return _socket;
}

std::string& CSession::GetSessionId()
{
    return _session_id;
}

void CSession::SetUserId(int uid)
{
    _user_uid = uid;
}

int CSession::GetUserId()
{
    return _user_uid;
}

void CSession::Start()
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

void CSession::Send(std::string msg, short msgid)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << std::endl;
        return;
    }

    const size_t payload_len = msg.size();
    if (payload_len > static_cast<size_t>(MAX_LENGTH) ||
        payload_len > static_cast<size_t>(std::numeric_limits<short>::max()))
    {
        std::cout << "session: " << _session_id << " send payload too large, size is " << payload_len << std::endl;
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
                             std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Send(char* msg, short max_length, short msgid)
{
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE)
    {
        std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << std::endl;
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
                             std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Close()
{
    std::lock_guard<std::mutex> lock(_session_mtx);
    if (_socket.is_open())
    {
        boost::system::error_code ignored;
        _socket.close(ignored);
    }
    _b_close = true;
}

void CSession::DetachServer()
{
    _server.store(nullptr, std::memory_order_release);
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
    return shared_from_this();
}

exec::task<void> CSession::ReadLoop(std::shared_ptr<CSession> self)
{
    namespace net = boost::asio;
    using exec::asio::use_sender;

    (void) self; // 帧持有 self 续命(传值参数)
    try
    {
        while (!_b_close)
        {
            // ── 读 head(定长 HEAD_TOTAL_LEN)──────────────────────────────
            co_await net::async_read(_socket,
                                     net::buffer(_data, HEAD_TOTAL_LEN),
                                     net::transfer_exactly(HEAD_TOTAL_LEN),
                                     use_sender);

            {
                auto* srv = _server.load(std::memory_order_acquire);
                if (srv == nullptr || !srv->CheckValid(_session_id))
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
                if (auto* srv = _server.load(std::memory_order_acquire); srv != nullptr)
                {
                    srv->ClearSession(_session_id);
                }
                co_return;
            }

            short msg_len = 0;
            memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
            msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
            if (msg_len > MAX_LENGTH)
            {
                std::cout << "invalid data length is " << msg_len << std::endl;
                if (auto* srv = _server.load(std::memory_order_acquire); srv != nullptr)
                {
                    srv->ClearSession(_session_id);
                }
                co_return;
            }

            _recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);

            // ── 读 body(定长 msg_len)────────────────────────────────────
            co_await net::async_read(_socket, net::buffer(_data, msg_len), net::transfer_exactly(msg_len), use_sender);

            {
                auto* srv = _server.load(std::memory_order_acquire);
                if (srv == nullptr || !srv->CheckValid(_session_id))
                {
                    Close();
                    co_return;
                }
            }

            memcpy(_recv_msg_node->_data, _data, msg_len);
            _recv_msg_node->_cur_len += msg_len;
            _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
            UpdateHeartbeat();

            LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
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

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self)
{
    try
    {
        auto self = shared_from_this();
        if (!error)
        {
            std::lock_guard<std::mutex> lock(_send_lock);
            // cout << "send data " << _send_que.front()->_data+HEAD_LENGTH << std::endl;
            _send_que.pop();
            if (!_send_que.empty())
            {
                auto& msgnode = _send_que.front();
                boost::asio::async_write(_socket,
                                         boost::asio::buffer(msgnode->_data, msgnode->_total_len),
                                         std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_self));
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

void CSession::NotifyOffline(int uid)
{
    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;

    Send(JsonToWireString(rtvalue), ID_NOTIFY_OFF_LINE_REQ);
    return;
}

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
    : _session(session)
    , _recvnode(recvnode)
{
}

bool CSession::IsHeartbeatExpired(std::time_t& now)
{
    constexpr double kHeartbeatExpireSeconds = 45.0;
    double diff_sec = std::difftime(now, _last_heartbeat);
    if (diff_sec > kHeartbeatExpireSeconds)
    {
        std::cout << "heartbeat expired, session id is  " << _session_id << std::endl;
        return true;
    }

    return false;
}

void CSession::UpdateHeartbeat()
{
    time_t now = std::time(nullptr);
    _last_heartbeat = now;
}

bool CSession::TryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds)
{
    if (interval_seconds <= 0)
    {
        _last_online_route_refresh.store(now, std::memory_order_relaxed);
        return true;
    }

    auto last = _last_online_route_refresh.load(std::memory_order_relaxed);
    while (last <= 0 || std::difftime(now, last) >= interval_seconds)
    {
        if (_last_online_route_refresh.compare_exchange_weak(last,
                                                             now,
                                                             std::memory_order_relaxed,
                                                             std::memory_order_relaxed))
        {
            return true;
        }
    }
    return false;
}

void CSession::MarkOnlineRouteRefreshed(std::time_t now)
{
    _last_online_route_refresh.store(now, std::memory_order_relaxed);
}

void CSession::DealExceptionSession()
{
    auto self = shared_from_this();

    auto uid_str = std::to_string(_user_uid);
    auto lock_key = LOCK_PREFIX + uid_str;
    auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
    Defer defer(
        [identifier, lock_key, self, this]()
        {
            if (auto* srv = _server.load(std::memory_order_acquire); srv != nullptr)
            {
                srv->ClearSession(_session_id);
            }
            else if (_user_uid > 0)
            {
                UserMgr::GetInstance()->RmvUserSession(_user_uid, _session_id);
            }
            RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
        });

    if (identifier.empty())
    {
        return;
    }
    std::string redis_session_id = "";
    auto bsuccess = RedisMgr::GetInstance()->Get(USER_SESSION_PREFIX + uid_str, redis_session_id);
    if (!bsuccess)
    {
        return;
    }

    if (redis_session_id != _session_id)
    {
        return;
    }

    auto& cfg = ConfigMgr::Inst();
    RedisMgr::GetInstance()->SRem(std::string(SERVER_ONLINE_USERS_PREFIX) + cfg["SelfServer"]["Name"], uid_str);
    RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);

    RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
}
