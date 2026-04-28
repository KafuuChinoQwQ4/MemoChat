#include "UserMgr.h"
#include "CSession.h"
#include "RedisMgr.h"
#include "logging/Logger.h"

UserMgr:: ~ UserMgr(){
	_uid_to_session.clear();
}


std::shared_ptr<CSession> UserMgr::GetSession(int uid)
{
	if (uid <= 0) {
		memolog::LogWarn("usermgr.get_session.invalid", "invalid uid",
			{{"uid", std::to_string(uid)}});
		return nullptr;
	}
	std::lock_guard<std::mutex> lock(_session_mtx);
	auto iter = _uid_to_session.find(uid);
	if (iter == _uid_to_session.end()) {
		memolog::LogInfo("usermgr.get_session", "session not found in local map",
			{{"uid", std::to_string(uid)}, {"map_size", std::to_string(_uid_to_session.size())}});
		return nullptr;
	}

	// 检查 session 是否有效
	if (!iter->second) {
		memolog::LogWarn("usermgr.get_session", "session is null",
			{{"uid", std::to_string(uid)}, {"map_size", std::to_string(_uid_to_session.size())}});
		_uid_to_session.erase(iter);
		return nullptr;
	}

	memolog::LogInfo("usermgr.get_session", "session found in local map",
		{{"uid", std::to_string(uid)}, {"session_id", iter->second->GetSessionId()}});
	return iter->second;
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session)
{
	if (uid <= 0 || !session) {
		memolog::LogWarn("usermgr.set_session.invalid", "invalid params",
			{{"uid", std::to_string(uid)}});
		return;
	}
	std::lock_guard<std::mutex> lock(_session_mtx);
	memolog::LogInfo("usermgr.set_session", "setting session",
		{{"uid", std::to_string(uid)}, {"session_id", session->GetSessionId()}});
	_uid_to_session[uid] = session;
}

void UserMgr::RmvUserSession(int uid, std::string session_id)
{ 
	if (uid <= 0 || session_id.empty()) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(_session_mtx);
		memolog::LogInfo("usermgr.rmv_session", "removing session",
			{{"uid", std::to_string(uid)}, {"session_id", session_id}});
		auto iter = _uid_to_session.find(uid);
		if (iter == _uid_to_session.end()) {
			memolog::LogInfo("usermgr.rmv_session", "session not found",
				{{"uid", std::to_string(uid)}});
			return;
		}
	
		auto session_id_ = iter->second->GetSessionId();

		if (session_id_ != session_id) {
			memolog::LogInfo("usermgr.rmv_session", "session id mismatch",
				{{"uid", std::to_string(uid)}, {"expected", session_id}, {"actual", session_id_}});
			return;
		}
		_uid_to_session.erase(uid);
		memolog::LogInfo("usermgr.rmv_session", "session removed",
			{{"uid", std::to_string(uid)}});
	}

}

UserMgr::UserMgr()
{

}
