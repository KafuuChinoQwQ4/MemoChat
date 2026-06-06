#pragma once

#include "json/GlazeCompat.h"

#include <vector>

class IDeliveryGateway
{
public:
    virtual ~IDeliveryGateway() = default;

    virtual void PushPayload(const std::vector<int>& recipients,
                             short msgid,
                             const memochat::json::JsonValue& payload,
                             int exclude_uid = 0) = 0;
    virtual bool TryPushPayload(const std::vector<int>& recipients,
                                short msgid,
                                const memochat::json::JsonValue& payload,
                                int exclude_uid = 0,
                                bool enqueue_on_failure = false) = 0;
};
