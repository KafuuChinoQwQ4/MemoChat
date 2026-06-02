#include "EmailDeliveryTaskCodec.h"

#include "json/GlazeCompat.h"

#include <utility>

namespace varifyservice
{

std::string SerializeEmailTask(const EmailDeliveryTaskPayload& task)
{
    memochat::json::JsonValue root(memochat::json::object_t{});
    root["email"] = task.email;
    root["code"] = task.code;
    root["trace_id"] = task.trace_id;
    root["retry_count"] = task.retry_count;

    return memochat::json::glaze_stringify(root);
}

bool ParseEmailTask(const std::string& body, EmailDeliveryTaskPayload* out)
{
    if (!out)
    {
        return false;
    }

    memochat::json::JsonValue root;
    if (!memochat::json::reader_parse(body, root) || !root.is_object())
    {
        return false;
    }

    EmailDeliveryTaskPayload parsed;
    parsed.email = memochat::json::glaze_safe_get<std::string>(root, "email", "");
    parsed.code = memochat::json::glaze_safe_get<std::string>(root, "code", "");
    parsed.trace_id = memochat::json::glaze_safe_get<std::string>(root, "trace_id", "");
    parsed.retry_count = memochat::json::glaze_safe_get<int>(root, "retry_count", 0);

    if (parsed.email.empty() || parsed.code.empty())
    {
        return false;
    }

    *out = std::move(parsed);
    return true;
}

} // namespace varifyservice
