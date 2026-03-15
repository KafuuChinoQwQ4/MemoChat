#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "PostgresMgr.h"
#include "MongoMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "ChatGrpcClient.h"
#include "ChatRuntime.h"
#include "ChatHandlerRegistrars.h"
#include "AsyncEventDispatcher.h"
#include "InlineTaskBus.h"
#include "KafkaAsyncEventBus.h"
#include "RedisAsyncEventBus.h"
#include "RabbitMqTaskBus.h"
#include "ChatRelationService.h"
#include "ChatSessionService.h"
#include "GroupMessageService.h"
#include "MessageDeliveryService.h"
#include "PrivateMessageService.h"
#include "TaskDispatcher.h"
#include "DistLock.h"
#include "cluster/ChatClusterDiscovery.h"
#include "auth/ChatLoginTicket.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <json/writer.h>
#include <sstream>
#include "CServer.h"
using namespace std;

namespace {
constexpr int64_t kPermChangeGroupInfo = 1LL << 0;
constexpr int64_t kPermDeleteMessages = 1LL << 1;
constexpr int64_t kPermInviteUsers = 1LL << 2;
constexpr int64_t kPermManageAdmins = 1LL << 3;
constexpr int64_t kPermPinMessages = 1LL << 4;
constexpr int64_t kPermBanUsers = 1LL << 5;
constexpr int64_t kPermManageTopics = 1LL << 6;
constexpr int64_t kDefaultAdminPermBits =
	kPermChangeGroupInfo | kPermDeleteMessages | kPermInviteUsers | kPermPinMessages | kPermBanUsers;

enum class OnlineRouteKind {
	Offline,
	Local,
	Remote,
	Stale
};

struct OnlineRouteDecision {
	OnlineRouteKind kind = OnlineRouteKind::Offline;
	std::shared_ptr<CSession> session;
	std::string redis_server;
	bool local_session_found = false;
};

std::string ServerOnlineUsersKey(const std::string& server_name) {
	return std::string(SERVER_ONLINE_USERS_PREFIX) + server_name;
}

std::string RelationBootstrapCacheKey(int uid) {
	return std::string("relation_bootstrap_") + std::to_string(uid);
}

int RelationBootstrapCacheTtlSec() {
	auto& cfg = ConfigMgr::Inst();
	const auto raw = cfg.GetValue("RelationBootstrapCache", "TtlSec");
	if (raw.empty()) {
		return 15;
	}
	try {
		return std::max(1, std::stoi(raw));
	}
	catch (...) {
		return 15;
	}
}

void InvalidateRelationBootstrapCache(int uid) {
	if (uid <= 0) {
		return;
	}
	RedisMgr::GetInstance()->Del(RelationBootstrapCacheKey(uid));
}

bool TryAppendCachedRelationBootstrapJson(int uid, Json::Value& out) {
	if (uid <= 0) {
		return false;
	}

	std::string payload;
	if (!RedisMgr::GetInstance()->Get(RelationBootstrapCacheKey(uid), payload) || payload.empty()) {
		return false;
	}

	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value cached;
	std::string errors;
	if (!reader->parse(payload.data(), payload.data() + payload.size(), &cached, &errors) || !cached.isObject()) {
		return false;
	}

	if (cached.isMember("apply_list")) {
		out["apply_list"] = cached["apply_list"];
	}
	if (cached.isMember("friend_list")) {
		out["friend_list"] = cached["friend_list"];
	}
	return true;
}

void CacheRelationBootstrapJson(int uid, const Json::Value& payload) {
	if (uid <= 0 || !payload.isObject()) {
		return;
	}

	Json::StreamWriterBuilder builder;
	builder["indentation"] = "";
	const auto json = Json::writeString(builder, payload);
	if (json.empty()) {
		return;
	}

	RedisMgr::GetInstance()->SetEx(RelationBootstrapCacheKey(uid), json, RelationBootstrapCacheTtlSec());
}

std::string TrimCopy(const std::string& text) {
	const auto begin = text.find_first_not_of(" \t\r\n");
	if (begin == std::string::npos) {
		return std::string();
	}
	const auto end = text.find_last_not_of(" \t\r\n");
	return text.substr(begin, end - begin + 1);
}

std::vector<std::string> KnownChatServerNames() {
	std::vector<std::string> servers;
	auto& cfg = ConfigMgr::Inst();
	static const auto cluster = memochat::cluster::LoadStaticChatClusterConfig(
		[&cfg](const std::string& section, const std::string& key) {
			return cfg.GetValue(section, key);
		},
		TrimCopy(cfg["SelfServer"]["Name"]));
	for (const auto& node : cluster.enabledNodes()) {
		servers.push_back(node.name);
	}
	return servers;
}

void RepairOnlineRouteState(int uid, const std::shared_ptr<CSession>& session, const std::string& server_name) {
	if (uid <= 0 || !session || server_name.empty()) {
		return;
	}
	const auto uid_str = std::to_string(uid);
	RedisMgr::GetInstance()->Set(USERIPPREFIX + uid_str, server_name);
	RedisMgr::GetInstance()->Set(USER_SESSION_PREFIX + uid_str, session->GetSessionId());
	RedisMgr::GetInstance()->SAdd(ServerOnlineUsersKey(server_name), uid_str);
}

std::string ResolveServerFromOnlineSets(const std::string& uid_str) {
	if (uid_str.empty()) {
		return std::string();
	}

	for (const auto& server_name : KnownChatServerNames()) {
		std::vector<std::string> online_uids;
		RedisMgr::GetInstance()->SMembers(ServerOnlineUsersKey(server_name), online_uids);
		if (std::find(online_uids.begin(), online_uids.end(), uid_str) != online_uids.end()) {
			return server_name;
		}
	}
	return std::string();
}

void ClearTrackedOnlineRoute(int uid, const std::string& server_name) {
	if (uid <= 0 || server_name.empty()) {
		return;
	}
	const auto uid_str = std::to_string(uid);
	RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
	RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
	RedisMgr::GetInstance()->SRem(ServerOnlineUsersKey(server_name), uid_str);
}

OnlineRouteDecision ResolveOnlineRoute(int uid) {
	OnlineRouteDecision route;
	if (uid <= 0) {
		return route;
	}

	const auto self_name = ConfigMgr::Inst()["SelfServer"]["Name"];
	auto session = UserMgr::GetInstance()->GetSession(uid);
	if (session) {
		route.kind = OnlineRouteKind::Local;
		route.session = session;
		route.redis_server = self_name;
		route.local_session_found = true;
		RepairOnlineRouteState(uid, session, self_name);
		return route;
	}

	const auto uid_str = std::to_string(uid);
	std::string redis_server;
	if (!RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, redis_server) || redis_server.empty()) {
		redis_server = ResolveServerFromOnlineSets(uid_str);
		if (redis_server.empty()) {
			return route;
		}
	}
	route.redis_server = redis_server;
	if (redis_server != self_name) {
		route.kind = OnlineRouteKind::Remote;
		return route;
	}

	std::string reloaded_server;
	if (RedisMgr::GetInstance()->Get(USERIPPREFIX + uid_str, reloaded_server) && !reloaded_server.empty()) {
		route.redis_server = reloaded_server;
		if (reloaded_server != self_name) {
			route.kind = OnlineRouteKind::Remote;
			return route;
		}
	}

	route.kind = OnlineRouteKind::Stale;
	ClearTrackedOnlineRoute(uid, self_name);
	return route;
}

const char* RouteResultName(OnlineRouteKind kind) {
	switch (kind) {
	case OnlineRouteKind::Local:
		return "local";
	case OnlineRouteKind::Remote:
		return "remote";
	case OnlineRouteKind::Stale:
		return "stale";
	case OnlineRouteKind::Offline:
	default:
		return "offline";
	}
}

int64_t NowMs() {
	return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string GetChatAuthSecret() {
	static const std::string secret = []() {
		auto& cfg = ConfigMgr::Inst();
		auto value = cfg.GetValue("ChatAuth", "HmacSecret");
		if (value.empty()) {
			value = "memochat-dev-chat-secret";
		}
		return value;
	}();
	return secret;
}

std::string JsonToCompactString(const Json::Value& value) {
	Json::StreamWriterBuilder builder;
	builder["indentation"] = "";
	return Json::writeString(builder, value);
}

bool ParseJsonObject(const std::string& payload, Json::Value& root) {
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	std::string errors;
	return reader->parse(payload.data(), payload.data() + payload.size(), &root, &errors) && root.isObject();
}

int64_t AllocateGroupSeq(int64_t group_id) {
	if (group_id <= 0) {
		return 0;
	}
	int64_t seq = 0;
	const auto key = std::string("group:") + std::to_string(group_id) + ":seq";
	if (!RedisMgr::GetInstance()->Incr(key, seq)) {
		return 0;
	}
	return seq;
}

void BindTcpTraceContext(const std::shared_ptr<CSession>& session, const std::string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(msg_data, root) || !root.isObject()) {
		memolog::TraceContext::SetTraceId(memolog::TraceContext::NewId());
		memolog::TraceContext::SetRequestId(memolog::TraceContext::NewId());
		memolog::TraceContext::SetSpanId("");
		if (session) {
			memolog::TraceContext::SetSessionId(session->GetSessionId());
		}
		return;
	}

	std::string trace_id = root.get("trace_id", "").asString();
	if (trace_id.empty()) {
		trace_id = memolog::TraceContext::NewId();
	}
	std::string request_id = root.get("request_id", "").asString();
	if (request_id.empty()) {
		request_id = memolog::TraceContext::NewId();
	}
	memolog::TraceContext::SetTraceId(trace_id);
	memolog::TraceContext::SetRequestId(request_id);
	memolog::TraceContext::SetSpanId(root.get("span_id", "").asString());
	if (root.isMember("uid")) {
		memolog::TraceContext::SetUid(std::to_string(root["uid"].asInt()));
	} else if (root.isMember("fromuid")) {
		memolog::TraceContext::SetUid(std::to_string(root["fromuid"].asInt()));
	}
	if (session) {
		memolog::TraceContext::SetSessionId(session->GetSessionId());
	}
}

