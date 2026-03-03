#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "ChatGrpcClient.h"
#include "DistLock.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include "CServer.h"
using namespace std;

LogicSystem::LogicSystem():_b_stop(false), _p_server(nullptr){
	RegisterCallBacks();
	_worker_thread = std::thread (&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem(){
	_b_stop = true;
	_consume.notify_one();
	_worker_thread.join();
}

void LogicSystem::PostMsgToQue(shared_ptr < LogicNode> msg) {
	std::unique_lock<std::mutex> unique_lk(_mutex);
	_msg_que.push(msg);

	if (_msg_que.size() == 1) {
		unique_lk.unlock();
		_consume.notify_one();
	}
}


void LogicSystem::SetServer(std::shared_ptr<CServer> pserver) {
	_p_server = pserver;
}


void LogicSystem::DealMsg() {
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);

		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk);
		}


		if (_b_stop ) {
			while (!_msg_que.empty()) {
				auto msg_node = _msg_que.front();
				cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
				auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
				if (call_back_iter == _fun_callbacks.end()) {
					_msg_que.pop();
					continue;
				}
				call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
					std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
				_msg_que.pop();
			}
			break;
		}


		auto msg_node = _msg_que.front();
		cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == _fun_callbacks.end()) {
			_msg_que.pop();
			std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
			continue;
		}
		call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id, 
			std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
		_msg_que.pop();
	}
}

void LogicSystem::RegisterCallBacks() {
	_fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfo, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_HEART_BEAT_REQ] = std::bind(&LogicSystem::HeartBeatHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_CREATE_GROUP_REQ] = std::bind(&LogicSystem::CreateGroupHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_GET_GROUP_LIST_REQ] = std::bind(&LogicSystem::GetGroupListHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_GET_DIALOG_LIST_REQ] = std::bind(&LogicSystem::GetDialogListHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_INVITE_GROUP_MEMBER_REQ] = std::bind(&LogicSystem::InviteGroupMemberHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_APPLY_JOIN_GROUP_REQ] = std::bind(&LogicSystem::ApplyJoinGroupHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_REVIEW_GROUP_APPLY_REQ] = std::bind(&LogicSystem::ReviewGroupApplyHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_GROUP_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealGroupChatMsg, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_GROUP_HISTORY_REQ] = std::bind(&LogicSystem::GroupHistoryHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_PRIVATE_HISTORY_REQ] = std::bind(&LogicSystem::PrivateHistoryHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_UPDATE_GROUP_ANNOUNCEMENT_REQ] = std::bind(&LogicSystem::UpdateGroupAnnouncementHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_UPDATE_GROUP_ICON_REQ] = std::bind(&LogicSystem::UpdateGroupIconHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_SET_GROUP_ADMIN_REQ] = std::bind(&LogicSystem::SetGroupAdminHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_MUTE_GROUP_MEMBER_REQ] = std::bind(&LogicSystem::MuteGroupMemberHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_KICK_GROUP_MEMBER_REQ] = std::bind(&LogicSystem::KickGroupMemberHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[ID_QUIT_GROUP_REQ] = std::bind(&LogicSystem::QuitGroupHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
	
}

void LogicSystem::LoginHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	const int protocol_version = root.get("protocol_version", 1).asInt();
	auto trace_id = root.get("trace_id", "").asString();
	if (trace_id.empty()) {
		trace_id = memolog::TraceContext::NewId();
	}
	memolog::TraceContext::SetTraceId(trace_id);
	memolog::TraceContext::SetRequestId(memolog::TraceContext::NewId());
	memolog::TraceContext::SetUid(std::to_string(uid));
	memolog::TraceContext::SetSessionId(session->GetSessionId());
	Defer clear_trace([]() {
		memolog::TraceContext::Clear();
		});
	memolog::LogInfo("chat.login.received", "chat login request received",
		{
			{"uid", std::to_string(uid)},
			{"session_id", session->GetSessionId()},
			{"tcp_msg_id", std::to_string(msg_id)}
		});

	Json::Value  rtvalue;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
		});
	rtvalue["trace_id"] = trace_id;
	rtvalue["protocol_version"] = 2;

	if (protocol_version != 2) {
		rtvalue["error"] = ErrorCodes::ProtocolVersionMismatch;
		return;
	}



	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (!success) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		memolog::LogWarn("chat.login.failed", "uid invalid", { {"error_code", std::to_string(ErrorCodes::UidInvalid)} });
		return ;
	}

	if (token_value != token) {
		rtvalue["error"] = ErrorCodes::TokenInvalid;
		memolog::LogWarn("chat.login.failed", "token invalid", { {"error_code", std::to_string(ErrorCodes::TokenInvalid)} });
		return ;
	}

	rtvalue["error"] = ErrorCodes::Success;


	std::string base_key = USER_BASE_INFO + uid_str;
	auto user_info = std::make_shared<UserInfo>();
	bool b_base = GetBaseInfo(base_key, uid, user_info);
	if (!b_base) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		memolog::LogWarn("chat.login.failed", "user base info not found", { {"error_code", std::to_string(ErrorCodes::UidInvalid)} });
		return;
	}
	rtvalue["uid"] = uid;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;
	rtvalue["user_id"] = user_info->user_id;


	std::vector<std::shared_ptr<ApplyInfo>> apply_list;
	auto b_apply = GetFriendApplyInfo(uid, apply_list);
	if (b_apply) {
		for (auto& apply : apply_list) {
			Json::Value obj;
			obj["name"] = apply->_name;
			obj["uid"] = apply->_uid;
			obj["user_id"] = apply->_user_id;
			obj["icon"] = apply->_icon;
			obj["nick"] = apply->_nick;
			obj["sex"] = apply->_sex;
			obj["desc"] = apply->_desc;
			obj["status"] = apply->_status;
			for (const auto& tag : apply->_labels) {
				obj["labels"].append(tag);
			}
			rtvalue["apply_list"].append(obj);
		}
	}


	std::vector<std::shared_ptr<UserInfo>> friend_list;
	bool b_friend_list = GetFriendList(uid, friend_list);
	for (auto& friend_ele : friend_list) {
		Json::Value obj;
		obj["name"] = friend_ele->name;
		obj["uid"] = friend_ele->uid;
		obj["user_id"] = friend_ele->user_id;
		obj["icon"] = friend_ele->icon;
		obj["nick"] = friend_ele->nick;
		obj["sex"] = friend_ele->sex;
		obj["desc"] = friend_ele->desc;
		obj["back"] = friend_ele->back;
		for (const auto& tag : friend_ele->labels) {
			obj["labels"].append(tag);
		}
		rtvalue["friend_list"].append(obj);
	}

	auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
	{


		auto lock_key = LOCK_PREFIX + uid_str;
		auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);

		Defer defer2([this, identifier, lock_key]() {
			RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
			});


		std::string uid_ip_value = "";
		auto uid_ip_key = USERIPPREFIX + uid_str;
		bool b_ip = RedisMgr::GetInstance()->Get(uid_ip_key, uid_ip_value);

		if (b_ip) {

			auto& cfg = ConfigMgr::Inst();
			auto self_name = cfg["SelfServer"]["Name"];

			if (uid_ip_value == self_name) {

				auto old_session = UserMgr::GetInstance()->GetSession(uid);


				if (old_session) {
					old_session->NotifyOffline(uid);

					_p_server->ClearSession(old_session->GetSessionId());
				}

			}
			else {


				KickUserReq kick_req;
				kick_req.set_uid(uid);
				ChatGrpcClient::GetInstance()->NotifyKickUser(uid_ip_value, kick_req);
			}
		}


		session->SetUserId(uid);

		std::string  ipkey = USERIPPREFIX + uid_str;
		RedisMgr::GetInstance()->Set(ipkey, server_name);

		UserMgr::GetInstance()->SetUserSession(uid, session);
		std::string  uid_session_key = USER_SESSION_PREFIX + uid_str;
		RedisMgr::GetInstance()->Set(uid_session_key, session->GetSessionId());

	}
	memolog::LogInfo("chat.login.succeeded", "chat login success",
		{
			{"uid", std::to_string(uid)},
			{"session_id", session->GetSessionId()}
		});

	return;
}

