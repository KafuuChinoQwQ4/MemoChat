#pragma once
#include "Singleton.h"
#include <queue>
#include <thread>
#include "CSession.h"
#include <queue>
#include <map>
#include <functional>
#include "const.h"
#include "json/GlazeCompat.h"


#include <unordered_map>
#include "data.h"
#include <atomic>
#include <vector>
#include <memory>
#include <array>
#include <mutex>
#include <condition_variable>

class CServer;
typedef  function<void(shared_ptr<CSession>, const short &msg_id, const string &msg_data)> FunCallBack;
class ChatSessionServiceRegistrar;
class ChatRelationServiceRegistrar;
class PrivateMessageServiceRegistrar;
class GroupMessageServiceRegistrar;
class AsyncEventDispatcherRegistrar;
class ChatSessionService;
class ChatRelationService;
class PrivateMessageService;
class GroupMessageService;
class AsyncEventDispatcher;
class IAsyncEventBus;
class IAsyncTaskBus;
class MessageDeliveryService;
class TaskDispatcher;

class LogicSystem:public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
	friend class ChatSessionServiceRegistrar;
	friend class ChatRelationServiceRegistrar;
	friend class PrivateMessageServiceRegistrar;
	friend class GroupMessageServiceRegistrar;
	friend class AsyncEventDispatcherRegistrar;
	friend class ChatSessionService;
	friend class ChatRelationService;
	friend class PrivateMessageService;
	friend class GroupMessageService;
	friend class AsyncEventDispatcher;
	friend class MessageDeliveryService;
public:
	~LogicSystem();
	void PostMsgToQue(shared_ptr < LogicNode> msg);
	void SetServer(std::shared_ptr<CServer> pserver);
	MessageDeliveryService& MessageDelivery();
	bool PublishTask(const std::string& task_type, const std::string& routing_key, const memochat::json::JsonValue& payload, int delay_ms = 0, int max_retries = 0, std::string* error = nullptr);
	bool PublishAsyncEvent(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error = nullptr);
	bool ExpediteOutboxRepair(int64_t outbox_id);

	static constexpr size_t kDefaultWorkerCount = 4;
	static constexpr size_t kMaxWorkerCount = 16;
	uint64_t GetQueueSize() const { return _msg_que_size.load(std::memory_order_relaxed); }
	uint64_t GetDroppedTotal() const { return _msg_dropped_total.load(std::memory_order_relaxed); }

private:
	LogicSystem();
	size_t dispatchToWorker(const shared_ptr<LogicNode>& msg);
	void workerLoop(size_t worker_id);
	void RegisterCallBacks();
	void LoginHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data);
	void GetRelationBootstrapHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
	void SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void HeartBeatHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void CreateGroupHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void GetGroupListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void GetDialogListHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void InviteGroupMemberHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void ApplyJoinGroupHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void ReviewGroupApplyHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void DealGroupChatMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void GroupHistoryHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void EditGroupMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void RevokeGroupMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void EditPrivateMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void RevokePrivateMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void PrivateHistoryHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void SyncDraftHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void PinDialogHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void ForwardGroupMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void ForwardPrivateMsgHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void GroupReadAckHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void PrivateReadAckHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void UpdateGroupAnnouncementHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void UpdateGroupIconHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void SetGroupAdminHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void MuteGroupMemberHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void KickGroupMemberHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void QuitGroupHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
	void PushGroupPayload(const std::vector<int>& recipients, short msgid, const memochat::json::JsonValue& payload, int exclude_uid = 0);
	void BuildGroupListJson(int uid, memochat::json::JsonValue& out);
	void BuildDialogListJson(int uid, memochat::json::JsonValue& out);
	void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out);
	bool isPureDigit(const std::string& str);
	void GetUserByUid(std::string uid_str, memochat::json::JsonValue& rtvalue);
	void GetUserByName(std::string name, memochat::json::JsonValue& rtvalue);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo> &userinfo);
	bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>> & user_list);
	void DealAsyncEvents();
	void DealTasks();
	void HandlePrivateAsyncEvent(const memochat::json::JsonValue& root);
	void HandleGroupAsyncEvent(const memochat::json::JsonValue& root);
	void NotifyMessageStatus(const memochat::json::JsonValue& payload);

	// Multi-worker thread pool for message processing
	static constexpr size_t kMaxWorkers = 16;
	std::array<std::thread, kMaxWorkers> _worker_threads;
	std::array<std::queue<shared_ptr<LogicNode>>, kMaxWorkers> _worker_queues;
	std::array<std::mutex, kMaxWorkers> _worker_mutexes;
	std::unique_ptr<std::condition_variable[]> _worker_conds;
	std::atomic<bool> _b_stop{false};
	size_t _num_workers = kDefaultWorkerCount;
	std::atomic<size_t> _next_worker{0};

	std::thread _event_worker_thread;
	std::thread _task_worker_thread;
	bool _event_stop = false;

	std::map<short, FunCallBack> _fun_callbacks;
	std::shared_ptr<CServer> _p_server;
	std::unique_ptr<ChatSessionService> _chat_session_service;
	std::unique_ptr<ChatRelationService> _chat_relation_service;
	std::unique_ptr<PrivateMessageService> _private_message_service;
	std::unique_ptr<GroupMessageService> _group_message_service;
	std::unique_ptr<AsyncEventDispatcher> _async_event_dispatcher;
	std::shared_ptr<IAsyncEventBus> _async_event_bus;
	std::unique_ptr<TaskDispatcher> _task_dispatcher;
	std::shared_ptr<IAsyncTaskBus> _task_bus;
	std::unique_ptr<MessageDeliveryService> _message_delivery_service;

	// Bounded queue metrics
	std::atomic<uint64_t> _msg_que_size{0};
	std::atomic<uint64_t> _msg_dropped_total{0};
	std::atomic<uint64_t> _backpressure_log_counter{0};
};