void LogPrivateRoute(const std::string& event,
	int from_uid,
	int to_uid,
	const std::string& msg_id,
	const OnlineRouteDecision& route,
	const std::string& grpc_status,
	bool notify_delivered) {
	const auto fields = std::map<std::string, std::string>{
		{"from_uid", std::to_string(from_uid)},
		{"to_uid", std::to_string(to_uid)},
		{"msg_id", msg_id},
		{"redis_server", route.redis_server},
		{"route_result", RouteResultName(route.kind)},
		{"local_session_found", route.local_session_found ? "true" : "false"},
		{"grpc_status", grpc_status},
		{"notify_delivered", notify_delivered ? "true" : "false"}
	};
	if (route.kind == OnlineRouteKind::Stale || !notify_delivered) {
		memolog::LogWarn(event, "private message notify not delivered", fields);
		return;
	}
	memolog::LogInfo(event, "private message notify delivered", fields);
}
}

LogicSystem::LogicSystem():_b_stop(false), _event_stop(false), _p_server(nullptr){
	_chat_session_service = std::make_unique<ChatSessionService>(*this);
	_chat_relation_service = std::make_unique<ChatRelationService>(*this);
	_private_message_service = std::make_unique<PrivateMessageService>(*this);
	_group_message_service = std::make_unique<GroupMessageService>(*this);
	const auto task_bus_backend = memochat::chatruntime::TaskBusBackend();
	if (task_bus_backend == "rabbitmq" && RabbitMqTaskBus::BuildAvailable()) {
		_task_bus = std::make_shared<RabbitMqTaskBus>();
		memolog::LogInfo("chat.task_bus.rabbitmq", "chat task bus backend selected",
			{{"configured_backend", task_bus_backend}});
	}
	else {
		if (task_bus_backend == "rabbitmq" && !RabbitMqTaskBus::BuildAvailable()) {
			memolog::LogWarn("chat.task_bus.rabbitmq_unavailable",
				"chat task bus rabbitmq backend is not available in current build, fallback to inline",
				{{"configured_backend", task_bus_backend}, {"fallback_backend", "inline"}});
		}
		else if (task_bus_backend != "inline") {
			memolog::LogWarn("chat.task_bus.unsupported_backend",
				"task bus backend is not implemented yet, fallback to inline",
				{{"configured_backend", task_bus_backend}, {"fallback_backend", "inline"}});
		}
		_task_bus = std::make_shared<InlineTaskBus>();
	}
	_message_delivery_service = std::make_unique<MessageDeliveryService>(
		[this](const std::string& task_type, const std::string& routing_key, const Json::Value& payload, int delay_ms, int max_retries, std::string* error) {
			return PublishTask(task_type, routing_key, payload, delay_ms, max_retries, error);
		});
	const auto async_event_bus_backend = memochat::chatruntime::AsyncEventBusBackend();
	if (async_event_bus_backend == "kafka" && KafkaAsyncEventBus::BuildAvailable()) {
		_async_event_bus = std::make_shared<KafkaAsyncEventBus>(
			[this](int64_t outbox_id, int delay_ms, int max_retries, std::string* error) {
				Json::Value payload(Json::objectValue);
				payload["outbox_id"] = static_cast<Json::Int64>(outbox_id);
				return PublishTask("outbox_repair",
					memochat::chatruntime::TaskRoutingOutboxRepair(),
					payload,
					delay_ms,
					max_retries,
					error);
			});
		memolog::LogInfo("chat.async_event_bus.kafka", "chat async event bus backend selected",
			{{"configured_backend", async_event_bus_backend}});
	}
	else {
		if (async_event_bus_backend == "kafka" && !KafkaAsyncEventBus::BuildAvailable()) {
			memolog::LogWarn("chat.async_event_bus.kafka_unavailable",
				"chat async event bus kafka backend is not available in current build, fallback to redis",
				{{"configured_backend", async_event_bus_backend}, {"fallback_backend", "redis"}});
		}
		else if (async_event_bus_backend != "redis") {
			memolog::LogWarn("chat.async_event_bus.unsupported_backend",
				"async event bus backend is not implemented yet, fallback to redis",
				{{"configured_backend", async_event_bus_backend}, {"fallback_backend", "redis"}});
		}
		_async_event_bus = std::make_shared<RedisAsyncEventBus>();
	}
	_async_event_dispatcher = std::make_unique<AsyncEventDispatcher>(
		_async_event_bus,
		[this]() { return _event_stop; },
		[this](const std::vector<int>& recipients, short msgid, const Json::Value& payload, int exclude_uid) {
			MessageDelivery().PushPayload(recipients, msgid, payload, exclude_uid);
		});
	_task_dispatcher = std::make_unique<TaskDispatcher>(
		_task_bus,
		[this]() { return _event_stop; },
		[this](const Json::Value& payload, bool) {
			const int recipient_uid = payload.get("recipient_uid", 0).asInt();
			const short msgid = static_cast<short>(payload.get("msgid", 0).asInt());
			const int exclude_uid = payload.get("exclude_uid", 0).asInt();
			if (recipient_uid <= 0 || msgid <= 0 || !payload.isObject() || !payload.isMember("payload")) {
				return true;
			}
			return MessageDelivery().TryPushPayload({ recipient_uid }, msgid, payload["payload"], exclude_uid, false);
		},
		[this](int64_t outbox_id) {
			return ExpediteOutboxRepair(outbox_id);
		});
	RegisterCallBacks();
	_worker_thread = std::thread (&LogicSystem::DealMsg, this);
	_event_worker_thread = std::thread(&LogicSystem::DealAsyncEvents, this);
	_task_worker_thread = std::thread(&LogicSystem::DealTasks, this);
}

LogicSystem::~LogicSystem(){
	_b_stop = true;
	_event_stop = true;
	_consume.notify_one();
	if (_worker_thread.joinable()) {
		_worker_thread.join();
	}
	if (_event_worker_thread.joinable()) {
		_event_worker_thread.join();
	}
	if (_task_worker_thread.joinable()) {
		_task_worker_thread.join();
	}
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

MessageDeliveryService& LogicSystem::MessageDelivery()
{
	return *_message_delivery_service;
}

bool LogicSystem::PublishTask(const std::string& task_type, const std::string& routing_key, const Json::Value& payload, int delay_ms, int max_retries, std::string* error)
{
	if (!_task_dispatcher) {
		if (error) {
			*error = "task_dispatcher_unavailable";
		}
		return false;
	}
	return _task_dispatcher->PublishTask(task_type, routing_key, payload, delay_ms, max_retries, error);
}

bool LogicSystem::ExpediteOutboxRepair(int64_t outbox_id)
{
	return PostgresMgr::GetInstance()->ExpediteChatOutboxEventRetry(outbox_id);
}


void LogicSystem::DealMsg() {
	for (;;) {
		shared_ptr<LogicNode> msg_node;
		{
			std::unique_lock<std::mutex> unique_lk(_mutex);
			_consume.wait(unique_lk, [this]() {
				return _b_stop || !_msg_que.empty();
			});
			if (_b_stop && _msg_que.empty()) {
				break;
			}
			msg_node = _msg_que.front();
			_msg_que.pop();
		}

		if (!msg_node || !msg_node->_recvnode) {
			continue;
		}

		cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << endl;
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == _fun_callbacks.end()) {
			std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
			continue;
		}
		const std::string msg_data(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len);
		BindTcpTraceContext(msg_node->_session, msg_data);
		Defer clear_trace([]() {
			memolog::TraceContext::Clear();
		});
		call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
			msg_data);
	}
}

bool LogicSystem::PublishAsyncEvent(const std::string& topic, const Json::Value& payload, std::string* error)
{
	return _async_event_dispatcher->PublishAsyncEvent(topic, payload, error);
}

void LogicSystem::DealTasks()
{
	if (_task_dispatcher) {
		_task_dispatcher->DealTasks();
	}
}

void LogicSystem::DealAsyncEvents()
{
	_async_event_dispatcher->DealAsyncEvents();
}

void LogicSystem::NotifyMessageStatus(const Json::Value& payload)
{
	_async_event_dispatcher->NotifyMessageStatus(payload);
}

void LogicSystem::HandlePrivateAsyncEvent(const Json::Value& root)
{
	_async_event_dispatcher->HandlePrivateAsyncEvent(root);
}

void LogicSystem::HandleGroupAsyncEvent(const Json::Value& root)
{
	_async_event_dispatcher->HandleGroupAsyncEvent(root);
}

void LogicSystem::RegisterCallBacks() {
	ChatSessionServiceRegistrar().Register(*this, _fun_callbacks);
	ChatRelationServiceRegistrar().Register(*this, _fun_callbacks);
	PrivateMessageServiceRegistrar().Register(*this, _fun_callbacks);
	GroupMessageServiceRegistrar().Register(*this, _fun_callbacks);
	AsyncEventDispatcherRegistrar().Register(*this, _fun_callbacks);
}