void LogicSystem::SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const std::string user_id = root["user_id"].asString();
	std::cout << "user SearchInfo user_id is " << user_id << endl;

	Json::Value  rtvalue;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_SEARCH_USER_RSP);
		});

	if (!root.isMember("user_id") || user_id.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	int uid = 0;
	if (!MysqlMgr::GetInstance()->GetUidByUserId(user_id, uid) || uid <= 0) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	GetUserByUid(std::to_string(uid), rtvalue);
	return;
}

void LogicSystem::AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto applyname = root["applyname"].asString();
	auto bakname = root["bakname"].asString();
	auto touid = root["touid"].asInt();
	std::vector<std::string> labels;
	if (root.isMember("labels") && root["labels"].isArray()) {
		for (const auto& one : root["labels"]) {
			labels.push_back(one.asString());
		}
	}
	std::cout << "user login uid is  " << uid << " applyname  is "
		<< applyname << " bakname is " << bakname << " touid is " << touid << endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_ADD_FRIEND_RSP);
		});


	MysqlMgr::GetInstance()->AddFriendApply(uid, touid);
	MysqlMgr::GetInstance()->ReplaceApplyTags(uid, touid, labels);


	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}


	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];


	std::string base_key = USER_BASE_INFO + std::to_string(uid);
	auto apply_info = std::make_shared<UserInfo>();
	bool b_info = GetBaseInfo(base_key, uid, apply_info);


	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {

			Json::Value  notify;
			notify["error"] = ErrorCodes::Success;
			notify["applyuid"] = uid;
			notify["name"] = applyname;
			notify["desc"] = "";
			if (b_info) {
				notify["icon"] = apply_info->icon;
				notify["sex"] = apply_info->sex;
				notify["nick"] = apply_info->nick;
				notify["user_id"] = apply_info->user_id;
			}
			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
		}

		return ;
	}

	
	AddFriendReq add_req;
	add_req.set_applyuid(uid);
	add_req.set_touid(touid);
	add_req.set_name(applyname);
	add_req.set_desc("");
	if (b_info) {
		add_req.set_icon(apply_info->icon);
		add_req.set_sex(apply_info->sex);
		add_req.set_nick(apply_info->nick);
	}


	ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value,add_req);

}

