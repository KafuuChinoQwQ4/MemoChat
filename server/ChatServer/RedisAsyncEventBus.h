#pragma once

#include "IAsyncEventBus.h"

class RedisAsyncEventBus : public IAsyncEventBus {
public:
    bool Publish(const std::string& topic, const Json::Value& payload, std::string* error = nullptr) override;
    bool ConsumeOnce(const std::vector<std::string>& topics, AsyncConsumedEvent& event, std::string* error = nullptr) override;
    void AckLastConsumed() override;
    void NackLastConsumed(const std::string& error) override;

private:
    std::string _last_topic;
    std::string _last_serialized;
};
