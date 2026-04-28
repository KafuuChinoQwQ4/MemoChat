#pragma once

#include "ChatAsyncEvent.h"

#include <string>
#include <vector>

class IAsyncEventBus {
public:
    virtual ~IAsyncEventBus() = default;

    virtual bool Publish(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error = nullptr) = 0;
    virtual bool ConsumeOnce(const std::vector<std::string>& topics, AsyncConsumedEvent& event, std::string* error = nullptr) = 0;
    virtual void AckLastConsumed() = 0;
    virtual void NackLastConsumed(const std::string& error) = 0;
};
