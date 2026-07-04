#include "logging/Redaction.hpp"

import memochat.logging.redaction_algorithms;

namespace memolog
{

using JsonValue = glz::generic_json<>;
namespace
{

std::string ToLower(std::string value)
{
    memochat::logging::redaction_modules::LowerAsciiInPlace(value.data(), value.size());
    return value;
}

std::string RedactToken(const std::string& value)
{
    if (memochat::logging::redaction_modules::ShouldCollapseTokenMask(value.size()))
    {
        return "****";
    }
    return value.substr(0, 4) + "..." + value.substr(value.size() - 4);
}

std::string RedactEmail(const std::string& value)
{
    const auto at = value.find('@');
    if (!memochat::logging::redaction_modules::IsMaskableEmail(at != std::string::npos, at))
    {
        return "****";
    }
    const auto local = value.substr(0, at);
    const auto domain = value.substr(at);
    const auto visible_prefix = memochat::logging::redaction_modules::VisibleEmailLocalPrefix(local.size());
    if (visible_prefix == 1U)
    {
        return local.substr(0, 1) + "***" + domain;
    }
    return local.substr(0, 2) + "***" + domain;
}

} // namespace

bool IsSensitiveKey(const std::string& key)
{
    const std::string lower = ToLower(key);
    const int kind = memochat::logging::redaction_modules::ClassifyLowerSensitiveKey(lower.data(), lower.size());
    return memochat::logging::redaction_modules::IsSensitiveKind(kind);
}

std::string RedactValue(const std::string& key, const std::string& value, bool enabled)
{
    const std::string lower = ToLower(key);
    const int kind = memochat::logging::redaction_modules::ClassifyLowerSensitiveKey(lower.data(), lower.size());
    if (!enabled || !memochat::logging::redaction_modules::IsSensitiveKind(kind))
    {
        return value;
    }
    if (memochat::logging::redaction_modules::ShouldRedactAsEmail(kind))
    {
        return RedactEmail(value);
    }
    if (memochat::logging::redaction_modules::ShouldRedactAsToken(kind))
    {
        return RedactToken(value);
    }
    return "****";
}

JsonValue RedactJson(const JsonValue& input, bool enabled)
{
    if (!enabled)
    {
        return input;
    }

    if (input.is_object())
    {
        using object_t = JsonValue::object_t;
        JsonValue out{object_t{}};
        if (auto* obj = input.get_if<object_t>())
        {
            for (const auto& [key, value] : *obj)
            {
                if (value.is_string() && IsSensitiveKey(key))
                {
                    out.get_object()[key] = JsonValue{RedactValue(key, value.get_string(), true)};
                }
                else
                {
                    out.get_object()[key] = RedactJson(value, enabled);
                }
            }
        }
        return out;
    }

    if (input.is_array())
    {
        using array_t = JsonValue::array_t;
        array_t arr;
        if (auto* vec = input.get_if<array_t>())
        {
            for (const auto& v : *vec)
            {
                arr.push_back(RedactJson(v, enabled));
            }
        }
        return JsonValue{std::move(arr)};
    }

    return input;
}

} // namespace memolog
