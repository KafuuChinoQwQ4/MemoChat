#pragma once

#include "ChatTaskEnvelope.h"

#include <string>
#include <vector>

class IAsyncTaskBus {
public:
    virtual ~IAsyncTaskBus() = default;

    virtual bool Publish(const TaskEnvelope& task, std::string* error = nullptr) = 0;
    virtual bool ConsumeOnce(const std::vector<std::string>& routing_keys, ConsumedTask& task, std::string* error = nullptr) = 0;
    virtual void AckLastConsumed() = 0;
    virtual void NackLastConsumed(const std::string& error) = 0;
};
