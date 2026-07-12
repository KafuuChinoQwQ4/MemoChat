#pragma once
#include "Singleton.hpp"
#include "json/GlazeCompat.hpp"
#include "ports/ILogicSystemConfig.hpp"
#include "runtime/ExplicitThread.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <queue>
#include <map>
#include <memory>
#include <mutex>
#include <string>

class IChatSession;
class IChatSessionHost;
class ChatRuntimeComposition;
class LogicNode;
class MessageDeliveryService;

typedef std::function<void(std::shared_ptr<IChatSession>, const short& msg_id, const std::string& msg_data)>
    FunCallBack;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
    friend class ChatSessionService;

public:
    ~LogicSystem();
    bool Ready() const;
    const std::string& startupError() const;
    void PostMsgToQue(std::shared_ptr<LogicNode> msg);
    void SetServer(IChatSessionHost* host);
    MessageDeliveryService& MessageDelivery();
    bool PublishTask(const std::string& task_type,
                     const std::string& routing_key,
                     const memochat::json::JsonValue& payload,
                     int delay_ms = 0,
                     int max_retries = 0,
                     std::string* error = nullptr);
    bool
    PublishAsyncEvent(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error = nullptr);

    // Must be called by the infrastructure layer before the first GetInstance()
    // so the domain constructor never needs to name the concrete config type.
    static void SetWorkerConfig(const ILogicSystemConfig& cfg);

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
    std::array<memochat::runtime::ExplicitThread, kMaxWorkers> _worker_threads;
    std::array<std::queue<std::shared_ptr<LogicNode>>, kMaxWorkers> _worker_queues;
    std::array<std::mutex, kMaxWorkers> _worker_mutexes;
    std::unique_ptr<std::condition_variable[]> _worker_conds;
    std::atomic<bool> _b_stop{false};
    size_t _num_workers = kDefaultWorkerCount;
    std::atomic<size_t> _next_worker{0};

    std::map<short, FunCallBack> _fun_callbacks;
    IChatSessionHost* _p_server{nullptr};
    std::unique_ptr<ChatRuntimeComposition> _composition;
    std::string _startup_error;

    // Bounded queue metrics
    std::atomic<uint64_t> _msg_que_size{0};
    std::atomic<uint64_t> _msg_dropped_total{0};
    std::atomic<uint64_t> _backpressure_log_counter{0};
};
