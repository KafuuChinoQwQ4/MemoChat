#pragma once

#include "json/GlazeCompat.hpp"

#include <string>

class IEventPublisher
{
public:
    virtual ~IEventPublisher() = default;

    virtual bool
    PublishEvent(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error = nullptr) = 0;
};
