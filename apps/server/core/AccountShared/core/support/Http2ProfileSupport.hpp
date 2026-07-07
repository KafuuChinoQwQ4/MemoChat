#pragma once

#include <string>
#include <string_view>
#include "json/GlazeCompat.hpp"

namespace Http2ProfileSupport
{

struct ProfileResult
{
    int error = 0;
    std::string message;
    memochat::json::JsonValue data;
};

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root);
memochat::json::JsonValue MakeError(int error_code, const std::string& message);

ProfileResult HandleUserUpdateProfile(const memochat::json::JsonValue& req, const std::string& access_token);
ProfileResult HandleGetUserInfo(const std::string& token);

} // namespace Http2ProfileSupport
