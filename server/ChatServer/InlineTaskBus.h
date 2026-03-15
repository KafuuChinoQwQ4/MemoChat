#pragma once

#include "IAsyncTaskBus.h"

#include <condition_variable>
#include <deque>
#include <mutex>

class InlineTaskBus : public IAsyncTaskBus {
public:
    bool Publish(const TaskEnvelope& task, std::string* error = nullptr) override;
    bool ConsumeOnce(const std::vector<std::string>& routing_keys, ConsumedTask& task, std::string* error = nullptr) override;
    void AckLastConsumed() override;
    void NackLastConsumed(const std::string& error) override;

private:
    bool RoutingKeyAccepted(const std::vector<std::string>& routing_keys, const TaskEnvelope& task) const;

    std::deque<TaskEnvelope> _queue;
    std::mutex _mutex;
    ConsumedTask _last_consumed;
    bool _has_last = false;
};