void LogicSystem::AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();
	auto back_name = root["back"].asString();
	std::vector<std::string> labels;
	if (root.isMember("labels") && root["labels"].isArray()) {
		for (const auto& one : root["labels"]) {
			labels.push_back(one.asString());
		}
	}
	std::cout << "from " << uid << " auth friend to " << touid << std::endl;

	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	auto user_info = std::make_shared<UserInfo>();

	std::string base_key = USER_BASE_INFO + std::to_string(touid);
	bool b_info = GetBaseInfo(base_key, touid, user_info);
	if (b_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
		rtvalue["uid"] = touid;
		rtvalue["user_id"] = user_info->user_id;
	}
	else {
		rtvalue["error"] = ErrorCodes::UidInvalid;
	}


	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_AUTH_FRIEND_RSP);
		});


	MysqlMgr::GetInstance()->AuthFriendApply(uid, touid);


	MysqlMgr::GetInstance()->AddFriend(uid, touid,back_name);
	MysqlMgr::GetInstance()->ReplaceFriendTags(uid, touid, labels);
	auto requesterApplyTags = MysqlMgr::GetInstance()->GetApplyTags(touid, uid);
	MysqlMgr::GetInstance()->ReplaceFriendTags(touid, uid, requesterApplyTags);


	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];

	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {

			Json::Value  notify;
			notify["error"] = ErrorCodes::Success;
			notify["fromuid"] = uid;
			notify["touid"] = touid;
			std::string base_key = USER_BASE_INFO + std::to_string(uid);
			auto user_info = std::make_shared<UserInfo>();
			bool b_info = GetBaseInfo(base_key, uid, user_info);
			if (b_info) {
				notify["name"] = user_info->name;
				notify["nick"] = user_info->nick;
				notify["icon"] = user_info->icon;
				notify["sex"] = user_info->sex;
				notify["user_id"] = user_info->user_id;
			}
			else {
				notify["error"] = ErrorCodes::UidInvalid;
			}


			std::string return_str = notify.toStyledString();
			session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
		}

		return ;
	}


	AuthFriendReq auth_req;
	auth_req.set_fromuid(uid);
	auth_req.set_touid(touid);


	ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	auto uid = root["fromuid"].asInt();
	auto touid = root["touid"].asInt();

	const Json::Value arrays = root["text_array"];
	
	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
		});

	if (uid <= 0 || touid <= 0 || !arrays.isArray() || arrays.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	Json::Value normalized = Json::arrayValue;
	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(uid);
	text_msg_req.set_touid(touid);
	const int conv_uid_min = std::min(uid, touid);
	const int conv_uid_max = std::max(uid, touid);
	for (const auto& txt_obj : arrays) {
		const std::string content = txt_obj["content"].asString();
		const std::string msgid = txt_obj["msgid"].asString();
		int64_t created_at = txt_obj.get("created_at", 0).asInt64();
		if (created_at <= 0) {
			created_at = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count());
		}
		if (content.empty() || msgid.empty()) {
			rtvalue["error"] = ErrorCodes::Error_Json;
			return;
		}

		PrivateMessageInfo msg;
		msg.msg_id = msgid;
		msg.conv_uid_min = conv_uid_min;
		msg.conv_uid_max = conv_uid_max;
		msg.from_uid = uid;
		msg.to_uid = touid;
		msg.content = content;
		msg.created_at = created_at;
		if (!MysqlMgr::GetInstance()->SavePrivateMessage(msg)) {
			rtvalue["error"] = ErrorCodes::RPCFailed;
			return;
		}

		Json::Value element;
		element["content"] = content;
		element["msgid"] = msgid;
		element["created_at"] = static_cast<Json::Int64>(created_at);
		normalized.append(element);

		auto* text_msg = text_msg_req.add_textmsgs();
		text_msg->set_msgid(msgid);
		text_msg->set_msgcontent(content);
	}
	rtvalue["text_array"] = normalized;


	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;
	std::string to_ip_value = "";
	bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
	if (!b_ip) {
		return;
	}

	auto& cfg = ConfigMgr::Inst();
	auto self_name = cfg["SelfServer"]["Name"];

	if (to_ip_value == self_name) {
		auto session = UserMgr::GetInstance()->GetSession(touid);
		if (session) {

			std::string return_str = rtvalue.toStyledString();
			session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		}

		return ;
	}



	ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);
}

void LogicSystem::HeartBeatHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["fromuid"].asInt();
	std::cout << "receive heart beat msg, uid is " << uid << std::endl;
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	session->Send(rtvalue.toStyledString(), ID_HEARTBEAT_RSP);
}

bool LogicSystem::isPureDigit(const std::string& str)
{
	for (char c : str) {
		if (!std::isdigit(c)) {
			return false;
		}
	}
	return true;
}

void LogicSystem::GetUserByUid(std::string uid_str, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = USER_BASE_INFO + uid_str;


	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto user_id = root["user_id"].asString();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		auto icon = root["icon"].asString();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email <<" icon is " << icon << endl;

		rtvalue["uid"] = uid;
		rtvalue["user_id"] = user_id;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		rtvalue["icon"] = icon;
		return;
	}

	auto uid = std::stoi(uid_str);


	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(uid);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}


	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["user_id"] = user_info->user_id;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;
	redis_root["icon"] = user_info->icon;

	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());


	rtvalue["uid"] = user_info->uid;
	rtvalue["user_id"] = user_info->user_id;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
	rtvalue["icon"] = user_info->icon;
}

