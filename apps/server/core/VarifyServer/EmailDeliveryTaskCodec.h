#pragma once

#include <string>

namespace varifyservice
{

struct EmailDeliveryTaskPayload
{
    std::string email;
    std::string code;
    std::string trace_id;
    int retry_count = 0;
};

std::string SerializeEmailTask(const EmailDeliveryTaskPayload& task);
bool ParseEmailTask(const std::string& body, EmailDeliveryTaskPayload* out);

} // namespace varifyservice