void LogicSystem::LoginHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	if (_chat_session_service) {
		_chat_session_service->HandleLogin(session, msg_id, msg_data);
		return;
	}
	const auto login_start_ms = NowMs();
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	auto token = root["token"].asString();
	const int protocol_version = root.get("protocol_version", 1).asInt();
	const auto verify_start_ms = NowMs();
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

	memochat::auth::ChatLoginTicketClaims ticket_claims;
	const auto self_server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
	std::string uid_str = std::to_string(uid);
	std::string relation_bootstrap_mode = "inline";
	if (protocol_version >= 3) {
		relation_bootstrap_mode = "explicit_pull";
		rtvalue["protocol_version"] = 3;
		std::string ticket_error;
		const auto login_ticket = root.get("login_ticket", "").asString();
		if (!memochat::auth::DecodeAndVerifyTicket(login_ticket, GetChatAuthSecret(), ticket_claims, &ticket_error)) {
			rtvalue["error"] = ErrorCodes::ChatTicketInvalid;
			memolog::LogWarn("chat.login.failed", "chat ticket invalid",
				{{"error_code", std::to_string(ErrorCodes::ChatTicketInvalid)}, {"detail", ticket_error}});
			return;
		}
		if (ticket_claims.expire_at_ms > 0 && ticket_claims.expire_at_ms < NowMs()) {
			rtvalue["error"] = ErrorCodes::ChatTicketExpired;
			memolog::LogWarn("chat.login.failed", "chat ticket expired",
				{{"error_code", std::to_string(ErrorCodes::ChatTicketExpired)}});
			return;
		}
		if (!ticket_claims.target_server.empty() && ticket_claims.target_server != self_server_name) {
			rtvalue["error"] = ErrorCodes::ChatServerMismatch;
			memolog::LogWarn("chat.login.failed", "chat ticket target server mismatch",
				{{"error_code", std::to_string(ErrorCodes::ChatServerMismatch)},
				 {"ticket_target_server", ticket_claims.target_server},
				 {"self_server", self_server_name}});
			return;
		}
		uid = ticket_claims.uid;
		uid_str = std::to_string(uid);
		memolog::TraceContext::SetUid(uid_str);
	} else if (protocol_version != 2) {
		rtvalue["error"] = ErrorCodes::ProtocolVersionMismatch;
		return;
	}

	if (protocol_version < 3) {
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
	}

	rtvalue["error"] = ErrorCodes::Success;

	auto user_info = std::make_shared<UserInfo>();
	if (protocol_version >= 3) {
		user_info->uid = ticket_claims.uid;
		user_info->user_id = ticket_claims.user_id;
		user_info->name = ticket_claims.name;
		user_info->nick = ticket_claims.nick;
		user_info->icon = ticket_claims.icon;
		user_info->desc = ticket_claims.desc;
		user_info->email = ticket_claims.email;
		user_info->sex = ticket_claims.sex;
	} else {
		std::string base_key = USER_BASE_INFO + uid_str;
		bool b_base = GetBaseInfo(base_key, uid, user_info);
		if (!b_base) {
			rtvalue["error"] = ErrorCodes::UidInvalid;
			memolog::LogWarn("chat.login.failed", "user base info not found", { {"error_code", std::to_string(ErrorCodes::UidInvalid)} });
			return;
		}
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

	if (protocol_version < 3) {
		AppendRelationBootstrapJson(uid, rtvalue);
	}

	const auto ticket_verify_ms = NowMs() - verify_start_ms;
	const auto attach_start_ms = NowMs();
	auto server_name = self_server_name;
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
		RepairOnlineRouteState(uid, session, server_name);

	}
	const auto session_attach_ms = NowMs() - attach_start_ms;
	memolog::LogInfo("chat.login.succeeded", "chat login success",
		{
			{"uid", std::to_string(uid)},
			{"session_id", session->GetSessionId()},
			{"login_protocol_version", std::to_string(protocol_version >= 3 ? 3 : 2)},
			{"ticket_verify_ms", std::to_string(ticket_verify_ms)},
			{"session_attach_ms", std::to_string(session_attach_ms)},
			{"relation_bootstrap_ms", "0"},
			{"relation_bootstrap_mode", relation_bootstrap_mode},
			{"chat_login_total_ms", std::to_string(NowMs() - login_start_ms)}
		});
	memolog::LogInfo("login.stage.summary", "chat login stage summary",
		{
			{"uid", std::to_string(uid)},
			{"login_protocol_version", std::to_string(protocol_version >= 3 ? 3 : 2)},
			{"ticket_verify_ms", std::to_string(ticket_verify_ms)},
			{"session_attach_ms", std::to_string(session_attach_ms)},
			{"relation_bootstrap_ms", "0"},
			{"relation_bootstrap_mode", relation_bootstrap_mode},
			{"chat_login_total_ms", std::to_string(NowMs() - login_start_ms)}
		});

	return;
}

void LogicSystem::GetRelationBootstrapHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
	if (_chat_session_service) {
		_chat_session_service->HandleRelationBootstrap(session, msg_id, msg_data);
		return;
	}
	const auto bootstrap_start_ms = NowMs();
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto trace_id = root.get("trace_id", "").asString();
	if (trace_id.empty()) {
		trace_id = memolog::TraceContext::NewId();
	}
	memolog::TraceContext::SetTraceId(trace_id);
	memolog::TraceContext::SetRequestId(memolog::TraceContext::NewId());
	if (session) {
		memolog::TraceContext::SetSessionId(session->GetSessionId());
	}
	Defer clear_trace([]() {
		memolog::TraceContext::Clear();
		});

	Json::Value rtvalue;
	rtvalue["trace_id"] = trace_id;
	rtvalue["protocol_version"] = root.get("protocol_version", 3).asInt();
	const int uid = session ? session->GetUserId() : 0;
	if (uid <= 0) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		if (session) {
			session->Send(rtvalue.toStyledString(), ID_GET_RELATION_BOOTSTRAP_RSP);
		}
		return;
	}

	memolog::TraceContext::SetUid(std::to_string(uid));
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["uid"] = uid;
	AppendRelationBootstrapJson(uid, rtvalue);
	if (session) {
		session->Send(rtvalue.toStyledString(), ID_GET_RELATION_BOOTSTRAP_RSP);
	}
	memolog::LogInfo("chat.relation_bootstrap.succeeded", "relation bootstrap fetched",
		{
			{"uid", std::to_string(uid)},
			{"tcp_msg_id", std::to_string(msg_id)},
			{"relation_bootstrap_ms", std::to_string(NowMs() - bootstrap_start_ms)},
			{"relation_bootstrap_mode", "explicit_pull"}
		});
}

void LogicSystem::SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_chat_relation_service) {
		_chat_relation_service->HandleSearchUser(session, msg_id, msg_data);
		return;
	}
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
	if (!PostgresMgr::GetInstance()->GetUidByUserId(user_id, uid) || uid <= 0) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	GetUserByUid(std::to_string(uid), rtvalue);
	return;
}

void LogicSystem::AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_chat_relation_service) {
		_chat_relation_service->HandleAddFriendApply(session, msg_id, msg_data);
		return;
	}
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


	PostgresMgr::GetInstance()->AddFriendApply(uid, touid);
	PostgresMgr::GetInstance()->ReplaceApplyTags(uid, touid, labels);
	InvalidateRelationBootstrapCache(touid);


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
	if (_chat_relation_service) {
		_chat_relation_service->HandleAuthFriendApply(session, msg_id, msg_data);
		return;
	}

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


	PostgresMgr::GetInstance()->AuthFriendApply(uid, touid);


	PostgresMgr::GetInstance()->AddFriend(uid, touid,back_name);
	PostgresMgr::GetInstance()->ReplaceFriendTags(uid, touid, labels);
	auto requesterApplyTags = PostgresMgr::GetInstance()->GetApplyTags(touid, uid);
	PostgresMgr::GetInstance()->ReplaceFriendTags(touid, uid, requesterApplyTags);
	InvalidateRelationBootstrapCache(uid);
	InvalidateRelationBootstrapCache(touid);


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
	if (_private_message_service) {
		_private_message_service->HandleTextChatMessage(session, msg_id, msg_data);
		return;
	}
	Json::Value root;
	ParseJsonObject(msg_data, root);

	const auto uid = root["fromuid"].asInt();
	const auto touid = root["touid"].asInt();
	const Json::Value arrays = root["text_array"];
	const bool kafka_shadow = memochat::chatruntime::FeatureEnabled("chat_private_kafka_shadow");
	const bool kafka_primary = memochat::chatruntime::FeatureEnabled("chat_private_kafka_primary");

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;

	Defer defer([&rtvalue, session]() {
		session->Send(JsonToCompactString(rtvalue), ID_TEXT_CHAT_MSG_RSP);
	});

	if (uid <= 0 || touid <= 0 || !arrays.isArray() || arrays.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	Json::Value normalized(Json::arrayValue);
	std::vector<PrivateMessageInfo> pending_messages;
	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(uid);
	text_msg_req.set_touid(touid);
	const int conv_uid_min = std::min(uid, touid);
	const int conv_uid_max = std::max(uid, touid);
	std::string first_msg_id;

	for (const auto& txt_obj : arrays) {
		PrivateMessageInfo msg;
		msg.msg_id = txt_obj["msgid"].asString();
		msg.content = txt_obj["content"].asString();
		msg.conv_uid_min = conv_uid_min;
		msg.conv_uid_max = conv_uid_max;
		msg.from_uid = uid;
		msg.to_uid = touid;
		msg.reply_to_server_msg_id = txt_obj.get("reply_to_server_msg_id", 0).asInt64();
		msg.edited_at_ms = txt_obj.get("edited_at_ms", 0).asInt64();
		msg.deleted_at_ms = txt_obj.get("deleted_at_ms", 0).asInt64();
		msg.created_at = txt_obj.get("created_at", 0).asInt64();
		if (txt_obj.isMember("forward_meta")) {
			msg.forward_meta_json = JsonToCompactString(txt_obj["forward_meta"]);
		}
		if (msg.created_at <= 0) {
			msg.created_at = NowMs();
		}
		if (msg.msg_id.empty() || msg.content.empty()) {
			rtvalue["error"] = ErrorCodes::Error_Json;
			return;
		}
		if (first_msg_id.empty()) {
			first_msg_id = msg.msg_id;
		}
		pending_messages.push_back(msg);

		Json::Value element;
		element["msgid"] = msg.msg_id;
		element["content"] = msg.content;
		element["created_at"] = static_cast<Json::Int64>(msg.created_at);
		if (msg.reply_to_server_msg_id > 0) {
			element["reply_to_server_msg_id"] = static_cast<Json::Int64>(msg.reply_to_server_msg_id);
		}
		if (!msg.forward_meta_json.empty()) {
			Json::Value forward_meta;
			if (ParseJsonObject(msg.forward_meta_json, forward_meta)) {
				element["forward_meta"] = forward_meta;
			}
		}
		if (msg.edited_at_ms > 0) {
			element["edited_at_ms"] = static_cast<Json::Int64>(msg.edited_at_ms);
		}
		if (msg.deleted_at_ms > 0) {
			element["deleted_at_ms"] = static_cast<Json::Int64>(msg.deleted_at_ms);
		}
		normalized.append(element);

		auto* text_msg = text_msg_req.add_textmsgs();
		text_msg->set_msgid(msg.msg_id);
		text_msg->set_msgcontent(msg.content);
	}

	const auto accept_ts = NowMs();
	rtvalue["client_msg_id"] = first_msg_id;
	rtvalue["accept_node"] = memochat::chatruntime::SelfServerName();
	rtvalue["accept_ts"] = static_cast<Json::Int64>(accept_ts);
	rtvalue["status"] = kafka_primary ? "accepted" : "persisted";
	if (!kafka_primary) {
		rtvalue["text_array"] = normalized;
	}

	Json::Value event_payload;
	event_payload["fromuid"] = uid;
	event_payload["touid"] = touid;
	event_payload["trace_id"] = root.get("trace_id", "").asString();
	event_payload["request_id"] = root.get("request_id", "").asString();
	event_payload["span_id"] = root.get("span_id", "").asString();
	event_payload["accept_node"] = memochat::chatruntime::SelfServerName();
	event_payload["accept_ts"] = static_cast<Json::Int64>(accept_ts);
	event_payload["text_array"] = normalized;

	if (kafka_primary || kafka_shadow) {
		std::string publish_error;
		if (!PublishAsyncEvent(memochat::chatruntime::TopicPrivate(), event_payload, &publish_error)) {
			if (kafka_primary) {
				rtvalue["error"] = ErrorCodes::RPCFailed;
				rtvalue["status"] = "failed";
				return;
			}
			memolog::LogWarn("chat.private.shadow_publish_failed", "private shadow publish failed",
				{ {"error", publish_error}, {"client_msg_id", first_msg_id} });
		}
	}

	if (kafka_primary) {
		return;
	}

	for (const auto& msg : pending_messages) {
		if (!PostgresMgr::GetInstance()->SavePrivateMessage(msg)) {
			rtvalue["error"] = ErrorCodes::RPCFailed;
			rtvalue["status"] = "failed";
			return;
		}
		if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SavePrivateMessage(msg)) {
			std::cerr << "[MongoMgr] SavePrivateMessage dual-write failed for msg_id=" << msg.msg_id << std::endl;
		}
	}

	const auto route = ResolveOnlineRoute(touid);
	if (route.kind == OnlineRouteKind::Offline || route.kind == OnlineRouteKind::Stale) {
		LogPrivateRoute("chat.private.route", uid, touid, first_msg_id, route, "skipped", false);
		return;
	}
	if (route.kind == OnlineRouteKind::Local && route.session) {
		route.session->Send(JsonToCompactString(rtvalue), ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		LogPrivateRoute("chat.private.route", uid, touid, first_msg_id, route, "n/a", true);
		return;
	}

	const auto notify_rsp = ChatGrpcClient::GetInstance()->NotifyTextChatMsg(route.redis_server, text_msg_req, rtvalue);
	if (notify_rsp.error() != ErrorCodes::Success) {
		if (notify_rsp.error() == ErrorCodes::TargetOffline) {
			ClearTrackedOnlineRoute(touid, route.redis_server);
		}
		LogPrivateRoute("chat.private.route", uid, touid, first_msg_id, route, std::to_string(notify_rsp.error()), false);
		return;
	}
	LogPrivateRoute("chat.private.route", uid, touid, first_msg_id, route, std::to_string(notify_rsp.error()), true);
}

