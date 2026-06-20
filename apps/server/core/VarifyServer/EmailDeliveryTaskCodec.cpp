#include "EmailDeliveryTaskCodec.h"

#include "json/TypedJsonCodec.h"

#include <utility>

namespace varifyservice
{

std::string SerializeEmailTask(const EmailDeliveryTaskPayload& task)
{
    return memochat::json::WriteTypedJsonOrEmptyObject(task);
}

bool ParseEmailTask(const std::string& body, EmailDeliveryTaskPayload* out)
{
    if (!out)
    {
        return false;
    }

    EmailDeliveryTaskPayload parsed;
    if (!memochat::json::ReadTypedJson(body, &parsed))
    {
        return false;
    }

    if (parsed.email.empty() || parsed.code.empty())
    {
        return false;
    }

    *out = std::move(parsed);
    return true;
}

} // namespace varifyservice