void LogicSystem::GetUserByName(std::string name, Json::Value& rtvalue)
{
	rtvalue["error"] = ErrorCodes::Success;

	std::string base_key = NAME_INFO + name;


	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		auto uid = root["uid"].asInt();
		auto user_id = root["user_id"].asString();
		auto name = root["name"].asString();
		auto pwd = root["pwd"].asString();
		auto email = root["email"].asString();
		auto nick = root["nick"].asString();
		auto desc = root["desc"].asString();
		auto sex = root["sex"].asInt();
		std::cout << "user  uid is  " << uid << " name  is "
			<< name << " pwd is " << pwd << " email is " << email << endl;

		rtvalue["uid"] = uid;
		rtvalue["user_id"] = user_id;
		rtvalue["pwd"] = pwd;
		rtvalue["name"] = name;
		rtvalue["email"] = email;
		rtvalue["nick"] = nick;
		rtvalue["desc"] = desc;
		rtvalue["sex"] = sex;
		return;
	}



	std::shared_ptr<UserInfo> user_info = nullptr;
	user_info = MysqlMgr::GetInstance()->GetUser(name);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}


	Json::Value redis_root;
	redis_root["uid"] = user_info->uid;
	redis_root["user_id"] = user_info->user_id;
	redis_root["pwd"] = user_info->pwd;
	redis_root["name"] = user_info->name;
	redis_root["email"] = user_info->email;
	redis_root["nick"] = user_info->nick;
	redis_root["desc"] = user_info->desc;
	redis_root["sex"] = user_info->sex;

	RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	

	rtvalue["uid"] = user_info->uid;
	rtvalue["user_id"] = user_info->user_id;
	rtvalue["pwd"] = user_info->pwd;
	rtvalue["name"] = user_info->name;
	rtvalue["email"] = user_info->email;
	rtvalue["nick"] = user_info->nick;
	rtvalue["desc"] = user_info->desc;
	rtvalue["sex"] = user_info->sex;
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{

	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Reader reader;
		Json::Value root;
		reader.parse(info_str, root);
		userinfo->uid = root["uid"].asInt();
		userinfo->user_id = root["user_id"].asString();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << endl;
	}
	else {


		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;


		Json::Value redis_root;
		redis_root["uid"] = uid;
		redis_root["user_id"] = userinfo->user_id;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}

	return true;
}

bool LogicSystem::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>> &list) {

	return MysqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list) {

	return MysqlMgr::GetInstance()->GetFriendList(self_id, user_list);
}

void LogicSystem::BuildGroupListJson(int uid, Json::Value& out)
{
	std::vector<std::shared_ptr<GroupInfo>> groups;
	MysqlMgr::GetInstance()->GetUserGroupList(uid, groups);
	for (const auto& group : groups) {
		if (!group) {
			continue;
		}
		Json::Value one;
		one["groupid"] = static_cast<Json::Int64>(group->group_id);
		one["group_code"] = group->group_code;
		one["name"] = group->name;
		one["icon"] = group->icon;
		one["owner_uid"] = group->owner_uid;
		one["announcement"] = group->announcement;
		one["member_limit"] = group->member_limit;
		one["member_count"] = group->member_count;
		one["is_all_muted"] = group->is_all_muted;
		one["role"] = group->role;
		out["group_list"].append(one);
	}

	std::vector<std::shared_ptr<GroupApplyInfo>> applies;
	MysqlMgr::GetInstance()->GetPendingGroupApplyForReviewer(uid, applies, 30);
	for (const auto& apply : applies) {
		if (!apply) {
			continue;
		}
		Json::Value one;
		one["apply_id"] = static_cast<Json::Int64>(apply->apply_id);
		one["groupid"] = static_cast<Json::Int64>(apply->group_id);
		std::shared_ptr<GroupInfo> group_info;
		if (MysqlMgr::GetInstance()->GetGroupById(apply->group_id, group_info) && group_info) {
			one["group_code"] = group_info->group_code;
		}
		one["applicant_uid"] = apply->applicant_uid;
		one["inviter_uid"] = apply->inviter_uid;
		auto applicant = MysqlMgr::GetInstance()->GetUser(apply->applicant_uid);
		if (applicant) {
			one["applicant_user_id"] = applicant->user_id;
		}
		if (apply->inviter_uid > 0) {
			auto inviter = MysqlMgr::GetInstance()->GetUser(apply->inviter_uid);
			if (inviter) {
				one["inviter_user_id"] = inviter->user_id;
			}
		}
		one["type"] = apply->type;
		one["status"] = apply->status;
		one["reason"] = apply->reason;
		out["pending_group_apply_list"].append(one);
	}
}

