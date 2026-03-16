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
#include "const.h"
#include "MsgNode.h"
using namespace std;


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


class CServer;
class LogicSystem;

class CSession: public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	virtual ~CSession();
	tcp::socket& GetSocket();
	std::string& GetSessionId();
	void SetUserId(int uid);
	int GetUserId();
	virtual void Start();
	virtual void Send(char* msg,  short max_length, short msgid);
	virtual void Send(std::string msg, short msgid);
	virtual void Close();
	std::shared_ptr<CSession> SharedSelf();
	void AsyncReadBody(int length);
	void AsyncReadHead(int total_len);
	void NotifyOffline(int uid);

	bool IsHeartbeatExpired(std::time_t& now);

	void UpdateHeartbeat();

	void DealExceptionSession();
private:
	void asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code& , std::size_t)> handler);
	void asyncReadLen(std::size_t  read_len, std::size_t total_len,
		std::function<void(const boost::system::error_code&, std::size_t)> handler);
	
	
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);
	tcp::socket _socket;
	std::string _session_id;
	char _data[MAX_LENGTH];
	CServer* _server;
	bool _b_close;
	std::queue<shared_ptr<SendNode> > _send_que;
	std::mutex _send_lock;

	std::shared_ptr<RecvNode> _recv_msg_node;
	bool _b_head_parse;

	std::shared_ptr<MsgNode> _recv_head_node;
	int _user_uid;

	std::atomic<time_t> _last_heartbeat;

	std::mutex _session_mtx;
};

class LogicNode {
	friend class LogicSystem;
public:
	LogicNode(shared_ptr<CSession>, shared_ptr<RecvNode>);
private:
	shared_ptr<CSession> _session;
	shared_ptr<RecvNode> _recvnode;
};
