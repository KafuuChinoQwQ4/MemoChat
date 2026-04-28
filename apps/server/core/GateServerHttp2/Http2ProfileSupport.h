#pragma once
#include <string>
#include <string_view>
#include "json/GlazeCompat.h"

namespace Http2ProfileSupport {

struct ProfileResult {
    int error = 0;
    std::string message;
    memochat::json::JsonValue data;
};

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root);
memochat::json::JsonValue MakeError(int error_code, const std::string& message);
ProfileResult HandleUserUpdateProfile(const memochat::json::JsonValue& req);
ProfileResult HandleGetUserInfo(int uid);

}  // namespace Http2ProfileSupport

