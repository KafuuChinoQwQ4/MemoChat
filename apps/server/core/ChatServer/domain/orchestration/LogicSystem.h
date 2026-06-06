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
#include "ports/IOutboxRepairScheduler.h"

#include <unordered_map>
#include "data.h"
#include <atomic>
#include <vector>
#include <memory>
#include <array>
#include <mutex>
#include <condition_variable>

class CServer;
typedef function<void(shared_ptr<CSession>, const short& msg_id, const string& msg_data)> FunCallBack;
class ChatSessionServiceRegistrar;
class ChatRelationServiceRegistrar;
class PrivateMessageServiceRegistrar;
class GroupMessageServiceRegistrar;
class AsyncEventDispatcherRegistrar;
class ChatSessionService;
class AsyncEventDispatcher;
class IAsyncEventBus;
class IAsyncTaskBus;
class IChatSessionConfig;
class IChatSessionRepository;
class IMessageRepository;
class IMessageServiceConfig;
class IRelationRepository;
class IRelationBootstrapCache;
class IRelationQueryService;
class IRelationQueryServiceConfig;
class IRelationServiceConfig;
class IRelationService;
class IPrivateMessageService;
class IGroupMessageService;
class IOnlineRouteStore;
class MessageDeliveryService;
class TaskDispatcher;
class ChatDeliveryRuntime;

class LogicSystem
    : public Singleton<LogicSystem>
    , public IOutboxRepairScheduler
{
    friend class Singleton<LogicSystem>;
    friend class ChatSessionServiceRegistrar;
    friend class ChatRelationServiceRegistrar;
    friend class PrivateMessageServiceRegistrar;
    friend class GroupMessageServiceRegistrar;
    friend class AsyncEventDispatcherRegistrar;
    friend class ChatSessionService;
    friend class AsyncEventDispatcher;
    friend class MessageDeliveryService;

public:
    ~LogicSystem();
    void PostMsgToQue(shared_ptr<LogicNode> msg);
    void SetServer(std::shared_ptr<CServer> pserver);
    MessageDeliveryService& MessageDelivery();
    bool PublishTask(const std::string& task_type,
                     const std::string& routing_key,
                     const memochat::json::JsonValue& payload,
                     int delay_ms = 0,
                     int max_retries = 0,
                     std::string* error = nullptr);
    bool
    PublishAsyncEvent(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error = nullptr);
    bool ExpediteOutboxRepair(int64_t outbox_id) override;

    static constexpr size_t kDefaultWorkerCount = 4;
    static constexpr size_t kMaxWorkerCount = 16;
    uint64_t GetQueueSize() const
    {
        return _msg_que_size.load(std::memory_order_relaxed);
    }
    uint64_t GetDroppedTotal() const
    {
        return _msg_dropped_total.load(std::memory_order_relaxed);
    }

private:
    LogicSystem();
    size_t dispatchToWorker(const shared_ptr<LogicNode>& msg);
    void workerLoop(size_t worker_id);
    void RegisterCallBacks();
    void DealAsyncEvents();
    void DealTasks();

    // Multi-worker thread pool for message processing
    static constexpr size_t kMaxWorkers = 16;
    std::array<std::thread, kMaxWorkers> _worker_threads;
    std::array<std::queue<shared_ptr<LogicNode>>, kMaxWorkers> _worker_queues;
    std::array<std::mutex, kMaxWorkers> _worker_mutexes;
    std::unique_ptr<std::condition_variable[]> _worker_conds;
    std::atomic<bool> _b_stop{false};
    size_t _num_workers = kDefaultWorkerCount;
    std::atomic<size_t> _next_worker{0};

    std::map<short, FunCallBack> _fun_callbacks;
    std::shared_ptr<CServer> _p_server;
    std::unique_ptr<ChatSessionService> _chat_session_service;
    std::unique_ptr<IRelationService> _chat_relation_service;
    std::unique_ptr<IPrivateMessageService> _private_message_service;
    std::unique_ptr<IGroupMessageService> _group_message_service;
    std::unique_ptr<MessageDeliveryService> _message_delivery_service;
    std::unique_ptr<AsyncEventDispatcher> _async_event_dispatcher;
    std::shared_ptr<IAsyncEventBus> _async_event_bus;
    std::unique_ptr<IChatSessionConfig> _session_config;
    std::unique_ptr<IChatSessionRepository> _session_repository;
    std::unique_ptr<IMessageServiceConfig> _message_service_config;
    std::unique_ptr<IMessageRepository> _message_repository;
    std::unique_ptr<IRelationRepository> _relation_repository;
    std::unique_ptr<IRelationBootstrapCache> _relation_bootstrap_cache;
    std::unique_ptr<IRelationQueryServiceConfig> _relation_query_service_config;
    std::unique_ptr<IRelationQueryService> _relation_query_service_remote;
    std::unique_ptr<IRelationServiceConfig> _relation_service_config;
    std::unique_ptr<IOnlineRouteStore> _online_route_store;
    std::unique_ptr<TaskDispatcher> _task_dispatcher;
    std::shared_ptr<IAsyncTaskBus> _task_bus;
    std::unique_ptr<ChatDeliveryRuntime> _delivery_runtime;

    // Bounded queue metrics
    std::atomic<uint64_t> _msg_que_size{0};
    std::atomic<uint64_t> _msg_dropped_total{0};
    std::atomic<uint64_t> _backpressure_log_counter{0};
};