void LogicSystem::HeartBeatHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	if (_chat_session_service) {
		_chat_session_service->HandleHeartbeat(session, msg_id, msg_data);
		return;
	}
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
	user_info = PostgresMgr::GetInstance()->GetUser(uid);
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
	user_info = PostgresMgr::GetInstance()->GetUser(name);
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
		user_info = PostgresMgr::GetInstance()->GetUser(uid);
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

	return PostgresMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list) {

	return PostgresMgr::GetInstance()->GetFriendList(self_id, user_list);
}

void LogicSystem::AppendRelationBootstrapJson(int uid, Json::Value& out)
{
	if (_chat_relation_service) {
		_chat_relation_service->AppendRelationBootstrapJson(uid, out);
	}
}

void LogicSystem::BuildGroupListJson(int uid, Json::Value& out)
{
	if (_group_message_service) {
		_group_message_service->BuildGroupListJson(uid, out);
	}
}

void LogicSystem::BuildDialogListJson(int uid, Json::Value& out)
{
	if (_chat_relation_service) {
		_chat_relation_service->BuildDialogListJson(uid, out);
	}
}

void LogicSystem::GetDialogListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_chat_relation_service) {
		_chat_relation_service->HandleGetDialogList(session, msg_id, msg_data);
		return;
	}
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

void LogicSystem::SyncDraftHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_chat_relation_service) {
		_chat_relation_service->HandleSyncDraft(session, msg_id, msg_data);
		return;
	}
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root.get("fromuid", root.get("uid", 0)).asInt();
	const std::string dialog_type = root.get("dialog_type", "").asString();
	const int peer_uid = root.get("peer_uid", 0).asInt();
	const int64_t group_id = root.get("groupid", root.get("group_id", 0)).asInt64();
	const bool has_mute_state = root.isMember("mute_state");
	const int mute_state = root.get("mute_state", 0).asInt();
	std::string draft_text = root.get("draft_text", "").asString();
	if (draft_text.size() > 2000) {
		draft_text.resize(2000);
	}

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["uid"] = uid;
	rtvalue["dialog_type"] = dialog_type;
	rtvalue["peer_uid"] = peer_uid;
	rtvalue["group_id"] = static_cast<Json::Int64>(group_id);
	rtvalue["draft_text"] = draft_text;
	if (has_mute_state) {
		rtvalue["mute_state"] = mute_state > 0 ? 1 : 0;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_SYNC_DRAFT_RSP);
		});

	if (uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	int normalized_peer_uid = 0;
	int64_t normalized_group_id = 0;
	if (dialog_type == "private") {
		normalized_peer_uid = peer_uid;
		if (normalized_peer_uid <= 0 || !PostgresMgr::GetInstance()->IsFriend(uid, normalized_peer_uid)) {
			rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
			return;
		}
	}
	else {
		normalized_group_id = group_id;
		if (normalized_group_id <= 0 || !PostgresMgr::GetInstance()->IsUserInGroup(normalized_group_id, uid)) {
			rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
			return;
		}
	}

	if (!PostgresMgr::GetInstance()->UpsertDialogDraft(uid, dialog_type, normalized_peer_uid, normalized_group_id, draft_text)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	if (has_mute_state
		&& !PostgresMgr::GetInstance()->UpsertDialogMuteState(uid, dialog_type, normalized_peer_uid, normalized_group_id, mute_state)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
}

void LogicSystem::PinDialogHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_chat_relation_service) {
		_chat_relation_service->HandlePinDialog(session, msg_id, msg_data);
		return;
	}
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root.get("fromuid", root.get("uid", 0)).asInt();
	const std::string dialog_type = root.get("dialog_type", "").asString();
	const int peer_uid = root.get("peer_uid", 0).asInt();
	const int64_t group_id = root.get("groupid", root.get("group_id", 0)).asInt64();
	int pinned_rank = root.get("pinned_rank", 0).asInt();
	if (pinned_rank < 0) {
		pinned_rank = 0;
	}
	if (pinned_rank > 1000000000) {
		pinned_rank = 1000000000;
	}

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["uid"] = uid;
	rtvalue["dialog_type"] = dialog_type;
	rtvalue["peer_uid"] = peer_uid;
	rtvalue["group_id"] = static_cast<Json::Int64>(group_id);
	rtvalue["pinned_rank"] = pinned_rank;
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_PIN_DIALOG_RSP);
		});

	if (uid <= 0 || (dialog_type != "private" && dialog_type != "group")) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	int normalized_peer_uid = 0;
	int64_t normalized_group_id = 0;
	if (dialog_type == "private") {
		normalized_peer_uid = peer_uid;
		if (normalized_peer_uid <= 0 || !PostgresMgr::GetInstance()->IsFriend(uid, normalized_peer_uid)) {
			rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
			return;
		}
	}
	else {
		normalized_group_id = group_id;
		if (normalized_group_id <= 0 || !PostgresMgr::GetInstance()->IsUserInGroup(normalized_group_id, uid)) {
			rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
			return;
		}
	}

	if (!PostgresMgr::GetInstance()->UpsertDialogPinned(uid, dialog_type, normalized_peer_uid, normalized_group_id, pinned_rank)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
}

