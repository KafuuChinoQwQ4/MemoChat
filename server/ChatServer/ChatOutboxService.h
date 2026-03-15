#pragma once

#include "KafkaConfig.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

class ChatOutboxService {
public:
    using PublishFn = std::function<bool(const std::string&, const std::string&, const std::string&, std::string*)>;
    using PublishRepairTaskFn = std::function<void(int64_t, int, int, const std::string&)>;

    ChatOutboxService(const KafkaConfig& config, PublishFn publish_fn, PublishRepairTaskFn publish_repair_task_fn = nullptr);
    ~ChatOutboxService();

    void Start();
    void Stop();

private:
    void RunLoop();

    KafkaConfig _config;
    PublishFn _publish_fn;
    PublishRepairTaskFn _publish_repair_task_fn;
    std::atomic<bool> _stop{ false };
    std::thread _thread;
};
