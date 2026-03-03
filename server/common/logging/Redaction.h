#pragma once

#include <json/value.h>

#include <string>

namespace memolog {

bool IsSensitiveKey(const std::string& key);
std::string RedactValue(const std::string& key, const std::string& value, bool enabled);
Json::Value RedactJson(const Json::Value& input, bool enabled);

} // namespace memolog