void LogicSystem::BuildDialogListJson(int uid, Json::Value& out)
{
	std::vector<std::shared_ptr<UserInfo>> friend_list;
	GetFriendList(uid, friend_list);
	for (const auto& peer : friend_list) {
		if (!peer) {
			continue;
		}
		Json::Value one;
		one["dialog_id"] = std::string("u_") + std::to_string(peer->uid);
		one["dialog_type"] = "private";
		one["peer_uid"] = peer->uid;
		one["title"] = peer->nick.empty() ? peer->name : peer->nick;
		one["avatar"] = peer->icon;
		one["last_msg_preview"] = "";
		one["last_msg_ts"] = static_cast<Json::Int64>(0);
		one["unread_count"] = 0;
		one["pinned_rank"] = 0;
		one["draft_text"] = "";
		one["mute_state"] = 0;
		out["dialogs"].append(one);
	}

	std::vector<std::shared_ptr<GroupInfo>> groups;
	MysqlMgr::GetInstance()->GetUserGroupList(uid, groups);
	for (const auto& group : groups) {
		if (!group) {
			continue;
		}
		Json::Value one;
		one["dialog_id"] = std::string("g_") + std::to_string(group->group_id);
		one["dialog_type"] = "group";
		one["group_id"] = static_cast<Json::Int64>(group->group_id);
		one["title"] = group->name;
		one["avatar"] = group->icon;
		one["last_msg_preview"] = "";
		one["last_msg_ts"] = static_cast<Json::Int64>(0);
		one["unread_count"] = 0;
		one["pinned_rank"] = 0;
		one["draft_text"] = "";
		one["mute_state"] = 0;
		out["dialogs"].append(one);
	}
}

void LogicSystem::GetDialogListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root.get("fromuid", root.get("uid", 0)).asInt();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["uid"] = uid;
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_GET_DIALOG_LIST_RSP);
		});

	if (uid <= 0) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	BuildDialogListJson(uid, rtvalue);
}

void LogicSystem::PushGroupPayload(const std::vector<int>& recipients, short msgid, const Json::Value& payload, int exclude_uid)
{
	if (recipients.empty()) {
		return;
	}

	std::unordered_set<int> uniq;
	for (int uid : recipients) {
		if (uid <= 0 || uid == exclude_uid) {
			continue;
		}
		uniq.insert(uid);
	}
	if (uniq.empty()) {
		return;
	}

	const std::string payload_str = payload.toStyledString();
	auto& cfg = ConfigMgr::Inst();
	const auto self_name = cfg["SelfServer"]["Name"];
	std::unordered_map<std::string, std::vector<int>> remote_server_uids;

	for (int uid : uniq) {
		std::string to_ip_value = "";
		const auto to_ip_key = USERIPPREFIX + std::to_string(uid);
		const bool online = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
		if (!online) {
			continue;
		}

		if (to_ip_value == self_name) {
			auto session = UserMgr::GetInstance()->GetSession(uid);
			if (session) {
				session->Send(payload_str, msgid);
			}
		}
		else {
			remote_server_uids[to_ip_value].push_back(uid);
		}
	}

	for (auto& entry : remote_server_uids) {
		auto& server_name = entry.first;
		auto& uids = entry.second;
		if (uids.empty()) {
			continue;
		}

		if (msgid == ID_NOTIFY_GROUP_CHAT_MSG_REQ) {
			GroupMessageNotifyReq req;
			req.set_tcp_msgid(msgid);
			req.set_payload_json(payload_str);
			for (int uid : uids) {
				req.add_touids(uid);
			}
			ChatGrpcClient::GetInstance()->NotifyGroupMessage(server_name, req);
			continue;
		}

		if (msgid == ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ) {
			GroupMemberBatchReq req;
			req.set_tcp_msgid(msgid);
			req.set_payload_json(payload_str);
			for (int uid : uids) {
				req.add_touids(uid);
			}
			ChatGrpcClient::GetInstance()->NotifyGroupMemberBatch(server_name, req);
			continue;
		}

		GroupEventNotifyReq req;
		req.set_tcp_msgid(msgid);
		req.set_payload_json(payload_str);
		for (int uid : uids) {
			req.add_touids(uid);
		}
		ChatGrpcClient::GetInstance()->NotifyGroupEvent(server_name, req);
	}
}

void LogicSystem::CreateGroupHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);

	const int owner_uid = root["fromuid"].asInt();
	const std::string group_name = root["name"].asString();
	const std::string announcement = root.get("announcement", "").asString();
	const int member_limit = root.get("member_limit", 200).asInt();
	std::vector<int> members;
	std::unordered_set<int> member_set;
	bool invalid_member_user_id = false;
	if (root.isMember("member_user_ids") && root["member_user_ids"].isArray()) {
		for (const auto& one : root["member_user_ids"]) {
			const std::string member_user_id = one.asString();
			int uid = 0;
			if (!MysqlMgr::GetInstance()->GetUidByUserId(member_user_id, uid) || uid <= 0) {
				invalid_member_user_id = true;
				break;
			}
			if (uid == owner_uid) {
				continue;
			}
			member_set.insert(uid);
		}
	}
	for (int uid : member_set) {
		members.push_back(uid);
	}

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_CREATE_GROUP_RSP);
		});

	if (owner_uid <= 0 || group_name.empty() || group_name.size() > 64 || invalid_member_user_id) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	for (int uid : members) {
		if (!MysqlMgr::GetInstance()->IsFriend(owner_uid, uid)) {
			rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
			return;
		}
	}

	int64_t group_id = 0;
	std::string group_code;
	if (!MysqlMgr::GetInstance()->CreateGroup(owner_uid, group_name, announcement, member_limit, members, group_id, group_code)
		|| group_id <= 0) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}

	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["group_code"] = group_code;
	rtvalue["name"] = group_name;
	rtvalue["announcement"] = announcement;
	BuildGroupListJson(owner_uid, rtvalue);

	if (!members.empty()) {
		Json::Value notify;
		notify["error"] = ErrorCodes::Success;
		notify["event"] = "group_invite";
		notify["groupid"] = static_cast<Json::Int64>(group_id);
		notify["group_code"] = group_code;
		notify["name"] = group_name;
		notify["operator_uid"] = owner_uid;
		PushGroupPayload(members, ID_NOTIFY_GROUP_INVITE_REQ, notify);
	}
}

