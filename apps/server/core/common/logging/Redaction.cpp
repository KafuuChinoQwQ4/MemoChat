#include "logging/Redaction.h"
#include <algorithm>
#include <cctype>

namespace memolog {

using JsonValue = glz::generic_json<>;
namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string RedactToken(const std::string& value) {
    if (value.size() <= 8U) {
        return "****";
    }
    return value.substr(0, 4) + "..." + value.substr(value.size() - 4);
}

std::string RedactEmail(const std::string& value) {
    const auto at = value.find('@');
    if (at == std::string::npos || at == 0U) {
        return "****";
    }
    const auto local = value.substr(0, at);
    const auto domain = value.substr(at);
    if (local.size() <= 2U) {
        return local.substr(0, 1) + "***" + domain;
    }
    return local.substr(0, 2) + "***" + domain;
}

} // namespace

bool IsSensitiveKey(const std::string& key) {
    const std::string lower = ToLower(key);
    return lower == "passwd" || lower == "password" || lower == "token" ||
           lower == "access_token" || lower == "refresh_token" ||
           lower == "authorization" || lower == "cookie" || lower == "email" ||
           lower == "phone" || lower == "verify_code";
}

std::string RedactValue(const std::string& key, const std::string& value, bool enabled) {
    if (!enabled || !IsSensitiveKey(key)) {
        return value;
    }
    const std::string lower = ToLower(key);
    if (lower == "email") {
        return RedactEmail(value);
    }
    if (lower == "token" || lower == "access_token" || lower == "refresh_token" ||
        lower == "authorization" || lower == "cookie") {
        return RedactToken(value);
    }
    return "****";
}

JsonValue RedactJson(const JsonValue& input, bool enabled) {
    if (!enabled) {
        return input;
    }

    if (input.is_object()) {
        using object_t = JsonValue::object_t;
        JsonValue out{object_t{}};
        if (auto* obj = input.get_if<object_t>()) {
            for (const auto& [key, value] : *obj) {
                if (value.is_string() && IsSensitiveKey(key)) {
                    out.get_object()[key] = JsonValue{RedactValue(key, value.get_string(), true)};
                } else {
                    out.get_object()[key] = RedactJson(value, enabled);
                }
            }
        }
        return out;
    }

    if (input.is_array()) {
        using array_t = JsonValue::array_t;
        array_t arr;
        if (auto* vec = input.get_if<array_t>()) {
            for (const auto& v : *vec) {
                arr.push_back(RedactJson(v, enabled));
            }
        }
        return JsonValue{std::move(arr)};
    }

    return input;
}

} // namespace memolog
