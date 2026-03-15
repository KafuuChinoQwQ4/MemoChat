#pragma once
#include "Singleton.h"
#include <queue>
#include <thread>
#include "CSession.h"
#include <queue>
#include <map>
#include <functional>
#include "const.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <unordered_map>
#include "data.h"

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
	bool PublishTask(const std::string& task_type, const std::string& routing_key, const Json::Value& payload, int delay_ms = 0, int max_retries = 0, std::string* error = nullptr);
	bool PublishAsyncEvent(const std::string& topic, const Json::Value& payload, std::string* error = nullptr);
	bool ExpediteOutboxRepair(int64_t outbox_id);
private:
	LogicSystem();
	void DealMsg();
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
	void PushGroupPayload(const std::vector<int>& recipients, short msgid, const Json::Value& payload, int exclude_uid = 0);
	void BuildGroupListJson(int uid, Json::Value& out);
	void BuildDialogListJson(int uid, Json::Value& out);
	void AppendRelationBootstrapJson(int uid, Json::Value& out);
	bool isPureDigit(const std::string& str);
	void GetUserByUid(std::string uid_str, Json::Value& rtvalue);
	void GetUserByName(std::string name, Json::Value& rtvalue);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo> &userinfo);
	bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>> & user_list);
	void DealAsyncEvents();
	void DealTasks();
	void HandlePrivateAsyncEvent(const Json::Value& root);
	void HandleGroupAsyncEvent(const Json::Value& root);
	void NotifyMessageStatus(const Json::Value& payload);
	std::thread _worker_thread;
	std::thread _event_worker_thread;
	std::thread _task_worker_thread;
	std::queue<shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	bool _b_stop;
	bool _event_stop;
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
};