void LogicSystem::GetGroupListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	if (uid <= 0) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		session->Send(rtvalue.toStyledString(), ID_GET_GROUP_LIST_RSP);
		return;
	}

	BuildGroupListJson(uid, rtvalue);
	session->Send(rtvalue.toStyledString(), ID_GET_GROUP_LIST_RSP);
}

void LogicSystem::InviteGroupMemberHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int from_uid = root["fromuid"].asInt();
	const std::string target_user_id = root.get("target_user_id", "").asString();
	const int64_t group_id = root["groupid"].asInt64();
	const std::string reason = root.get("reason", "").asString();
	int to_uid = 0;
	if (!MysqlMgr::GetInstance()->GetUidByUserId(target_user_id, to_uid)) {
		to_uid = 0;
	}

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["touid"] = to_uid;
	rtvalue["target_user_id"] = target_user_id;
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_INVITE_GROUP_MEMBER_RSP);
		});

	if (from_uid <= 0 || to_uid <= 0 || group_id <= 0 || target_user_id.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	if (!MysqlMgr::GetInstance()->InviteGroupMember(group_id, from_uid, to_uid, reason)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::shared_ptr<GroupInfo> group_info;
	MysqlMgr::GetInstance()->GetGroupById(group_id, group_info);

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_invite";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["group_code"] = group_info ? group_info->group_code : "";
	notify["name"] = group_info ? group_info->name : "";
	notify["operator_uid"] = from_uid;
	notify["target_user_id"] = target_user_id;
	notify["reason"] = reason;
	PushGroupPayload({ to_uid }, ID_NOTIFY_GROUP_INVITE_REQ, notify);
}

void LogicSystem::ApplyJoinGroupHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int from_uid = root["fromuid"].asInt();
	const std::string group_code = root.get("group_code", "").asString();
	int64_t group_id = 0;
	if (!MysqlMgr::GetInstance()->GetGroupIdByCode(group_code, group_id)) {
		group_id = 0;
	}
	const std::string reason = root.get("reason", "").asString();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["group_code"] = group_code;
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_APPLY_JOIN_GROUP_RSP);
		});

	if (from_uid <= 0 || group_id <= 0 || group_code.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	if (!MysqlMgr::GetInstance()->ApplyJoinGroup(group_id, from_uid, reason)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	MysqlMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> admins;
	for (const auto& one : members) {
		if (one && one->role >= 2) {
			admins.push_back(one->uid);
		}
	}
	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_apply";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["group_code"] = group_code;
	notify["applicant_uid"] = from_uid;
	auto applicant = MysqlMgr::GetInstance()->GetUser(from_uid);
	if (applicant) {
		notify["applicant_user_id"] = applicant->user_id;
	}
	notify["reason"] = reason;
	PushGroupPayload(admins, ID_NOTIFY_GROUP_APPLY_REQ, notify);
}

void LogicSystem::ReviewGroupApplyHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int reviewer_uid = root["fromuid"].asInt();
	const int64_t apply_id = root["apply_id"].asInt64();
	const bool agree = root.get("agree", false).asBool();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["apply_id"] = static_cast<Json::Int64>(apply_id);
	rtvalue["agree"] = agree;
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_REVIEW_GROUP_APPLY_RSP);
		});

	if (reviewer_uid <= 0 || apply_id <= 0) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	std::shared_ptr<GroupApplyInfo> apply_info;
	if (!MysqlMgr::GetInstance()->ReviewGroupApply(apply_id, reviewer_uid, agree, apply_info) || !apply_info) {
		rtvalue["error"] = ErrorCodes::GroupApplyNotFound;
		return;
	}

	rtvalue["groupid"] = static_cast<Json::Int64>(apply_info->group_id);
	rtvalue["applicant_uid"] = apply_info->applicant_uid;
	std::shared_ptr<GroupInfo> apply_group;
	if (MysqlMgr::GetInstance()->GetGroupById(apply_info->group_id, apply_group) && apply_group) {
		rtvalue["group_code"] = apply_group->group_code;
	}
	auto applicant = MysqlMgr::GetInstance()->GetUser(apply_info->applicant_uid);
	if (applicant) {
		rtvalue["applicant_user_id"] = applicant->user_id;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	MysqlMgr::GetInstance()->GetGroupMemberList(apply_info->group_id, members);
	std::vector<int> recipients;
	for (const auto& one : members) {
		if (one) {
			recipients.push_back(one->uid);
		}
	}
	recipients.push_back(apply_info->applicant_uid);

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_member_changed";
	notify["groupid"] = static_cast<Json::Int64>(apply_info->group_id);
	notify["group_code"] = apply_group ? apply_group->group_code : "";
	notify["applicant_uid"] = apply_info->applicant_uid;
	if (applicant) {
		notify["applicant_user_id"] = applicant->user_id;
	}
	notify["agree"] = agree;
	notify["operator_uid"] = reviewer_uid;
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void LogicSystem::DealGroupChatMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int from_uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();
	const Json::Value msg = root["msg"];

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = from_uid;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_GROUP_CHAT_MSG_RSP);
		});

	if (from_uid <= 0 || group_id <= 0 || !msg.isObject()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	int role = 0;
	if (!MysqlMgr::GetInstance()->GetUserRoleInGroup(group_id, from_uid, role)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	MysqlMgr::GetInstance()->GetGroupMemberList(group_id, members);
	const auto now_sec = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
	const auto now_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
	for (const auto& member : members) {
		if (member && member->uid == from_uid && member->mute_until > now_sec) {
			rtvalue["error"] = ErrorCodes::GroupMuted;
			return;
		}
	}

	const bool mention_all = msg.get("mention_all", false).asBool();
	if (mention_all && role < 2) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	GroupMessageInfo info;
	info.msg_id = msg.get("msgid", "").asString();
	info.group_id = group_id;
	info.from_uid = from_uid;
	info.msg_type = msg.get("msgtype", "text").asString();
	info.content = msg.get("content", "").asString();
	info.mentions_json = msg.get("mentions", Json::arrayValue).toStyledString();
	info.file_name = msg.get("file_name", "").asString();
	info.mime = msg.get("mime", "").asString();
	info.size = msg.get("size", 0).asInt();
	info.created_at = now_ms;
	if (info.msg_id.empty() || info.content.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	if (!MysqlMgr::GetInstance()->SaveGroupMessage(info)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}

	rtvalue["msg"] = msg;
	rtvalue["created_at"] = static_cast<Json::Int64>(now_ms);

	auto sender_info = MysqlMgr::GetInstance()->GetUser(from_uid);
	std::shared_ptr<GroupInfo> group_info;
	MysqlMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (sender_info) {
		rtvalue["from_name"] = sender_info->name;
		rtvalue["from_nick"] = sender_info->nick;
		rtvalue["from_icon"] = sender_info->icon;
		rtvalue["from_user_id"] = sender_info->user_id;
	}
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}

	std::vector<int> recipients;
	for (const auto& member : members) {
		if (member && member->status == 1) {
			recipients.push_back(member->uid);
		}
	}
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_CHAT_MSG_REQ, rtvalue, from_uid);
}

