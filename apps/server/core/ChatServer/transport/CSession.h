#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include <atomic>
#include <ctime>
#include <exec/task.hpp> // exec::task<> —— 读循环协程返回类型
#include "const.h"
#include "MsgNode.h"

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

class CServer;
class LogicSystem;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context& io_context, CServer* server);
    virtual ~CSession();
    tcp::socket& GetSocket();
    std::string& GetSessionId();
    void SetUserId(int uid);
    int GetUserId();
    virtual void Start();
    virtual void Send(char* msg, short max_length, short msgid);
    virtual void Send(std::string msg, short msgid);
    virtual void Close();
    std::shared_ptr<CSession> SharedSelf();
    void NotifyOffline(int uid);
    void DetachServer();

    bool IsHeartbeatExpired(std::time_t& now);

    void UpdateHeartbeat();
    bool TryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds);
    void MarkOnlineRouteRefreshed(std::time_t now);

    void DealExceptionSession();

private:
    exec::task<void> ReadLoop(std::shared_ptr<CSession> self); // self 传值覆盖 spawn→首行窗口

    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);
    tcp::socket _socket;
    std::string _session_id;
    char _data[MAX_LENGTH];
    std::atomic<CServer*> _server; // DetachServer release-store; ReadLoop acquire-load → 无数据竞争
    std::atomic<bool> _b_close;    // 跨线程(ReadLoop读/Close写);真正停读靠 _socket.close()
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::mutex _send_lock;

    std::shared_ptr<RecvNode> _recv_msg_node;
    bool _b_head_parse;

    std::shared_ptr<MsgNode> _recv_head_node;
    int _user_uid;

    std::atomic<std::time_t> _last_heartbeat;
    std::atomic<std::time_t> _last_online_route_refresh;

    std::mutex _session_mtx;
};

class LogicNode
{
    friend class LogicSystem;

public:
    LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);

private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recvnode;
};
