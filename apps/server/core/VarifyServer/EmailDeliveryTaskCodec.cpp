#include "EmailDeliveryTaskCodec.hpp"

#include "json/TypedJsonCodec.hpp"

import memochat.varify.email_delivery_task_algorithms;

#include <utility>

namespace varifyservice
{

std::string SerializeEmailTask(const EmailDeliveryTaskPayload& task)
{
    return memochat::json::WriteTypedJsonOrEmptyObject(task);
}

bool ParseEmailTask(const std::string& body, EmailDeliveryTaskPayload* out)
{
    if (!memochat::varify::email_delivery::modules::HasParseOutput(out != nullptr))
    {
        return false;
    }

    EmailDeliveryTaskPayload parsed;
    if (!memochat::json::ReadTypedJson(body, &parsed))
    {
        return false;
    }

    if (!memochat::varify::email_delivery::modules::HasRequiredEmailTaskFields(parsed.email.empty(),
                                                                               parsed.code.empty()))
    {
        return false;
    }

    *out = std::move(parsed);
    return true;
}

} // namespace varifyservice
