#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace varifyservice {

class EmailSender;

struct EmailDeliveryTask {
    std::string email;
    std::string code;
    std::string trace_id;
    int retry_count = 0;
};

class EmailTaskBus {
public:
    static EmailTaskBus& Instance();

    bool PublishVerifyDeliveryTask(const std::string& email,
                                  const std::string& code,
                                  const std::string& trace_id);

    bool IsHealthy() const { return rabbitmq_healthy_.load(std::memory_order_relaxed); }
    bool IsStarted() const { return started_.load(std::memory_order_acquire); }

    void StartWorker(EmailSender* sender);
    void StopWorker();

private:
    EmailTaskBus();
    ~EmailTaskBus();

    bool Connect();
    void Close();
    bool EnsureTopology();
    bool PublishToQueue(const std::string& routing_key, const std::string& body);

    bool ConsumeOnce(EmailDeliveryTask& task);
    void WorkerLoop(EmailSender* sender);
    void ClearLastConsumed();
    void AckLastConsumed();

    std::string config_host_;
    int config_port_ = 5672;
    std::string config_username_;
    std::string config_password_;
    std::string config_vhost_;
    std::string config_queue_;
    std::string config_retry_routing_key_;
    std::string config_dlq_routing_key_;
    int config_retry_delay_ms_ = 5000;
    int config_max_retries_ = 5;

    std::atomic<bool> started_{false};
    std::atomic<bool> rabbitmq_healthy_{false};
    std::atomic<bool> stop_{false};
    std::thread worker_thread_;

    void* connection_ = nullptr;
    void* last_envelope_ = nullptr;
    std::mutex mutex_;

    std::queue<EmailDeliveryTask> pending_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

} // namespace varifyservice