void LogicSystem::ForwardGroupMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_group_message_service) {
		_group_message_service->HandleForwardGroupMessage(session, msg_id, msg_data);
		return;
	}
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int from_uid = root.get("fromuid", root.get("uid", 0)).asInt();
	const int64_t group_id = root.get("groupid", 0).asInt64();
	const std::string source_msg_id = root.get("msgid", "").asString();
	std::string client_msg_id = root.get("client_msg_id", "").asString();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = from_uid;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	if (!client_msg_id.empty()) {
		rtvalue["client_msg_id"] = client_msg_id;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_FORWARD_GROUP_MSG_RSP);
		});

	if (from_uid <= 0 || group_id <= 0 || source_msg_id.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}
	if (!PostgresMgr::GetInstance()->IsUserInGroup(group_id, from_uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::shared_ptr<GroupMessageInfo> source_msg;
	if (!(
		(MongoMgr::GetInstance()->Enabled() &&
			MongoMgr::GetInstance()->GetGroupMessageByMsgId(group_id, source_msg_id, source_msg) && source_msg) ||
		(PostgresMgr::GetInstance()->GetGroupMessageByMsgId(group_id, source_msg_id, source_msg) && source_msg))) {
		rtvalue["error"] = ErrorCodes::GroupNotFound;
		return;
	}

	const int64_t now_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
	if (client_msg_id.empty()) {
		client_msg_id = std::to_string(from_uid) + "_" + std::to_string(now_ms);
		rtvalue["client_msg_id"] = client_msg_id;
	}

	GroupMessageInfo info;
	info.msg_id = client_msg_id;
	info.group_id = group_id;
	info.from_uid = from_uid;
	info.msg_type = source_msg->msg_type;
	info.content = source_msg->content;
	info.mentions_json = source_msg->mentions_json;
	info.file_name = source_msg->file_name;
	info.mime = source_msg->mime;
	info.size = source_msg->size;
	info.reply_to_server_msg_id = source_msg->reply_to_server_msg_id;
	Json::Value forward_meta;
	forward_meta["forwarded_from_msgid"] = source_msg_id;
	forward_meta["source_server_msg_id"] = static_cast<Json::Int64>(source_msg->server_msg_id);
	forward_meta["source_group_seq"] = static_cast<Json::Int64>(source_msg->group_seq);
	forward_meta["source_from_uid"] = source_msg->from_uid;
	if (!source_msg->forward_meta_json.empty()) {
		Json::Reader prev_forward_reader;
		Json::Value prev_forward_meta;
		if (prev_forward_reader.parse(source_msg->forward_meta_json, prev_forward_meta)) {
			forward_meta["prev_forward_meta"] = prev_forward_meta;
		}
	}
	info.forward_meta_json = forward_meta.toStyledString();
	info.created_at = now_ms;
	int64_t server_msg_id = 0;
	int64_t group_seq = 0;
	if (!PostgresMgr::GetInstance()->SaveGroupMessage(info, &server_msg_id, &group_seq)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	info.server_msg_id = server_msg_id;
	info.group_seq = group_seq;
	auto sender_info_for_group_mongo = PostgresMgr::GetInstance()->GetUser(from_uid);
	if (sender_info_for_group_mongo) {
		info.from_name = sender_info_for_group_mongo->name;
		info.from_nick = sender_info_for_group_mongo->nick;
		info.from_icon = sender_info_for_group_mongo->icon;
	}
	if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SaveGroupMessage(info)) {
		std::cerr << "[MongoMgr] SaveGroupMessage dual-write failed for msg_id=" << info.msg_id << std::endl;
	}

	Json::Value forwarded_msg;
	forwarded_msg["msgid"] = info.msg_id;
	forwarded_msg["msgtype"] = info.msg_type;
	forwarded_msg["content"] = info.content;
	Json::Value mentions(Json::arrayValue);
	if (!info.mentions_json.empty()) {
		Json::Reader mentions_reader;
		Json::Value parsed_mentions;
		if (mentions_reader.parse(info.mentions_json, parsed_mentions) && parsed_mentions.isArray()) {
			mentions = parsed_mentions;
		}
	}
	forwarded_msg["mentions"] = mentions;
	if (!info.file_name.empty()) {
		forwarded_msg["file_name"] = info.file_name;
	}
	if (!info.mime.empty()) {
		forwarded_msg["mime"] = info.mime;
	}
	if (info.size > 0) {
		forwarded_msg["size"] = info.size;
	}
	forwarded_msg["forwarded_from_msgid"] = source_msg_id;
	forwarded_msg["created_at"] = static_cast<Json::Int64>(now_ms);
	forwarded_msg["server_msg_id"] = static_cast<Json::Int64>(server_msg_id);
	forwarded_msg["group_seq"] = static_cast<Json::Int64>(group_seq);
	if (info.reply_to_server_msg_id > 0) {
		forwarded_msg["reply_to_server_msg_id"] = static_cast<Json::Int64>(info.reply_to_server_msg_id);
	}
	{
		Json::Reader forward_reader;
		Json::Value parsed_forward_meta;
		if (forward_reader.parse(info.forward_meta_json, parsed_forward_meta)) {
			forwarded_msg["forward_meta"] = parsed_forward_meta;
		}
	}

	rtvalue["msg"] = forwarded_msg;
	rtvalue["created_at"] = static_cast<Json::Int64>(now_ms);
	rtvalue["server_msg_id"] = static_cast<Json::Int64>(server_msg_id);
	rtvalue["group_seq"] = static_cast<Json::Int64>(group_seq);
	if (info.reply_to_server_msg_id > 0) {
		rtvalue["reply_to_server_msg_id"] = static_cast<Json::Int64>(info.reply_to_server_msg_id);
	}
	rtvalue["forward_meta"] = forward_meta;

	auto sender_info = PostgresMgr::GetInstance()->GetUser(from_uid);
	if (sender_info) {
		rtvalue["from_name"] = sender_info->name;
		rtvalue["from_nick"] = sender_info->nick;
		rtvalue["from_icon"] = sender_info->icon;
		rtvalue["from_user_id"] = sender_info->user_id;
	}
	std::shared_ptr<GroupInfo> group_info;
	if (PostgresMgr::GetInstance()->GetGroupById(group_id, group_info) && group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& member : members) {
		if (member && member->status == 1) {
			recipients.push_back(member->uid);
		}
	}
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_CHAT_MSG_REQ, rtvalue, from_uid);
}

void LogicSystem::ForwardPrivateMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_private_message_service) {
		_private_message_service->HandleForwardPrivateMessage(session, msg_id, msg_data);
		return;
	}
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int from_uid = root.get("fromuid", root.get("uid", 0)).asInt();
	const int peer_uid = root.get("peer_uid", root.get("touid", 0)).asInt();
	const std::string source_msg_id = root.get("msgid", "").asString();
	std::string client_msg_id = root.get("client_msg_id", "").asString();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = from_uid;
	rtvalue["peer_uid"] = peer_uid;
	rtvalue["touid"] = peer_uid;
	if (!client_msg_id.empty()) {
		rtvalue["client_msg_id"] = client_msg_id;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_FORWARD_PRIVATE_MSG_RSP);
		});

	if (from_uid <= 0 || peer_uid <= 0 || source_msg_id.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}
	if (!PostgresMgr::GetInstance()->IsFriend(from_uid, peer_uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::shared_ptr<PrivateMessageInfo> source_msg;
	if (!(
		(MongoMgr::GetInstance()->Enabled() &&
			MongoMgr::GetInstance()->GetPrivateMessageByMsgId(source_msg_id, source_msg) && source_msg) ||
		(PostgresMgr::GetInstance()->GetPrivateMessageByMsgId(source_msg_id, source_msg) && source_msg))) {
		rtvalue["error"] = ErrorCodes::GroupNotFound;
		return;
	}

	const int conv_uid_min = std::min(from_uid, peer_uid);
	const int conv_uid_max = std::max(from_uid, peer_uid);
	if (source_msg->conv_uid_min != conv_uid_min || source_msg->conv_uid_max != conv_uid_max) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	const int64_t now_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
	if (client_msg_id.empty()) {
		client_msg_id = std::to_string(from_uid) + "_" + std::to_string(now_ms);
		rtvalue["client_msg_id"] = client_msg_id;
	}

	PrivateMessageInfo info;
	info.msg_id = client_msg_id;
	info.conv_uid_min = conv_uid_min;
	info.conv_uid_max = conv_uid_max;
	info.from_uid = from_uid;
	info.to_uid = peer_uid;
	info.content = source_msg->content;
	info.reply_to_server_msg_id = source_msg->reply_to_server_msg_id;
	info.created_at = now_ms;

	Json::Value forward_meta;
	forward_meta["forwarded_from_msgid"] = source_msg_id;
	forward_meta["source_from_uid"] = source_msg->from_uid;
	forward_meta["source_conv_uid_min"] = source_msg->conv_uid_min;
	forward_meta["source_conv_uid_max"] = source_msg->conv_uid_max;
	forward_meta["source_created_at"] = static_cast<Json::Int64>(source_msg->created_at);
	if (!source_msg->forward_meta_json.empty()) {
		Json::Reader prev_forward_reader;
		Json::Value prev_forward_meta;
		if (prev_forward_reader.parse(source_msg->forward_meta_json, prev_forward_meta)) {
			forward_meta["prev_forward_meta"] = prev_forward_meta;
		}
	}
	info.forward_meta_json = forward_meta.toStyledString();

	if (!PostgresMgr::GetInstance()->SavePrivateMessage(info)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SavePrivateMessage(info)) {
		std::cerr << "[MongoMgr] SavePrivateMessage dual-write failed for msg_id=" << info.msg_id << std::endl;
	}

	Json::Value msg_obj;
	msg_obj["msgid"] = info.msg_id;
	msg_obj["content"] = info.content;
	msg_obj["created_at"] = static_cast<Json::Int64>(now_ms);
	if (info.reply_to_server_msg_id > 0) {
		msg_obj["reply_to_server_msg_id"] = static_cast<Json::Int64>(info.reply_to_server_msg_id);
	}
	msg_obj["forward_meta"] = forward_meta;
	rtvalue["msg"] = msg_obj;
	rtvalue["text_array"].append(msg_obj);
	rtvalue["created_at"] = static_cast<Json::Int64>(now_ms);

	TextChatMsgReq text_msg_req;
	text_msg_req.set_fromuid(from_uid);
	text_msg_req.set_touid(peer_uid);
	auto* text_msg = text_msg_req.add_textmsgs();
	text_msg->set_msgid(info.msg_id);
	text_msg->set_msgcontent(info.content);

	const auto route = ResolveOnlineRoute(peer_uid);
	if (route.kind == OnlineRouteKind::Offline || route.kind == OnlineRouteKind::Stale) {
		LogPrivateRoute("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, "skipped", false);
		return;
	}

	if (route.kind == OnlineRouteKind::Local && route.session) {
		route.session->Send(rtvalue.toStyledString(), ID_NOTIFY_TEXT_CHAT_MSG_REQ);
		LogPrivateRoute("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, "n/a", true);
		return;
	}

	const auto notify_rsp = ChatGrpcClient::GetInstance()->NotifyTextChatMsg(route.redis_server, text_msg_req, rtvalue);
	if (notify_rsp.error() != ErrorCodes::Success) {
		if (notify_rsp.error() == ErrorCodes::TargetOffline) {
			ClearTrackedOnlineRoute(peer_uid, route.redis_server);
		}
		LogPrivateRoute("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, std::to_string(notify_rsp.error()), false);
		return;
	}
	LogPrivateRoute("chat.private.forward.route", from_uid, peer_uid, info.msg_id, route, std::to_string(notify_rsp.error()), true);
}

void LogicSystem::GroupReadAckHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_group_message_service) {
		_group_message_service->HandleGroupReadAck(session, msg_id, msg_data);
		return;
	}
	(void)session;
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root.get("fromuid", root.get("uid", 0)).asInt();
	const int64_t group_id = root.get("groupid", 0).asInt64();
	int64_t read_ts = root.get("read_ts", 0).asInt64();
	if (uid <= 0 || group_id <= 0) {
		return;
	}
	if (!PostgresMgr::GetInstance()->IsUserInGroup(group_id, uid)) {
		return;
	}
	if (read_ts <= 0) {
		read_ts = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}
	PostgresMgr::GetInstance()->UpsertGroupReadState(uid, group_id, read_ts);

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& member : members) {
		if (member && member->status == 1) {
			recipients.push_back(member->uid);
		}
	}
	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_read_ack";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["fromuid"] = uid;
	notify["read_ts"] = static_cast<Json::Int64>(read_ts);
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, uid);
}

