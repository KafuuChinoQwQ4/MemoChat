#pragma once

#include "json/GlazeCompat.h"

#include <string>

class IDeliveryTaskPublisher
{
public:
    virtual ~IDeliveryTaskPublisher() = default;

    virtual bool PublishDeliveryTask(const std::string& task_type,
                                     const std::string& routing_key,
                                     const memochat::json::JsonValue& payload,
                                     int delay_ms,
                                     int max_retries,
                                     std::string* error = nullptr) = 0;
};
