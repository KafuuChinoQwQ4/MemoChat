#pragma once

#include <string>
#include <vector>

namespace Json {
class Value;
}

class MessageDeliveryService {
public:
    MessageDeliveryService() = default;

    void PushPayload(const std::vector<int>& recipients, short msgid, const Json::Value& payload, int exclude_uid = 0);
};