void LogicSystem::PrivateReadAckHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_private_message_service) {
		_private_message_service->HandlePrivateReadAck(session, msg_id, msg_data);
		return;
	}
	(void)session;
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root.get("fromuid", root.get("uid", 0)).asInt();
	const int peer_uid = root.get("peer_uid", 0).asInt();
	int64_t read_ts = root.get("read_ts", 0).asInt64();
	if (uid <= 0 || peer_uid <= 0) {
		return;
	}
	if (!PostgresMgr::GetInstance()->IsFriend(uid, peer_uid)) {
		return;
	}
	if (read_ts <= 0) {
		read_ts = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}
	PostgresMgr::GetInstance()->UpsertPrivateReadState(uid, peer_uid, read_ts);

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "private_read_ack";
	notify["fromuid"] = uid;
	notify["peer_uid"] = peer_uid;
	notify["read_ts"] = static_cast<Json::Int64>(read_ts);
	PushGroupPayload({ peer_uid }, ID_NOTIFY_PRIVATE_READ_ACK_REQ, notify);
}

void LogicSystem::PushGroupPayload(const std::vector<int>& recipients, short msgid, const Json::Value& payload, int exclude_uid)
{
	MessageDelivery().PushPayload(recipients, msgid, payload, exclude_uid);
}

void LogicSystem::CreateGroupHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_group_message_service) {
		_group_message_service->HandleCreateGroup(session, msg_id, msg_data);
		return;
	}
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
			if (!PostgresMgr::GetInstance()->GetUidByUserId(member_user_id, uid) || uid <= 0) {
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
		if (!PostgresMgr::GetInstance()->IsFriend(owner_uid, uid)) {
			rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
			return;
		}
	}

	int64_t group_id = 0;
	std::string group_code;
	if (!PostgresMgr::GetInstance()->CreateGroup(owner_uid, group_name, announcement, member_limit, members, group_id, group_code)
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
	if (_group_message_service) {
		_group_message_service->HandleGetGroupList(session, msg_id, msg_data);
		return;
	}
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
	if (_group_message_service) {
		_group_message_service->HandleInviteGroupMember(session, msg_id, msg_data);
		return;
	}
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int from_uid = root["fromuid"].asInt();
	const std::string target_user_id = root.get("target_user_id", "").asString();
	const int64_t group_id = root["groupid"].asInt64();
	const std::string reason = root.get("reason", "").asString();
	int to_uid = 0;
	if (!PostgresMgr::GetInstance()->GetUidByUserId(target_user_id, to_uid)) {
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

	if (!PostgresMgr::GetInstance()->InviteGroupMember(group_id, from_uid, to_uid, reason)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::shared_ptr<GroupInfo> group_info;
	PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);

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
	if (_group_message_service) {
		_group_message_service->HandleApplyJoinGroup(session, msg_id, msg_data);
		return;
	}
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int from_uid = root["fromuid"].asInt();
	const std::string group_code = root.get("group_code", "").asString();
	int64_t group_id = 0;
	if (!PostgresMgr::GetInstance()->GetGroupIdByCode(group_code, group_id)) {
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

	if (!PostgresMgr::GetInstance()->ApplyJoinGroup(group_id, from_uid, reason)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
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
	auto applicant = PostgresMgr::GetInstance()->GetUser(from_uid);
	if (applicant) {
		notify["applicant_user_id"] = applicant->user_id;
	}
	notify["reason"] = reason;
	PushGroupPayload(admins, ID_NOTIFY_GROUP_APPLY_REQ, notify);
}

void LogicSystem::ReviewGroupApplyHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_group_message_service) {
		_group_message_service->HandleReviewGroupApply(session, msg_id, msg_data);
		return;
	}
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
	if (!PostgresMgr::GetInstance()->ReviewGroupApply(apply_id, reviewer_uid, agree, apply_info) || !apply_info) {
		rtvalue["error"] = ErrorCodes::GroupApplyNotFound;
		return;
	}

	rtvalue["groupid"] = static_cast<Json::Int64>(apply_info->group_id);
	rtvalue["applicant_uid"] = apply_info->applicant_uid;
	std::shared_ptr<GroupInfo> apply_group;
	if (PostgresMgr::GetInstance()->GetGroupById(apply_info->group_id, apply_group) && apply_group) {
		rtvalue["group_code"] = apply_group->group_code;
	}
	auto applicant = PostgresMgr::GetInstance()->GetUser(apply_info->applicant_uid);
	if (applicant) {
		rtvalue["applicant_user_id"] = applicant->user_id;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(apply_info->group_id, members);
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
	if (_group_message_service) {
		_group_message_service->HandleGroupChatMessage(session, msg_id, msg_data);
		return;
	}
	Json::Value root;
	ParseJsonObject(msg_data, root);
	const int from_uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();
	const Json::Value msg = root["msg"];
	const std::string client_msg_id = msg.get("msgid", "").asString();
	const bool kafka_shadow = memochat::chatruntime::FeatureEnabled("chat_group_kafka_shadow");
	const bool kafka_primary = memochat::chatruntime::FeatureEnabled("chat_group_kafka_primary");

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = from_uid;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	if (!client_msg_id.empty()) {
		rtvalue["client_msg_id"] = client_msg_id;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_GROUP_CHAT_MSG_RSP);
		});

	if (from_uid <= 0 || group_id <= 0 || !msg.isObject()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	int role = 0;
	if (!PostgresMgr::GetInstance()->GetUserRoleInGroup(group_id, from_uid, role)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
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
	info.msg_id = client_msg_id;
	info.group_id = group_id;
	info.from_uid = from_uid;
	info.msg_type = msg.get("msgtype", "text").asString();
	info.content = msg.get("content", "").asString();
	info.mentions_json = msg.get("mentions", Json::arrayValue).toStyledString();
	info.file_name = msg.get("file_name", "").asString();
	info.mime = msg.get("mime", "").asString();
	info.size = msg.get("size", 0).asInt();
	info.reply_to_server_msg_id = msg.get("reply_to_server_msg_id", 0).asInt64();
	if (msg.isMember("forward_meta")) {
		info.forward_meta_json = msg["forward_meta"].toStyledString();
	}
	info.edited_at_ms = msg.get("edited_at_ms", 0).asInt64();
	info.deleted_at_ms = msg.get("deleted_at_ms", 0).asInt64();
	info.created_at = now_ms;
	if (info.msg_id.empty() || info.content.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	const auto accept_ts = NowMs();
	rtvalue["accept_node"] = memochat::chatruntime::SelfServerName();
	rtvalue["accept_ts"] = static_cast<Json::Int64>(accept_ts);
	rtvalue["status"] = kafka_primary ? "accepted" : "persisted";

	auto sender_info = PostgresMgr::GetInstance()->GetUser(from_uid);
	std::shared_ptr<GroupInfo> group_info;
	PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (sender_info) {
		rtvalue["from_name"] = sender_info->name;
		rtvalue["from_nick"] = sender_info->nick;
		rtvalue["from_icon"] = sender_info->icon;
		rtvalue["from_user_id"] = sender_info->user_id;
	}
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}

	Json::Value event_payload;
	event_payload["fromuid"] = from_uid;
	event_payload["groupid"] = static_cast<Json::Int64>(group_id);
	event_payload["trace_id"] = root.get("trace_id", "").asString();
	event_payload["request_id"] = root.get("request_id", "").asString();
	event_payload["span_id"] = root.get("span_id", "").asString();
	event_payload["accept_node"] = memochat::chatruntime::SelfServerName();
	event_payload["accept_ts"] = static_cast<Json::Int64>(accept_ts);
	event_payload["msg"] = msg;
	if (sender_info) {
		event_payload["from_name"] = sender_info->name;
		event_payload["from_nick"] = sender_info->nick;
		event_payload["from_icon"] = sender_info->icon;
		event_payload["from_user_id"] = sender_info->user_id;
	}
	if (group_info) {
		event_payload["group_code"] = group_info->group_code;
	}

	if (kafka_primary || kafka_shadow) {
		std::string publish_error;
		if (!PublishAsyncEvent(memochat::chatruntime::TopicGroup(), event_payload, &publish_error)) {
			if (kafka_primary) {
				rtvalue["error"] = ErrorCodes::RPCFailed;
				rtvalue["status"] = "failed";
				return;
			}
			memolog::LogWarn("chat.group.shadow_publish_failed", "group shadow publish failed",
				{ {"error", publish_error}, {"client_msg_id", client_msg_id} });
		}
	}

	if (kafka_primary) {
		return;
	}

	int64_t server_msg_id = 0;
	int64_t group_seq = 0;
	if (!PostgresMgr::GetInstance()->SaveGroupMessage(info, &server_msg_id, &group_seq)) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		rtvalue["status"] = "failed";
		return;
	}
	info.server_msg_id = server_msg_id;
	info.group_seq = group_seq;
	if (sender_info) {
		info.from_name = sender_info->name;
		info.from_nick = sender_info->nick;
		info.from_icon = sender_info->icon;
	}
	if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->SaveGroupMessage(info)) {
		std::cerr << "[MongoMgr] SaveGroupMessage dual-write failed for msg_id=" << info.msg_id << std::endl;
	}

	Json::Value msg_out = msg;
	msg_out["created_at"] = static_cast<Json::Int64>(now_ms);
	msg_out["server_msg_id"] = static_cast<Json::Int64>(server_msg_id);
	msg_out["group_seq"] = static_cast<Json::Int64>(group_seq);
	rtvalue["msg"] = msg_out;
	rtvalue["created_at"] = static_cast<Json::Int64>(now_ms);
	rtvalue["server_msg_id"] = static_cast<Json::Int64>(server_msg_id);
	rtvalue["group_seq"] = static_cast<Json::Int64>(group_seq);

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
	if (_group_message_service) {
		_group_message_service->HandleGroupHistory(session, msg_id, msg_data);
		return;
	}
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();
	const int64_t before_ts = root.get("before_ts", 0).asInt64();
	const int64_t before_seq = root.get("before_seq", 0).asInt64();
	const int limit = root.get("limit", 20).asInt();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["has_more"] = false;
	rtvalue["next_before_seq"] = static_cast<Json::Int64>(0);
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_GROUP_HISTORY_RSP);
		});

	if (uid <= 0 || group_id <= 0 || !PostgresMgr::GetInstance()->IsUserInGroup(group_id, uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMessageInfo>> msgs;
	bool has_more = false;
	if (!(
		(MongoMgr::GetInstance()->Enabled() &&
			MongoMgr::GetInstance()->GetGroupHistory(group_id, before_ts, before_seq, limit, msgs, has_more)) ||
		PostgresMgr::GetInstance()->GetGroupHistory(group_id, before_ts, before_seq, limit, msgs, has_more))) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	rtvalue["has_more"] = has_more;
	std::shared_ptr<GroupInfo> group_info;
	if (PostgresMgr::GetInstance()->GetGroupById(group_id, group_info) && group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}

	for (const auto& one : msgs) {
		if (!one) {
			continue;
		}
		const int64_t created_at = one->created_at;
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
		item["created_at"] = static_cast<Json::Int64>(created_at);
		item["server_msg_id"] = static_cast<Json::Int64>(one->server_msg_id);
		item["group_seq"] = static_cast<Json::Int64>(one->group_seq);
		if (one->reply_to_server_msg_id > 0) {
			item["reply_to_server_msg_id"] = static_cast<Json::Int64>(one->reply_to_server_msg_id);
		}
		if (!one->forward_meta_json.empty()) {
			Json::Reader forward_reader;
			Json::Value forward_meta;
			if (forward_reader.parse(one->forward_meta_json, forward_meta)) {
				item["forward_meta"] = forward_meta;
			}
		}
		if (one->edited_at_ms > 0) {
			item["edited_at_ms"] = static_cast<Json::Int64>(one->edited_at_ms);
		}
		if (one->deleted_at_ms > 0) {
			item["deleted_at_ms"] = static_cast<Json::Int64>(one->deleted_at_ms);
		}
		item["from_name"] = one->from_name;
		item["from_nick"] = one->from_nick;
		item["from_icon"] = one->from_icon;
		if ((one->from_name.empty() || one->from_nick.empty()) && one->from_uid > 0) {
			auto from_user = PostgresMgr::GetInstance()->GetUser(one->from_uid);
			if (from_user) {
				item["from_name"] = from_user->name;
				item["from_nick"] = from_user->nick;
				item["from_icon"] = from_user->icon;
			}
		}
		rtvalue["messages"].append(item);
	}
	if (!msgs.empty() && msgs.back()) {
		rtvalue["next_before_seq"] = static_cast<Json::Int64>(msgs.back()->group_seq);
	}
	int64_t read_ts = 0;
	if (!msgs.empty() && msgs.front()) {
		read_ts = msgs.front()->created_at;
	}
	if (read_ts <= 0) {
		read_ts = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}
	PostgresMgr::GetInstance()->UpsertGroupReadState(uid, group_id, read_ts);
}

void LogicSystem::EditPrivateMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_private_message_service) {
		_private_message_service->HandleEditPrivateMessage(session, msg_id, msg_data);
		return;
	}
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int peer_uid = root["peer_uid"].asInt();
	const std::string target_msg_id = root.get("msgid", "").asString();
	const std::string content = root.get("content", "").asString();
	const int64_t now_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = uid;
	rtvalue["peer_uid"] = peer_uid;
	rtvalue["msgid"] = target_msg_id;
	rtvalue["content"] = content;
	rtvalue["edited_at_ms"] = static_cast<Json::Int64>(now_ms);
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_EDIT_PRIVATE_MSG_RSP);
		});

	if (uid <= 0 || peer_uid <= 0 || target_msg_id.empty() || content.empty() || content.size() > 4096) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}
	if (!PostgresMgr::GetInstance()->IsFriend(uid, peer_uid)) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	if (!PostgresMgr::GetInstance()->UpdatePrivateMessageContent(uid, peer_uid, target_msg_id, content, now_ms)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}
	if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->UpdatePrivateMessageContent(uid, peer_uid, target_msg_id, content, now_ms)) {
		std::cerr << "[MongoMgr] UpdatePrivateMessageContent sync failed for msg_id=" << target_msg_id << std::endl;
	}

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "private_msg_edited";
	notify["fromuid"] = uid;
	notify["peer_uid"] = peer_uid;
	notify["msgid"] = target_msg_id;
	notify["content"] = content;
	notify["edited_at_ms"] = static_cast<Json::Int64>(now_ms);
	PushGroupPayload({ uid, peer_uid }, ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ, notify);
}