void LogicSystem::GroupHistoryHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();
	const int64_t before_ts = root.get("before_ts", 0).asInt64();
	const int limit = root.get("limit", 20).asInt();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_GROUP_HISTORY_RSP);
		});

	if (uid <= 0 || group_id <= 0 || !MysqlMgr::GetInstance()->IsUserInGroup(group_id, uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMessageInfo>> msgs;
	if (!MysqlMgr::GetInstance()->GetGroupHistory(group_id, before_ts, limit, msgs)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	std::shared_ptr<GroupInfo> group_info;
	if (MysqlMgr::GetInstance()->GetGroupById(group_id, group_info) && group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}

	for (const auto& one : msgs) {
		if (!one) {
			continue;
		}
		Json::Value item;
		item["msgid"] = one->msg_id;
		item["groupid"] = static_cast<Json::Int64>(one->group_id);
		item["fromuid"] = one->from_uid;
		item["msgtype"] = one->msg_type;
		item["content"] = one->content;
		Json::Value mentions(Json::arrayValue);
		if (!one->mentions_json.empty()) {
			Json::Reader mentions_reader;
			Json::Value parsed_mentions;
			if (mentions_reader.parse(one->mentions_json, parsed_mentions)) {
				mentions = parsed_mentions;
			}
		}
		item["mentions"] = mentions;
		item["file_name"] = one->file_name;
		item["mime"] = one->mime;
		item["size"] = one->size;
		item["created_at"] = static_cast<Json::Int64>(one->created_at);
		item["from_name"] = one->from_name;
		item["from_nick"] = one->from_nick;
		item["from_icon"] = one->from_icon;
		rtvalue["messages"].append(item);
	}
}

void LogicSystem::PrivateHistoryHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int peer_uid = root["peer_uid"].asInt();
	const int64_t before_ts = root.get("before_ts", 0).asInt64();
	const int limit = root.get("limit", 20).asInt();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["peer_uid"] = peer_uid;
	rtvalue["has_more"] = false;
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_PRIVATE_HISTORY_RSP);
		});

	if (uid <= 0 || peer_uid <= 0 || limit <= 0) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	if (!MysqlMgr::GetInstance()->IsFriend(uid, peer_uid)) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	std::vector<std::shared_ptr<PrivateMessageInfo>> messages;
	bool has_more = false;
	if (!MysqlMgr::GetInstance()->GetPrivateHistory(uid, peer_uid, before_ts, limit, messages, has_more)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	rtvalue["has_more"] = has_more;
	for (const auto& one : messages) {
		if (!one) {
			continue;
		}
		Json::Value item;
		item["msgid"] = one->msg_id;
		item["content"] = one->content;
		item["fromuid"] = one->from_uid;
		item["touid"] = one->to_uid;
		item["created_at"] = static_cast<Json::Int64>(one->created_at);
		rtvalue["messages"].append(item);
	}
}

void LogicSystem::UpdateGroupAnnouncementHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();
	const std::string announcement = root.get("announcement", "").asString();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["announcement"] = announcement;
	std::shared_ptr<GroupInfo> group_info;
	MysqlMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_UPDATE_GROUP_ANNOUNCEMENT_RSP);
		});

	if (!MysqlMgr::GetInstance()->UpdateGroupAnnouncement(group_id, uid, announcement)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	MysqlMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& one : members) {
		if (one) {
			recipients.push_back(one->uid);
		}
	}

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_announcement_updated";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["group_code"] = group_info ? group_info->group_code : "";
	notify["announcement"] = announcement;
	notify["operator_uid"] = uid;
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void LogicSystem::UpdateGroupIconHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();
	const std::string icon = root.get("icon", "").asString();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["icon"] = icon;
	std::shared_ptr<GroupInfo> group_info;
	MysqlMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_UPDATE_GROUP_ICON_RSP);
		});

	if (uid <= 0 || group_id <= 0 || icon.empty() || icon.size() > 512) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	if (!MysqlMgr::GetInstance()->UpdateGroupIcon(group_id, uid, icon)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	MysqlMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& one : members) {
		if (one) {
			recipients.push_back(one->uid);
		}
	}

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_icon_updated";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["group_code"] = group_info ? group_info->group_code : "";
	notify["icon"] = icon;
	notify["operator_uid"] = uid;
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void LogicSystem::SetGroupAdminHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const std::string target_user_id = root.get("target_user_id", "").asString();
	const int64_t group_id = root["groupid"].asInt64();
	const bool is_admin = root.get("is_admin", true).asBool();
	int target_uid = 0;
	if (!MysqlMgr::GetInstance()->GetUidByUserId(target_user_id, target_uid)) {
		target_uid = 0;
	}

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["touid"] = target_uid;
	rtvalue["target_user_id"] = target_user_id;
	rtvalue["is_admin"] = is_admin;
	std::shared_ptr<GroupInfo> group_info;
	MysqlMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_SET_GROUP_ADMIN_RSP);
		});

	if (target_uid <= 0 || target_user_id.empty() ||
		!MysqlMgr::GetInstance()->SetGroupAdmin(group_id, uid, target_uid, is_admin)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	MysqlMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& one : members) {
		if (one) {
			recipients.push_back(one->uid);
		}
	}
	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_admin_changed";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["group_code"] = group_info ? group_info->group_code : "";
	notify["operator_uid"] = uid;
	notify["target_uid"] = target_uid;
	notify["target_user_id"] = target_user_id;
	notify["is_admin"] = is_admin;
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void LogicSystem::MuteGroupMemberHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const std::string target_user_id = root.get("target_user_id", "").asString();
	const int64_t group_id = root["groupid"].asInt64();
	const int mute_seconds = root.get("mute_seconds", 0).asInt();
	int target_uid = 0;
	if (!MysqlMgr::GetInstance()->GetUidByUserId(target_user_id, target_uid)) {
		target_uid = 0;
	}
	const auto now = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
	const int64_t mute_until = (mute_seconds > 0) ? now + mute_seconds : 0;

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["touid"] = target_uid;
	rtvalue["target_user_id"] = target_user_id;
	rtvalue["mute_until"] = static_cast<Json::Int64>(mute_until);
	std::shared_ptr<GroupInfo> group_info;
	MysqlMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_MUTE_GROUP_MEMBER_RSP);
		});

	if (target_uid <= 0 || target_user_id.empty() ||
		!MysqlMgr::GetInstance()->MuteGroupMember(group_id, uid, target_uid, mute_until)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	MysqlMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& one : members) {
		if (one) {
			recipients.push_back(one->uid);
		}
	}
	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_mute_changed";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["group_code"] = group_info ? group_info->group_code : "";
	notify["operator_uid"] = uid;
	notify["target_uid"] = target_uid;
	notify["target_user_id"] = target_user_id;
	notify["mute_until"] = static_cast<Json::Int64>(mute_until);
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void LogicSystem::KickGroupMemberHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const std::string target_user_id = root.get("target_user_id", "").asString();
	const int64_t group_id = root["groupid"].asInt64();
	int target_uid = 0;
	if (!MysqlMgr::GetInstance()->GetUidByUserId(target_user_id, target_uid)) {
		target_uid = 0;
	}

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["touid"] = target_uid;
	rtvalue["target_user_id"] = target_user_id;
	std::shared_ptr<GroupInfo> group_info;
	MysqlMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_KICK_GROUP_MEMBER_RSP);
		});

	if (target_uid <= 0 || target_user_id.empty() ||
		!MysqlMgr::GetInstance()->KickGroupMember(group_id, uid, target_uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	MysqlMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& one : members) {
		if (one) {
			recipients.push_back(one->uid);
		}
	}
	recipients.push_back(target_uid);

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_member_kicked";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["group_code"] = group_info ? group_info->group_code : "";
	notify["operator_uid"] = uid;
	notify["target_uid"] = target_uid;
	notify["target_user_id"] = target_user_id;
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void LogicSystem::QuitGroupHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	std::shared_ptr<GroupInfo> group_info;
	MysqlMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_QUIT_GROUP_RSP);
		});

	if (!MysqlMgr::GetInstance()->QuitGroup(group_id, uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	MysqlMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& one : members) {
		if (one) {
			recipients.push_back(one->uid);
		}
	}

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_member_quit";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["group_code"] = group_info ? group_info->group_code : "";
	notify["target_uid"] = uid;
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}
