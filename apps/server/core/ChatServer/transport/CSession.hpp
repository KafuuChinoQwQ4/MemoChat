#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <mutex>
#include <memory>
#include <atomic>
#include <ctime>
#include "IChatSession.hpp"
#include "IChatSessionHost.hpp"
#include "MsgNode.hpp"

namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

class LogicSystem;

class CSession
    : public IChatSession
    , public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context& io_context, IChatSessionHost* host);
    explicit CSession(IChatSessionHost* host = nullptr);
    virtual ~CSession();

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

    std::string sessionId() const override;
    int userId() const override;
    void setUserId(int uid) override;
    std::string transportName() const override;
    void send(std::string payload, short msg_id) override;
    void close() override;
    bool isHeartbeatExpired(std::time_t now) const override;
    void updateHeartbeat() override;
    bool tryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds) override;
    void markOnlineRouteRefreshed(std::time_t now) override;

protected:
    IChatSessionHost* Server() const;

private:
    std::string _session_id;
    std::atomic<IChatSessionHost*> _server; // DetachServer release-store; ReadLoop acquire-load → 无数据竞争
    int _user_uid;

    std::atomic<std::time_t> _last_heartbeat;
    std::atomic<std::time_t> _last_online_route_refresh;
};
