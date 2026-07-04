#pragma once

#include <string>
#include "json/GlazeCompat.hpp"

namespace memolog
{

bool IsSensitiveKey(const std::string& key);
std::string RedactValue(const std::string& key, const std::string& value, bool enabled);
memochat::json::JsonValue RedactJson(const memochat::json::JsonValue& input, bool enabled);

} // namespace memolog
