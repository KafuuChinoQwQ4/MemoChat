#pragma once
#include "Singleton.h"
#include "json/GlazeCompat.h"
#include "ports/IOutboxRepairScheduler.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <map>
#include <memory>
#include <mutex>
#include <string>

class CSession;
class CServer;
class ChatRuntimeComposition;
class LogicNode;
class MessageDeliveryService;

typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)> FunCallBack;

class LogicSystem
    : public Singleton<LogicSystem>
    , public IOutboxRepairScheduler
{
    friend class Singleton<LogicSystem>;
    friend class ChatSessionService;

public:
    ~LogicSystem();
    void PostMsgToQue(std::shared_ptr<LogicNode> msg);
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
    size_t dispatchToWorker(const std::shared_ptr<LogicNode>& msg);
    void workerLoop(size_t worker_id);
    void RegisterCallBacks();

    // Multi-worker thread pool for message processing
    static constexpr size_t kMaxWorkers = 16;
    std::array<std::thread, kMaxWorkers> _worker_threads;
    std::array<std::queue<std::shared_ptr<LogicNode>>, kMaxWorkers> _worker_queues;
    std::array<std::mutex, kMaxWorkers> _worker_mutexes;
    std::unique_ptr<std::condition_variable[]> _worker_conds;
    std::atomic<bool> _b_stop{false};
    size_t _num_workers = kDefaultWorkerCount;
    std::atomic<size_t> _next_worker{0};

    std::map<short, FunCallBack> _fun_callbacks;
    std::shared_ptr<CServer> _p_server;
    std::unique_ptr<ChatRuntimeComposition> _composition;

    // Bounded queue metrics
    std::atomic<uint64_t> _msg_que_size{0};
    std::atomic<uint64_t> _msg_dropped_total{0};
    std::atomic<uint64_t> _backpressure_log_counter{0};
};
