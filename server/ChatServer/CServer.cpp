#include "CServer.h"
#include <iostream>
#include "AsioIOServicePool.h"
#include "UserMgr.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"

CServer::CServer(boost::asio::io_context& io_context, short port):_io_context(io_context), _port(port),
_acceptor(io_context, tcp::endpoint(tcp::v4(),port)), _timer(_io_context, std::chrono::seconds(60))
{
	cout << "Server start success, listen on port : " << _port << endl;

	StartAccept();
}

CServer::~CServer() {
	cout << "Server destruct listen on port : " << _port << endl;
	
}

void CServer::HandleAccept(shared_ptr<CSession> new_session, const boost::system::error_code& error){
	if (!error) {
		new_session->Start();
		lock_guard<mutex> lock(_mutex);
		_sessions.insert(make_pair(new_session->GetSessionId(), new_session));
	}
	else {
		cout << "session accept failed, error is " << error.what() << endl;
	}

	StartAccept();
}

void CServer::StartAccept() {
	auto &io_context = AsioIOServicePool::GetInstance()->GetIOService();
	shared_ptr<CSession> new_session = make_shared<CSession>(io_context, this);
	_acceptor.async_accept(new_session->GetSocket(), std::bind(&CServer::HandleAccept, this, new_session, placeholders::_1));
}


void CServer::ClearSession(std::string session_id) {
	
	lock_guard<mutex> lock(_mutex);
	if (_sessions.find(session_id) != _sessions.end()) {
		auto uid = _sessions[session_id]->GetUserId();


		UserMgr::GetInstance()->RmvUserSession(uid, session_id);
	}

	_sessions.erase(session_id);
	
}


shared_ptr<CSession> CServer::GetSession(std::string uuid) {
	lock_guard<mutex> lock(_mutex);
	auto it = _sessions.find(uuid);
	if (it != _sessions.end()) {
		return it->second;
	}
	return nullptr;
}

bool CServer::CheckValid(std::string uuid)
{
	lock_guard<mutex> lock(_mutex);
	auto it = _sessions.find(uuid);
	if (it != _sessions.end()) {
		return true;
	}
	return false;
}

void CServer::on_timer(const boost::system::error_code& ec) {
	if (ec) {
		std::cout << "timer error: " << ec.message() << std::endl;
		return;
	}
	std::vector<std::pair<std::string, std::weak_ptr<CSession>>> expired;
	int session_count = 0;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		for (const auto& [id, session_sp] : _sessions) {
			time_t now = std::time(nullptr);
			if (session_sp->IsHeartbeatExpired(now)) {
				expired.emplace_back(id, session_sp);
			} else {
				session_count++;
			}
		}
	}

	for (auto& [id, wp] : expired) {
		if (auto sp = wp.lock()) {
			sp->Close();
			sp->DealExceptionSession();
		}
	}

	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];
	auto count_str = std::to_string(session_count);
	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, count_str);

	_timer.expires_after(std::chrono::seconds(60));
	_timer.async_wait([this](boost::system::error_code ec) {
		on_timer(ec);
	});
}

void CServer::StartTimer()
{

	auto self(shared_from_this());
	_timer.async_wait([self](boost::system::error_code ec) {
		self->on_timer(ec);
		});
}

void CServer::StopTimer()
{
	_timer.cancel();
}