void LogicSystem::RevokePrivateMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_private_message_service) {
		_private_message_service->HandleRevokePrivateMessage(session, msg_id, msg_data);
		return;
	}
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int peer_uid = root["peer_uid"].asInt();
	const std::string target_msg_id = root.get("msgid", "").asString();
	const int64_t now_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = uid;
	rtvalue["peer_uid"] = peer_uid;
	rtvalue["msgid"] = target_msg_id;
	rtvalue["content"] = "[消息已撤回]";
	rtvalue["deleted_at_ms"] = static_cast<Json::Int64>(now_ms);
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_REVOKE_PRIVATE_MSG_RSP);
		});

	if (uid <= 0 || peer_uid <= 0 || target_msg_id.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}
	if (!PostgresMgr::GetInstance()->IsFriend(uid, peer_uid)) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}
	if (!PostgresMgr::GetInstance()->RevokePrivateMessage(uid, peer_uid, target_msg_id, now_ms)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}
	if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->RevokePrivateMessage(uid, peer_uid, target_msg_id, now_ms)) {
		std::cerr << "[MongoMgr] RevokePrivateMessage sync failed for msg_id=" << target_msg_id << std::endl;
	}

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "private_msg_revoked";
	notify["fromuid"] = uid;
	notify["peer_uid"] = peer_uid;
	notify["msgid"] = target_msg_id;
	notify["content"] = "[消息已撤回]";
	notify["deleted_at_ms"] = static_cast<Json::Int64>(now_ms);
	PushGroupPayload({ uid, peer_uid }, ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ, notify);
}

void LogicSystem::EditGroupMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_group_message_service) {
		_group_message_service->HandleEditGroupMessage(session, msg_id, msg_data);
		return;
	}
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();
	const std::string target_msg_id = root.get("msgid", "").asString();
	const std::string content = root.get("content", "").asString();
	const int64_t now_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["msgid"] = target_msg_id;
	rtvalue["content"] = content;
	rtvalue["edited_at_ms"] = static_cast<Json::Int64>(now_ms);
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_EDIT_GROUP_MSG_RSP);
		});

	if (uid <= 0 || group_id <= 0 || target_msg_id.empty() || content.empty() || content.size() > 4096) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	if (!PostgresMgr::GetInstance()->IsUserInGroup(group_id, uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	if (!PostgresMgr::GetInstance()->UpdateGroupMessageContent(group_id, uid, target_msg_id, content, now_ms)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}
	if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->UpdateGroupMessageContent(group_id, uid, target_msg_id, content, now_ms)) {
		std::cerr << "[MongoMgr] UpdateGroupMessageContent sync failed for msg_id=" << target_msg_id << std::endl;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& member : members) {
		if (member && member->status == 1) {
			recipients.push_back(member->uid);
		}
	}

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_msg_edited";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["msgid"] = target_msg_id;
	notify["content"] = content;
	notify["edited_at_ms"] = static_cast<Json::Int64>(now_ms);
	notify["operator_uid"] = uid;
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, 0);
}

