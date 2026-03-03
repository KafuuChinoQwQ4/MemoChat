#include "logging/Redaction.h"

#include <algorithm>
#include <cctype>

namespace memolog {
namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
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
    return lower == "passwd" || lower == "password" || lower == "token" || lower == "email";
}

std::string RedactValue(const std::string& key, const std::string& value, bool enabled) {
    if (!enabled || !IsSensitiveKey(key)) {
        return value;
    }
    const std::string lower = ToLower(key);
    if (lower == "email") {
        return RedactEmail(value);
    }
    if (lower == "token") {
        return RedactToken(value);
    }
    return "****";
}

Json::Value RedactJson(const Json::Value& input, bool enabled) {
    if (!enabled) {
        return input;
    }
    if (input.isObject()) {
        Json::Value out(Json::objectValue);
        const auto members = input.getMemberNames();
        for (const auto& key : members) {
            const auto& value = input[key];
            if (value.isString() && IsSensitiveKey(key)) {
                out[key] = RedactValue(key, value.asString(), true);
            } else {
                out[key] = RedactJson(value, enabled);
            }
        }
        return out;
    }
    if (input.isArray()) {
        Json::Value out(Json::arrayValue);
        for (const auto& v : input) {
            out.append(RedactJson(v, enabled));
        }
        return out;
    }
    return input;
}

} // namespace memolog