void LogicSystem::RevokeGroupMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_group_message_service) {
		_group_message_service->HandleRevokeGroupMessage(session, msg_id, msg_data);
		return;
	}
	(void)msg_id;
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();
	const std::string target_msg_id = root.get("msgid", "").asString();
	const int64_t now_ms = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["msgid"] = target_msg_id;
	rtvalue["content"] = "[消息已撤回]";
	rtvalue["deleted_at_ms"] = static_cast<Json::Int64>(now_ms);
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_REVOKE_GROUP_MSG_RSP);
		});

	if (uid <= 0 || group_id <= 0 || target_msg_id.empty()) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	if (!PostgresMgr::GetInstance()->IsUserInGroup(group_id, uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	if (!PostgresMgr::GetInstance()->RevokeGroupMessage(group_id, uid, target_msg_id, now_ms)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}
	if (MongoMgr::GetInstance()->Enabled() && !MongoMgr::GetInstance()->RevokeGroupMessage(group_id, uid, target_msg_id, now_ms)) {
		std::cerr << "[MongoMgr] RevokeGroupMessage sync failed for msg_id=" << target_msg_id << std::endl;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
	std::vector<int> recipients;
	for (const auto& member : members) {
		if (member && member->status == 1) {
			recipients.push_back(member->uid);
		}
	}

	Json::Value notify;
	notify["error"] = ErrorCodes::Success;
	notify["event"] = "group_msg_revoked";
	notify["groupid"] = static_cast<Json::Int64>(group_id);
	notify["msgid"] = target_msg_id;
	notify["content"] = "[消息已撤回]";
	notify["deleted_at_ms"] = static_cast<Json::Int64>(now_ms);
	notify["operator_uid"] = uid;
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify, 0);
}

void LogicSystem::PrivateHistoryHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_private_message_service) {
		_private_message_service->HandlePrivateHistory(session, msg_id, msg_data);
		return;
	}
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int peer_uid = root["peer_uid"].asInt();
	const int64_t before_ts = root.get("before_ts", 0).asInt64();
	const std::string before_msg_id = root.get("before_msg_id", "").asString();
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

	if (!PostgresMgr::GetInstance()->IsFriend(uid, peer_uid)) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	std::vector<std::shared_ptr<PrivateMessageInfo>> messages;
	bool has_more = false;
	if (!(
		(MongoMgr::GetInstance()->Enabled() &&
			MongoMgr::GetInstance()->GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more)) ||
		PostgresMgr::GetInstance()->GetPrivateHistory(uid, peer_uid, before_ts, before_msg_id, limit, messages, has_more))) {
		rtvalue["error"] = ErrorCodes::RPCFailed;
		return;
	}
	rtvalue["has_more"] = has_more;
	int64_t max_peer_read_ts = 0;
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
		if (one->reply_to_server_msg_id > 0) {
			item["reply_to_server_msg_id"] = static_cast<Json::Int64>(one->reply_to_server_msg_id);
		}
		if (!one->forward_meta_json.empty()) {
			Json::Reader forward_reader;
			Json::Value forward_meta;
			if (forward_reader.parse(one->forward_meta_json, forward_meta)) {
				item["forward_meta"] = forward_meta;
			}
		}
		if (one->edited_at_ms > 0) {
			item["edited_at_ms"] = static_cast<Json::Int64>(one->edited_at_ms);
		}
		if (one->deleted_at_ms > 0) {
			item["deleted_at_ms"] = static_cast<Json::Int64>(one->deleted_at_ms);
		}
		rtvalue["messages"].append(item);
		if (one->from_uid == peer_uid && one->created_at > max_peer_read_ts) {
			max_peer_read_ts = one->created_at;
		}
	}
	if (max_peer_read_ts > 0) {
		PostgresMgr::GetInstance()->UpsertPrivateReadState(uid, peer_uid, max_peer_read_ts);
	}
}

void LogicSystem::UpdateGroupAnnouncementHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_group_message_service) {
		_group_message_service->HandleUpdateGroupAnnouncement(session, msg_id, msg_data);
		return;
	}
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
	PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_UPDATE_GROUP_ANNOUNCEMENT_RSP);
		});

	if (!PostgresMgr::GetInstance()->UpdateGroupAnnouncement(group_id, uid, announcement)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
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
	if (_group_message_service) {
		_group_message_service->HandleUpdateGroupIcon(session, msg_id, msg_data);
		return;
	}
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
	PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
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

	if (!PostgresMgr::GetInstance()->UpdateGroupIcon(group_id, uid, icon)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
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
	if (_group_message_service) {
		_group_message_service->HandleSetGroupAdmin(session, msg_id, msg_data);
		return;
	}
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const std::string target_user_id = root.get("target_user_id", "").asString();
	const int64_t group_id = root["groupid"].asInt64();
	const bool is_admin = root.get("is_admin", true).asBool();
	bool has_permission_bits = root.isMember("permission_bits");
	int64_t permission_bits = root.get("permission_bits", 0).asInt64();
	auto merge_perm_flag = [&](const char* key, int64_t bit) {
		if (!root.isMember(key)) {
			return;
		}
		has_permission_bits = true;
		if (root.get(key, false).asBool()) {
			permission_bits |= bit;
		}
		else {
			permission_bits &= ~bit;
		}
		};
	merge_perm_flag("can_change_group_info", kPermChangeGroupInfo);
	merge_perm_flag("can_delete_messages", kPermDeleteMessages);
	merge_perm_flag("can_invite_users", kPermInviteUsers);
	merge_perm_flag("can_manage_admins", kPermManageAdmins);
	merge_perm_flag("can_pin_messages", kPermPinMessages);
	merge_perm_flag("can_ban_users", kPermBanUsers);
	merge_perm_flag("can_manage_topics", kPermManageTopics);
	if (!is_admin) {
		permission_bits = 0;
		has_permission_bits = true;
	}
	int64_t requested_permission_bits = has_permission_bits ? permission_bits : 0;
	if (is_admin && !has_permission_bits && requested_permission_bits <= 0) {
		requested_permission_bits = kDefaultAdminPermBits;
	}
	int target_uid = 0;
	if (!PostgresMgr::GetInstance()->GetUidByUserId(target_user_id, target_uid)) {
		target_uid = 0;
	}

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["touid"] = target_uid;
	rtvalue["target_user_id"] = target_user_id;
	rtvalue["is_admin"] = is_admin;
	rtvalue["permission_bits"] = static_cast<Json::Int64>(requested_permission_bits);
	rtvalue["can_change_group_info"] = (requested_permission_bits & kPermChangeGroupInfo) != 0;
	rtvalue["can_delete_messages"] = (requested_permission_bits & kPermDeleteMessages) != 0;
	rtvalue["can_invite_users"] = (requested_permission_bits & kPermInviteUsers) != 0;
	rtvalue["can_manage_admins"] = (requested_permission_bits & kPermManageAdmins) != 0;
	rtvalue["can_pin_messages"] = (requested_permission_bits & kPermPinMessages) != 0;
	rtvalue["can_ban_users"] = (requested_permission_bits & kPermBanUsers) != 0;
	rtvalue["can_manage_topics"] = (requested_permission_bits & kPermManageTopics) != 0;
	std::shared_ptr<GroupInfo> group_info;
	PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_SET_GROUP_ADMIN_RSP);
		});

	if (target_uid <= 0 || target_user_id.empty() ||
		!PostgresMgr::GetInstance()->SetGroupAdmin(group_id, uid, target_uid, is_admin, requested_permission_bits)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
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
	notify["permission_bits"] = static_cast<Json::Int64>(requested_permission_bits);
	notify["can_change_group_info"] = (requested_permission_bits & kPermChangeGroupInfo) != 0;
	notify["can_delete_messages"] = (requested_permission_bits & kPermDeleteMessages) != 0;
	notify["can_invite_users"] = (requested_permission_bits & kPermInviteUsers) != 0;
	notify["can_manage_admins"] = (requested_permission_bits & kPermManageAdmins) != 0;
	notify["can_pin_messages"] = (requested_permission_bits & kPermPinMessages) != 0;
	notify["can_ban_users"] = (requested_permission_bits & kPermBanUsers) != 0;
	notify["can_manage_topics"] = (requested_permission_bits & kPermManageTopics) != 0;
	PushGroupPayload(recipients, ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ, notify);
}

void LogicSystem::MuteGroupMemberHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data)
{
	if (_group_message_service) {
		_group_message_service->HandleMuteGroupMember(session, msg_id, msg_data);
		return;
	}
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const std::string target_user_id = root.get("target_user_id", "").asString();
	const int64_t group_id = root["groupid"].asInt64();
	const int mute_seconds = root.get("mute_seconds", 0).asInt();
	int target_uid = 0;
	if (!PostgresMgr::GetInstance()->GetUidByUserId(target_user_id, target_uid)) {
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
	PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_MUTE_GROUP_MEMBER_RSP);
		});

	if (target_uid <= 0 || target_user_id.empty() ||
		!PostgresMgr::GetInstance()->MuteGroupMember(group_id, uid, target_uid, mute_until)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
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
	if (_group_message_service) {
		_group_message_service->HandleKickGroupMember(session, msg_id, msg_data);
		return;
	}
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const std::string target_user_id = root.get("target_user_id", "").asString();
	const int64_t group_id = root["groupid"].asInt64();
	int target_uid = 0;
	if (!PostgresMgr::GetInstance()->GetUidByUserId(target_user_id, target_uid)) {
		target_uid = 0;
	}

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	rtvalue["touid"] = target_uid;
	rtvalue["target_user_id"] = target_user_id;
	std::shared_ptr<GroupInfo> group_info;
	PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_KICK_GROUP_MEMBER_RSP);
		});

	if (target_uid <= 0 || target_user_id.empty() ||
		!PostgresMgr::GetInstance()->KickGroupMember(group_id, uid, target_uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
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
	if (_group_message_service) {
		_group_message_service->HandleQuitGroup(session, msg_id, msg_data);
		return;
	}
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	const int uid = root["fromuid"].asInt();
	const int64_t group_id = root["groupid"].asInt64();

	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["groupid"] = static_cast<Json::Int64>(group_id);
	std::shared_ptr<GroupInfo> group_info;
	PostgresMgr::GetInstance()->GetGroupById(group_id, group_info);
	if (group_info) {
		rtvalue["group_code"] = group_info->group_code;
	}
	Defer defer([&rtvalue, session]() {
		session->Send(rtvalue.toStyledString(), ID_QUIT_GROUP_RSP);
		});

	if (!PostgresMgr::GetInstance()->QuitGroup(group_id, uid)) {
		rtvalue["error"] = ErrorCodes::GroupPermissionDenied;
		return;
	}

	std::vector<std::shared_ptr<GroupMemberInfo>> members;
	PostgresMgr::GetInstance()->GetGroupMemberList(group_id, members);
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
